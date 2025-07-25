/**
 * Copyright (C) 2025  Shardul Nalegave
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include "ccask/writer_ringbuf.h"

#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"
#include "ccask/records.h"
#include "ccask/status.h"
#include "ccask/log.h"

typedef struct ccask_writer_ringbuf {
    ccask_datafile_record_t *buf;
    size_t capacity;
    size_t head;
    size_t tail;

    pthread_mutex_t mutex; // guard for head, tail, buf
    pthread_cond_t not_empty;  // signaled when new item pushed

    bool shutdown;
} ccask_writer_ringbuf_t;

static ccask_writer_ringbuf_t *ringbuf;

size_t ccask_writer_ringbuf_count(void) {
    size_t head, tail;

    pthread_mutex_lock((pthread_mutex_t*)&ringbuf->mutex);
    head = ringbuf->head;
    tail = ringbuf->tail;
    pthread_mutex_unlock((pthread_mutex_t*)&ringbuf->mutex);

    if (head >= tail) return head - tail;
    return ringbuf->capacity - (tail - head);
}

ccask_status_e ccask_writer_ringbuf_init(size_t capacity) {
    ringbuf = malloc(sizeof(ccask_writer_ringbuf_t));
    if (!ringbuf) {
        log_error("Couldn't initialize writer ring-buffer");
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    ringbuf->head = 0;
    ringbuf->tail = 0;
    ringbuf->shutdown = false;
    ringbuf->capacity = capacity;

    ringbuf->buf = calloc(capacity, sizeof(ccask_datafile_record_t));
    if (!ringbuf->buf) {
        free(ringbuf);
        ringbuf = NULL;
        log_error("Couldn't initialize writer ring-buffer");
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    pthread_mutex_init(&ringbuf->mutex, NULL);
    pthread_cond_init(&ringbuf->not_empty, NULL);

    return CCASK_OK;
}

void ccask_writer_ringbuf_start_shutdown(void) {
    pthread_mutex_lock(&ringbuf->mutex);
    ringbuf->shutdown = true;
    pthread_cond_broadcast(&ringbuf->not_empty);
    pthread_mutex_unlock(&ringbuf->mutex);
}

void ccask_writer_ringbuf_destroy(void) {
    pthread_cond_destroy(&ringbuf->not_empty);
    pthread_mutex_destroy(&ringbuf->mutex);
    free(ringbuf->buf);
    free(ringbuf);
}

ccask_status_e ccask_writer_ringbuf_push(
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
) {
    pthread_mutex_lock(&ringbuf->mutex);

    size_t next = (ringbuf->head + 1) % ringbuf->capacity;
    if (next == ringbuf->tail) {
        // full
        pthread_mutex_unlock(&ringbuf->mutex);
        log_warn("Failed to push record onto writer ring-buffer! (Max capacity reached)");
        ccask_errno = CCASK_ERR_RINGBUFFER_FULL;
        return CCASK_RETRY;
    }

    int res = ccask_create_datafile_record(
        ringbuf->buf[ringbuf->head],
        timestamp,
        key, key_size,
        value, value_size
    );
    if (res != CCASK_OK) {
        log_info("Failed to push record onto writer ring-buffer! (Could not create a datafile-record)");
        return CCASK_FAIL;
    }
    
    ringbuf->head = next;

    pthread_cond_signal(&ringbuf->not_empty);

    pthread_mutex_unlock(&ringbuf->mutex);
    return CCASK_OK;
}

ccask_status_e ccask_writer_ringbuf_pop(ccask_datafile_record_t record) {
    pthread_mutex_lock(&ringbuf->mutex);

    while (ringbuf->tail == ringbuf->head && !ringbuf->shutdown) {
        // empty, wait for a push or shutdown
        pthread_cond_wait(&ringbuf->not_empty, &ringbuf->mutex);
    }

    if (ringbuf->shutdown && ringbuf->tail == ringbuf->head) {
        pthread_mutex_unlock(&ringbuf->mutex);
        return CCASK_FAIL;
    }

    memcpy(record, ringbuf->buf[ringbuf->tail], sizeof(ccask_datafile_record_t)); // shallow-copy
    ringbuf->tail = (ringbuf->tail + 1) % ringbuf->capacity;

    pthread_mutex_unlock(&ringbuf->mutex);
    return CCASK_OK;
}
