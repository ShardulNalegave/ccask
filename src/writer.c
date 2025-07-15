
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

    size_t record_size = ccask_get_hintfile_record_total_size(record);
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
    
    CCASK_RETRY(5, res, ccask_keydir_upsert(key, header.key_size, file->file_id, pos, header.value_size, header.timestamp));
    if (res != CCASK_OK) {
        log_error("Record written to Active datafile but couldn't update Key-Directory");
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

static void* writer_thread_main(void*) {
    ccask_datafile_record_t record;
    while (true) {
        int res = ccask_writer_ringbuf_pop(record);
        if (res != CCASK_OK) break;

        ccask_write_record_blocking(record);
        free_datafile_record(record);
    }
}

ccask_status_e ccask_writer_start(size_t capacity) {
    if (ccask_writer_ringbuf_init(capacity) != CCASK_OK)
        return CCASK_FAIL;
    
    int res;
    CCASK_RETRY(5, res, pthread_create(&writer_thread, NULL, writer_thread_main, NULL));
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
