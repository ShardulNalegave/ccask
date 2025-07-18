
#include "ccask/keydir.h"

#include "stdlib.h"
#include "pthread.h"
#include "inttypes.h"
#include "uthash.h"
#include "ccask/files.h"
#include "ccask/iterator.h"
#include "ccask/status.h"
#include "ccask/log.h"

static pthread_rwlock_t hash_table_lock;
static ccask_keydir_record_t *hash_table = NULL;

static ccask_status_e recover_hintfile(ccask_file_t *file) {
    // get iterator for hintfile
    int res;
    ccask_hintfile_iter_t iter;
    CCASK_ATTEMPT(5, res, ccask_hintfile_iter_open(file->file_id, &iter));

    if (res != CCASK_OK) {
        log_error("Hintfile recovery failed (File ID = %" PRIu64 ")", file->file_id);
        return CCASK_FAIL;
    }

    uint64_t record_pos;
    ccask_hintfile_record_t record;
    while (ccask_hintfile_iter_next(&iter, record, &record_pos) == CCASK_OK) {
        ccask_hintfile_record_header_t header = ccask_get_hintfile_record_header(record);
        void *key = ccask_get_hintfile_record_key(record);
        
        int res;
        CCASK_ATTEMPT(5, res, ccask_keydir_upsert(
            key, header.key_size,
            file->file_id,
            header.record_pos,
            header.value_size,
            header.timestamp
        ));

        if (res != CCASK_OK) {
            log_error("Couldn't recover Hintfile ID = %" PRIu64 " record at position = %" PRIu64, file->file_id, record_pos);
        }

        free_hintfile_record(record); // free memory used by current record's buffers
    }

    ccask_hintfile_iter_close(&iter);
    log_info("Recovered Hintfile ID = %" PRIu64, file->file_id);
    return CCASK_OK;
}

static ccask_status_e recover_datafile(ccask_file_t *file) {
    // get iterator for datafile
    int res;
    ccask_datafile_iter_t iter;
    CCASK_ATTEMPT(5, res, ccask_datafile_iter_open(file->file_id, &iter));

    if (res != CCASK_OK) {
        log_error("Datafile recovery failed (File ID = %" PRIu64 ")", file->file_id);
        return CCASK_FAIL;
    }

    uint64_t record_pos;
    ccask_datafile_record_t record;
    while (ccask_datafile_iter_next(&iter, record, &record_pos) == CCASK_OK) {
        ccask_datafile_record_header_t header = ccask_get_datafile_record_header(record);
        void *key = ccask_get_datafile_record_key(record);

        int res;
        CCASK_ATTEMPT(5, res, ccask_keydir_upsert(
            key, header.key_size,
            file->file_id,
            record_pos,
            header.value_size,
            header.timestamp
        ));

        if (res != CCASK_OK) {
            log_error("Couldn't recover Datafile ID = %" PRIu64 " record at position = %" PRIu64, file->file_id, record_pos);
        }

        free_datafile_record(record); // free memory used by current record's buffers
    }

    ccask_datafile_iter_close(&iter);
    log_info("Recovered Datafile ID = %" PRIu64, file->file_id);
    return CCASK_OK;
}

static ccask_status_e keydir_recover(void) {
    ccask_file_t *file = ccask_files_get_oldest_file();
    while (file) {
        // TODO: Return error if partial recovery is disabled and recovery of a file fails
        if (file->has_hint) recover_hintfile(file);
        else recover_datafile(file);
        file = file->previous;
    }
}

ccask_status_e ccask_keydir_init(void) {
    hash_table = NULL;
    pthread_rwlock_init(&hash_table_lock, NULL);
    return keydir_recover();
}

void ccask_keydir_shutdown(void) {
    pthread_rwlock_wrlock(&hash_table_lock);
    ccask_keydir_record_t *entry, *tmp;
    HASH_ITER(hh, hash_table, entry, tmp) {
        HASH_DEL(hash_table, entry);
        free(entry->key);
        free(entry);
    }
    pthread_rwlock_unlock(&hash_table_lock);
    
    hash_table = NULL;
    pthread_rwlock_destroy(&hash_table_lock);
}

ccask_keydir_record_t* ccask_keydir_find(void *key, uint32_t key_size) {
    ccask_keydir_record_t *entry = NULL;
    pthread_rwlock_rdlock(&hash_table_lock);
    HASH_FIND(hh, hash_table, key, key_size, entry);
    pthread_rwlock_unlock(&hash_table_lock);
    return entry;
}

int ccask_keydir_delete(void *key, uint32_t key_size) {
    ccask_keydir_record_t *entry = ccask_keydir_find(key, key_size);
    if (!entry) {
        return CCASK_FAIL;
    }

    pthread_rwlock_wrlock(&hash_table_lock);
    HASH_DEL(hash_table, entry);
    free(entry->key);
    free(entry);
    pthread_rwlock_unlock(&hash_table_lock);

    return CCASK_OK;
}

int ccask_keydir_upsert(
    void *key,
    uint32_t key_size,
    uint64_t file_id,
    uint64_t record_pos,
    uint32_t value_size,
    uint32_t timestamp
) {
    ccask_keydir_record_t *entry = ccask_keydir_find(key, key_size);
    pthread_rwlock_wrlock(&hash_table_lock);

    if (entry) {
        entry->file_id = file_id;
        entry->record_pos = record_pos;
        entry->value_size = value_size;
        entry->timestamp = timestamp;
        pthread_rwlock_unlock(&hash_table_lock);
        return CCASK_OK;
    }

    entry = malloc(sizeof(ccask_keydir_record_t));
    if (!entry) {
        pthread_rwlock_unlock(&hash_table_lock);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_RETRY;
    }

    entry->key_size = key_size;
    entry->key = malloc(key_size);

    if (!entry->key) {
        pthread_rwlock_unlock(&hash_table_lock);
        free(entry);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_RETRY;
    }

    memcpy(entry->key, key, key_size);

    entry->file_id = file_id;
    entry->record_pos = record_pos;
    entry->value_size = value_size;
    entry->timestamp = timestamp;

    HASH_ADD_KEYPTR(hh, hash_table, entry->key, key_size, entry);

    pthread_rwlock_unlock(&hash_table_lock);
    return CCASK_OK;
}

ccask_keydir_record_iter_t ccask_keydir_record_iter(void) {
    pthread_rwlock_rdlock(&hash_table_lock);
    ccask_keydir_record_iter_t iter;
    iter.next = hash_table;
    return iter;
}

ccask_keydir_record_t* ccask_keydir_record_iter_next(ccask_keydir_record_iter_t *iter) {
    if (iter->next == NULL) return NULL;

    ccask_keydir_record_t *record = iter->next;
    iter->next = iter->next->hh.next;
    return record;
}

void ccask_keydir_record_iter_close(ccask_keydir_record_iter_t *iter) {
    iter->next = NULL;
    pthread_rwlock_unlock(&hash_table_lock);
}
