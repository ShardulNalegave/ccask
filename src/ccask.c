
#include "ccask.h"

#include "time.h"
#include "stdatomic.h"
#include "inttypes.h"
#include "ccask/files.h"
#include "ccask/keydir.h"
#include "ccask/writer.h"
#include "ccask/writer_ringbuf.h"
#include "ccask/reader.h"
#include "ccask/hint.h"
#include "ccask/log.h"
#include "ccask/utils.h"

volatile static _Atomic bool is_shutting_down = false;

ccask_status_e ccask_init(ccask_options_t opts) {
    atomic_store(&is_shutting_down, false);
    int res;

    CCASK_ATTEMPT(5, res, ccask_files_init(opts.data_dir, opts.datafile_rotate_threshold));
    if (res != CCASK_OK) {
        log_fatal("Couldn't initialize ccask-files");
        return CCASK_FAIL;
    }

    CCASK_ATTEMPT(5, res, ccask_keydir_init());
    if (res != CCASK_OK) {
        log_fatal("Couldn't initialize keydir");
        return CCASK_FAIL;
    }
    
    CCASK_ATTEMPT(5, res, ccask_writer_start(opts.writer_ringbuf_capacity));
    if (res != CCASK_OK) {
        log_fatal("Couldn't initialize writer");
        return CCASK_FAIL;
    }

    ccask_hintfile_generator_init();
    return CCASK_OK;
}

void ccask_shutdown(void) {
    atomic_store(&is_shutting_down, true);
    ccask_hintfile_generator_shutdown();
    ccask_writer_stop();
    ccask_keydir_shutdown();
    ccask_files_shutdown();
}

void ccask_free_record(ccask_record_t record) {
    free(record.value);
}

ccask_status_e ccask_get(void *key, uint32_t key_size, ccask_record_t *record) {
    ccask_keydir_record_t *kd_record = ccask_keydir_find(key, key_size);
    if (kd_record == NULL) {
        record->value = NULL;
        return CCASK_OK;
    }

    ccask_datafile_record_t df_record;
    if (ccask_allocate_datafile_record(df_record, kd_record->key_size, kd_record->value_size) != CCASK_OK)
        return CCASK_FAIL;
    
    int res;
    CCASK_ATTEMPT(5, res, ccask_read_datafile_record(kd_record->file_id, df_record, kd_record->record_pos));
    if (res != CCASK_OK) {
        log_error("Failed to read datafile record");
        return CCASK_FAIL;
    }

    ccask_datafile_record_header_t header = ccask_get_datafile_record_header(df_record);
    void *read_key = ccask_get_datafile_record_key(df_record);
    void *read_value = ccask_get_datafile_record_value(df_record);

    uint32_t crc = calculate_crc32(
        header.timestamp,
        header.key_size,
        header.value_size,
        read_key,
        read_value
    );

    if (crc != header.crc) {
        log_error(
            "Stored CRC for key doesn't match its actual CRC, returning NULL (%" PRIu32" != %" PRIu32 ")",
            header.crc, crc
        );
        free_datafile_record(df_record);
        ccask_errno = CCASK_ERR_CRC_INVALID;
        return CCASK_FAIL;
    }

    record->value = malloc(kd_record->value_size);
    if (!record->value) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }
    memcpy(record->value, read_value, kd_record->value_size);

    record->timestamp = header.timestamp;
    record->key_size = header.key_size;
    record->value_size = kd_record->value_size;

    free_datafile_record(df_record);
    return CCASK_OK;
}

ccask_status_e ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size) {
    if (atomic_load(&is_shutting_down)) {
        log_error("Cannot put values after shutdown has been initiated");
        return CCASK_FAIL;
    }

    int res;
    CCASK_ATTEMPT(5, res, ccask_writer_ringbuf_push(time(NULL), key, key_size, value, value_size));
    if (res != CCASK_OK) {
        log_error("Couldn't put record into writer ringbuf");
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

ccask_status_e ccask_put_blocking(void *key, uint32_t key_size, void *value, uint32_t value_size) {
    if (atomic_load(&is_shutting_down)) {
        log_error("Cannot put values after shutdown has been initiated");
        return CCASK_FAIL;
    }

    uint32_t timestamp = time(NULL);
    ccask_datafile_record_t record;

    int res = ccask_create_datafile_record(
        record,
        timestamp,
        key, key_size,
        value, value_size
    );

    if (res != CCASK_OK) {
        log_info("Failed to create datafile record during put (blocking)");
        return CCASK_FAIL;
    }

    res = ccask_write_record_blocking(record);
    if (res != CCASK_OK) {
        log_info("Failed to write datafile record during put (blocking)");
        return CCASK_FAIL;
    }

    free_datafile_record(record);
    return CCASK_OK;
}

ccask_status_e ccask_delete(void* key, uint32_t key_size) {
    return ccask_put(key, key_size, NULL, 0);
}

ccask_status_e ccask_delete_blocking(void* key, uint32_t key_size) {
    return ccask_put_blocking(key, key_size, NULL, 0);
}

struct ccask_keys_iter {
    ccask_keydir_record_iter_t keydir_iter;
};

ccask_keys_iter_t* ccask_list_keys(void) {
    ccask_keys_iter_t* iter = malloc(sizeof(ccask_keys_iter_t));
    if (!iter) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return NULL;
    }

    iter->keydir_iter = ccask_keydir_record_iter();
    return iter;
}

ccask_status_e ccask_keys_iter_next(ccask_keys_iter_t *iter, void **key, uint32_t *key_size) {
    ccask_keydir_record_t *record = ccask_keydir_record_iter_next(&iter->keydir_iter);
    if (record == NULL) {
        ccask_errno = CCASK_ERR_ITER_END;
        return CCASK_FAIL;
    }

    *key = record->key;
    *key_size = record->key_size;
    return CCASK_OK;
}

void ccask_keys_iter_close(ccask_keys_iter_t *iter) {
    ccask_keydir_record_iter_close(&iter->keydir_iter);
    free(iter);
}
