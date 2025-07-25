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

#include "ccask/writer.h"

#include "stdbool.h"
#include "pthread.h"
#include "unistd.h"
#include "ccask/keydir.h"
#include "ccask/files.h"
#include "ccask/writer_ringbuf.h"
#include "ccask/utils.h"
#include "ccask/log.h"

static pthread_t writer_thread;

ccask_status_e ccask_write_record_blocking(ccask_datafile_record_t record) {
    ccask_file_t *file = ccask_files_get_active_file();

    pthread_rwlock_wrlock(&file->rwlock);

    off_t pos = lseek(file->fd, 0, SEEK_END);
    if (pos < 0) {
        log_error("lseek failed during write record to active-datafile");
        pthread_rwlock_unlock(&file->rwlock);
        return CCASK_FAIL;
    }

    size_t record_size = ccask_get_datafile_record_total_size(record);
    if ((size_t)pos + record_size > MAX_ACTIVE_FILE_SIZE) {
        ccask_files_rotate();
        pthread_rwlock_unlock(&file->rwlock);
        return ccask_write_record_blocking(record);
    }

    int res = safe_writev(file->fd, record, 3);
    if (res != CCASK_OK) {
        log_error("Failed to write datafile-record to active datafile");
        pthread_rwlock_unlock(&file->rwlock);
        return CCASK_FAIL;
    }

    pthread_rwlock_unlock(&file->rwlock);

    ccask_datafile_record_header_t header = ccask_get_datafile_record_header(record);
    void *key = ccask_get_datafile_record_key(record);
    
    CCASK_ATTEMPT(5, res, ccask_keydir_upsert(key, header.key_size, file->file_id, pos, header.value_size, header.timestamp));
    if (res != CCASK_OK) {
        log_error("Record written to Active datafile but couldn't update Key-Directory");
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

static void* writer_thread_main(void *arg) {
    (void)arg;

    ccask_datafile_record_t record;
    while (true) {
        int res = ccask_writer_ringbuf_pop(record);
        if (res != CCASK_OK) break;

        ccask_write_record_blocking(record);
        free_datafile_record(record);
    }

    return NULL;
}

ccask_status_e ccask_writer_start(size_t capacity) {
    if (ccask_writer_ringbuf_init(capacity) != CCASK_OK)
        return CCASK_FAIL;
    
    int res;
    CCASK_ATTEMPT(5, res, pthread_create(&writer_thread, NULL, writer_thread_main, NULL));
    if (res != 0) {
        ccask_writer_ringbuf_destroy();
        ccask_errno = CCASK_ERR_COULDNT_START_THREAD;
        log_info("Couldn't start writer");
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

void ccask_writer_stop(void) {
    ccask_writer_ringbuf_start_shutdown();
    pthread_join(writer_thread, NULL);
    ccask_writer_ringbuf_destroy();
}
