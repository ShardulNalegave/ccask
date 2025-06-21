
#include "ccask/core.h"

#include "time.h"
#include "stdatomic.h"
#include "inttypes.h"
#include "ccask/files.h"
#include "ccask/keydir.h"
#include "ccask/writer.h"
#include "ccask/writer_ringbuf.h"
#include "ccask/reader.h"
#include "ccask/hint.h"
#include "ccask/errors.h"
#include "ccask/log.h"
#include "ccask/utils.h"

static _Atomic bool is_shutting_down = false;

int ccask_init(ccask_options_t opts) {
    atomic_store(&is_shutting_down, false);

    int res; int retry_counter = 0;
    do {
        res = ccask_files_init(opts.data_dir, opts.active_file_max_size);
    } while (res == CCASK_RETRY && retry_counter++ <= 5);

    if (res != CCASK_OK) {
        log_fatal("Couldn't initialize ccask-files");
        return CCASK_FAIL;
    }
    
    ccask_keydir_init();
    
    if (ccask_writer_start(opts.writer_ringbuf_capacity) != CCASK_OK) {
        log_fatal("Couldn't initialize writer");
        return CCASK_FAIL;
    }

    ccask_hintfile_generator_init();
    return CCASK_OK;
}

int ccask_shutdown(void) {
    atomic_store(&is_shutting_down, true);
    ccask_hintfile_generator_shutdown();
    ccask_writer_stop();
    ccask_keydir_shutdown();
    ccask_files_shutdown();
}

int ccask_get(void* key, uint32_t key_size, void** value) {
    ccask_keydir_record_t *kd_record = ccask_keydir_find(key, key_size);
    if (kd_record == NULL) {
        *value = NULL;
        return CCASK_OK;
    }

    ccask_datafile_record_t record;
    ccask_allocate_datafile_record(record, kd_record->key_size, kd_record->value_size);
    int res = ccask_read_datafile_record(kd_record->file_id, record, kd_record->record_pos);
    if (res != CCASK_OK) {
        log_error("Failed to read datafile record");
        *value = NULL;
        return CCASK_FAIL;
    }

    ccask_datafile_record_header_t header = ccask_get_datafile_record_header(record);
    void *read_key = ccask_get_datafile_record_key(record);
    void *read_value = ccask_get_datafile_record_value(record);

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
        free_datafile_record(record);
        return CCASK_FAIL;
    }

    *value = malloc(kd_record->value_size);
    memcpy(*value, read_value, kd_record->value_size);
    free_datafile_record(record);

    return kd_record->value_size;
}

int ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size) {
    if (atomic_load(&is_shutting_down)) {
        log_error("Cannot put values after shutdown has been initiated");
        return CCASK_FAIL;
    }

    int res; int retry_counter = 0;
    do {
        res = ccask_writer_ringbuf_push(time(NULL), key, key_size, value, value_size);
    } while (res == CCASK_RETRY && retry_counter++ <= 5);

    if (res != CCASK_OK) {
        log_error("Couldn't put record into writer ringbuf");
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

int ccask_put_blocking(void* key, uint32_t key_size, void* value, uint32_t value_size) {
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

int ccask_delete(void* key, uint32_t key_size) {
    return ccask_put(key, key_size, NULL, 0);
}

int ccask_delete_blocking(void* key, uint32_t key_size) {
    return ccask_put_blocking(key, key_size, NULL, 0);
}
