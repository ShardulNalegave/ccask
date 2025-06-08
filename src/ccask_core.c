
#include "ccask_core.h"

#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "zlib.h"
#include "log.h"

#include "ccask_crc_check.h"
#include "ccask_files.h"
#include "ccask_keydir.h"
#include "ccask_records.h"

static ccask_state_t state;

int ccask_init() {
    state.data_dir = "./test_data"; // TODO: this should be fetched from a config file
    
    if (ccask_files_init(state.data_dir) != 0)
        return -1;

    ccask_keydir_recover(&state);
    return 0;
}

void ccask_shutdown() {
    ccask_files_destroy();
    ccask_keydir_free_all();
}

int ccask_get(void* key, uint32_t key_size, void** value) {
    ccask_keydir_record_t* kd_record = ccask_keydir_find(key, key_size);
    if (!kd_record) return -1;

    uint8_t* record = ccask_files_read_chunk(kd_record->file_id, kd_record->record_pos, 16 + key_size + kd_record->value_size);
    uint32_t stored_crc = ((uint32_t*)record)[0];

    uint32_t crc = ccask_crc_calculate_with_header(
        record + 4,
        key,
        key_size,
        record + 16 + key_size,
        kd_record->value_size
    );

    if (crc != stored_crc) {
        log_error("Stored CRC for key doesn't match its actual CRC, returning NULL\n\t(%d != %d)", stored_crc, crc);
        free(record);
        return -1;
    }

    *value = malloc(kd_record->value_size);
    memcpy(*value, record + 16 + key_size, kd_record->value_size);
    free(record);

    return kd_record->value_size;
}

int ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size) {
    ccask_datafile_record_t* record = malloc(sizeof(ccask_datafile_record_t));

    uint8_t* key_buff = malloc(key_size);
    uint8_t* value_buff = malloc(value_size);

    memcpy(key_buff, key, key_size);
    memcpy(value_buff, value, value_size);

    record->key = key_buff;
    record->value = value_buff;
    record->key_size = key_size;
    record->value_size = value_size;

    record->timestamp = time(NULL);

    uint8_t header[12];
    memcpy(header, &record->timestamp, 4);
    memcpy(header + 4, &record->key_size, 4);
    memcpy(header + 8, &record->value_size, 4);

    record->crc = ccask_crc_calculate_with_datafile_record(record);

    uint8_t* buff;
    uint64_t buff_len = ccask_datafile_record_serialize(&buff, record);

    off_t record_pos = ccask_files_write_chunk(buff, buff_len);
    if(record_pos < 0) {
        return -1;
    }

    ccask_keydir_upsert(
        key,
        key_size,
        ccask_files_get_active_datafile()->id,
        record_pos,
        value_size,
        record->timestamp
    );

    free(record->key);
    free(record->value);
    free(record);

    return 0;
}

void ccask_delete(void* key, uint32_t key_size) {
    ccask_put(key, key_size, NULL, 0); // write a tombstone record
    ccask_keydir_delete(key, key_size);
}