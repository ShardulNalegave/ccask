
#include "ccask_keydir.h"
#include "log.h"

static ccask_keydir_record_t* keydir_hash_table;

void ccask_keydir_recover(ccask_state_t* state) {
    keydir_hash_table = NULL;
    // TODO: Read all existing data files and hint files to recover persisted data
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