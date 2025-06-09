
#include "ccask_keydir.h"

#include "log.h"
#include "ccask_files.h"
#include "ccask_records.h"
#include "ccask_records_iter.h"
#include "ccask_crc_check.h"

static ccask_keydir_record_t* keydir_hash_table;

int keydir_recover_hintfile(ccask_file_t* file) {
    ccask_hintfile_iter_t* iter;
    if (ccask_hintfile_record_iter_open(file, &iter) < 0) {
        log_error("Couldn't read hintfile ID=%d while recovery", file->id);
        return -1;
    }

    int num_recovered = 0;
    ccask_hintfile_record_t* record;

    ccask_hintfile_record_iter_next(iter, &record);

    while (record) {
        if (ccask_keydir_upsert(
            record->key,
            record->key_size,
            file->id,
            record->record_pos,
            record->value_size,
            record->timestamp
        ) < 0) {
            log_error("Error while recovering a record from datafile ID=%d", file->id);
        }

        num_recovered++;
        free(record);
        ccask_hintfile_record_iter_next(iter, &record);
    }

    log_info("Hintfile ID=%d recovered (%d records)", file->id, num_recovered);
    ccask_hintfile_record_iter_close(iter);
    return 0;
}

int keydir_recover_datafile(ccask_file_t* file) {
    ccask_datafile_iter_t* iter;
    if (ccask_datafile_record_iter_open(file, &iter) < 0) {
        log_error("Couldn't read datafile ID=%d while recovery", file->id);
        return -1;
    }

    int num_recovered = 0;
    ccask_datafile_record_t* record;

    uint64_t record_pos = iter->offset;
    ccask_datafile_record_iter_next(iter, &record);

    while (record) {
        uint32_t crc = ccask_crc_calculate_with_datafile_record(record);
        if (crc != record->crc) {
            log_error("CRC check failed for record from datafile ID=%s while recovery", file->id);
        } else if (ccask_keydir_upsert(
            record->key,
            record->key_size,
            file->id,
            record_pos,
            record->value_size,
            record->timestamp
        ) < 0) {
            log_error("Error while recovering a record from datafile ID=%d", file->id);
        } else {
            num_recovered++;
        }
        
        free(record);
        uint64_t record_pos = iter->offset;
        ccask_datafile_record_iter_next(iter, &record);
    }

    log_info("Datafile ID=%d recovered (%d records)", file->id, num_recovered);
    ccask_datafile_record_iter_close(iter);
    return 0;
}

void ccask_keydir_recover(ccask_state_t* state) {
    keydir_hash_table = NULL;
    ccask_file_t* file = ccask_files_get_oldest_datafile();
    while (file) {
        if (file->hintfile_path != NULL) {
            keydir_recover_hintfile(file);
        } else {
            keydir_recover_datafile(file);
        }
        file = file->previous;
    }

    log_info("Key-Directory recovery complete");
}

void ccask_keydir_free_all() {
    ccask_keydir_record_t *entry, *tmp;
    HASH_ITER(hh, keydir_hash_table, entry, tmp) {
        HASH_DEL(keydir_hash_table, entry);
        free(entry->key);
        free(entry);
    }
    keydir_hash_table = NULL;
}

ccask_keydir_record_t* ccask_keydir_find(uint8_t* key, uint32_t key_size) {
    ccask_keydir_record_t* entry = NULL;
    HASH_FIND(hh, keydir_hash_table, key, key_size, entry);
    return entry;
}

int ccask_keydir_delete(uint8_t* key, uint32_t key_size) {
    ccask_keydir_record_t* entry = NULL;
    HASH_FIND(hh, keydir_hash_table, key, key_size, entry);
    if (!entry) return -1;
    HASH_DEL(keydir_hash_table, entry);
    free(entry->key);
    free(entry);
    return 0;
}

int ccask_keydir_upsert(
    uint8_t* key,
    uint32_t key_size,
    uint64_t file_id,
    uint64_t record_pos,
    uint32_t value_size,
    uint32_t timestamp
) {
    ccask_keydir_record_t* entry = NULL;
    HASH_FIND(hh, keydir_hash_table, key, key_size, entry);

    if (entry) {
        entry->file_id = file_id;
        entry->record_pos = record_pos;
        entry->value_size = value_size;
        entry->timestamp = timestamp;
        return 0;
    }

    entry = malloc(sizeof(ccask_keydir_record_t));
    if (!entry) return -1;

    entry->key_size = key_size;
    entry->key = malloc(key_size);

    if (!entry->key) {
        free(entry);
        return -1;
    }

    memcpy(entry->key, key, key_size);

    entry->file_id = file_id;
    entry->record_pos = record_pos;
    entry->value_size = value_size;
    entry->timestamp = timestamp;

    HASH_ADD_KEYPTR(hh, keydir_hash_table, key, key_size, entry);
    return 0;
}