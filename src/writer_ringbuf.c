
#include "ccask/writer_ringbuf.h"

#include "stdlib.h"
#include "string.h"
#include "unistd.h"
#include "pthread.h"
#include "ccask/records.h"
#include "ccask/errors.h"
#include "ccask/log.h"

#define WRITER_RINGBUF_CAPACITY 10

typedef struct ccask_writer_ringbuf_t {
    ccask_datafile_record_t *buf;
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
    return WRITER_RINGBUF_CAPACITY - (tail - head);
}

int ccask_writer_ringbuf_init(void) {
    int retry_counter = 0;
    do {
        ringbuf = malloc(sizeof(ccask_writer_ringbuf_t));
    } while (!ringbuf && retry_counter++ <= 5);

    if (!ringbuf) {
        log_error("Couldn't initialize writer ring-buffer");
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    ringbuf->head = 0;
    ringbuf->tail = 0;
    ringbuf->shutdown = false;

    retry_counter = 0;
    do {
        ringbuf->buf = calloc(WRITER_RINGBUF_CAPACITY, sizeof(ccask_datafile_record_t));
    } while (!ringbuf->buf && retry_counter++ <= 5);

    if (!ringbuf->buf) {
        free(ringbuf);
        ringbuf = NULL;
        log_error("Couldn't initialize writer ring-buffer");
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    pthread_mutex_init(&ringbuf->mutex, NULL);
    pthread_cond_init(&ringbuf->not_empty, NULL);

    return CCASK_OK;
}

void ccask_writer_ringbuf_shutdown(void) {
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

int ccask_writer_ringbuf_push(
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
) {
    pthread_mutex_lock(&ringbuf->mutex);

    size_t next = (ringbuf->head + 1) % WRITER_RINGBUF_CAPACITY;
    if (next == ringbuf->tail) {
        // full
        pthread_mutex_unlock(&ringbuf->mutex);
        log_warn("Failed to push record onto writer ring-buffer! (Max capacity reached)");
        ccask_errno = ERR_WRITER_RINGBUF_FULL;
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

int ccask_writer_ringbuf_pop(ccask_datafile_record_t record) {
    pthread_mutex_lock(&ringbuf->mutex);

    while (ringbuf->tail == ringbuf->head && !ringbuf->shutdown) {
        // empty, wait for a push or shutdown
        pthread_cond_wait(&ringbuf->not_empty, &ringbuf->mutex);
    }

    if (ringbuf->shutdown && ringbuf->tail == ringbuf->head) {
        pthread_mutex_unlock(&ringbuf->mutex);
        record = NULL;
        return CCASK_OK;
    }

    memcpy(record, ringbuf->buf[ringbuf->tail], sizeof(ccask_datafile_record_t)); // shallow-copy
    ringbuf->tail = (ringbuf->tail + 1) % WRITER_RINGBUF_CAPACITY;

    pthread_mutex_unlock(&ringbuf->mutex);
    return CCASK_OK;
}
