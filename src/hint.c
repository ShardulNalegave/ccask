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

#include "ccask/hint.h"

#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "inttypes.h"
#include "ccask/files.h"
#include "ccask/iterator.h"
#include "ccask/utils.h"
#include "ccask/status.h"
#include "ccask/log.h"

typedef struct thread_handle_t {
    pthread_t thread;
    struct thread_handle_t *next;
} thread_handle_t;

static thread_handle_t *threads;

void ccask_hintfile_generator_init(void) {
    threads = NULL;
}

void ccask_hintfile_generator_shutdown(void) {
    thread_handle_t* curr = threads;
    while (curr) {
        pthread_join(curr->thread, NULL);

        thread_handle_t *temp = curr->next;
        free(curr);
        curr = temp;
    }
}

void* hintfile_generator_thread(void* arg) {
    ccask_file_t *file = (ccask_file_t*)arg;
    uint64_t file_id = file->file_id;

    // get file-descriptor for new hintfile
    int hintfile_fd;
    CCASK_ATTEMPT(5, hintfile_fd, ccask_files_get_hintfile_fd(file_id));
    if (hintfile_fd < 0) {
        log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
        return NULL;
    }

    // get iterator for datafile
    int res;
    ccask_datafile_iter_t iter;
    CCASK_ATTEMPT(5, res, ccask_datafile_iter_open(file_id, &iter));
    if (res != CCASK_OK) {
        log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
        return NULL;
    }

    uint64_t record_pos;
    ccask_datafile_record_t record;
    ccask_hintfile_record_t hint_record;
    while (ccask_datafile_iter_next(&iter, record, &record_pos) == CCASK_OK) {
        ccask_datafile_record_header_t header = ccask_get_datafile_record_header(record);
        void *key = ccask_get_datafile_record_key(record);
        int res = ccask_create_hintfile_record(
            hint_record,
            header.timestamp,
            header.key_size,
            header.value_size,
            record_pos,
            key
        );

        if (res != CCASK_OK)
            goto generation_failed;
            
        if (safe_writev(hintfile_fd, hint_record, 2) != CCASK_OK)
            goto generation_failed;

        free_datafile_record(record);
        free_hintfile_record(hint_record);
        continue;

generation_failed:
        log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
        free_datafile_record(record);
        free_hintfile_record(hint_record);
        ccask_datafile_iter_close(&iter);

        close(hintfile_fd);
        CCASK_ATTEMPT(5, res, ccask_files_delete(file_id, FILE_HINT));
        if (res != CCASK_OK) log_error("Couldn't delete partially written hintfile ID = %" PRIu64, file_id);
        
        return NULL;
    }

    ccask_datafile_iter_close(&iter);
    close(hintfile_fd);
    file->has_hint = true;
    log_info("Hintfile generation completed (File ID = %" PRIu64 ")", file_id);
}

ccask_status_e ccask_hintfile_generate(ccask_file_t* file) {
    thread_handle_t* handle = malloc(sizeof(thread_handle_t));
    if (!handle) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_RETRY;
    }

    int res;
    CCASK_ATTEMPT(5, res, pthread_create(&handle->thread, NULL, hintfile_generator_thread, file));

    if (res != 0) {
        log_error("Could not start Hintfile generation thread for ID = %" PRIu64, file->file_id);
        free(handle);
        ccask_errno = CCASK_ERR_COULDNT_START_THREAD;
        return CCASK_FAIL;
    }

    handle->next = threads;
    threads = handle;
    return CCASK_OK;
}
