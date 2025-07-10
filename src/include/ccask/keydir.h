
#ifndef CCASK_KEYDIR_H
#define CCASK_KEYDIR_H

#include "stdint.h"
#include "uthash.h"

#include "ccask/status.h"

typedef struct ccask_keydir_record {
    void *key;
    uint32_t key_size;

    uint64_t file_id;
    uint64_t record_pos;
    uint32_t value_size;
    uint32_t timestamp;

    UT_hash_handle hh;
} ccask_keydir_record_t;

ccask_status_e ccask_keydir_init(void);
void ccask_keydir_shutdown(void);

ccask_keydir_record_t* ccask_keydir_find(void *key, uint32_t key_size);
ccask_status_e ccask_keydir_delete(void *key, uint32_t key_size);

ccask_status_e ccask_keydir_upsert(
    void *key,
    uint32_t key_size,
    uint64_t file_id,
    uint64_t record_pos,
    uint32_t value_size,
    uint32_t timestamp
);

typedef struct ccask_keydir_record_iter {
    ccask_keydir_record_t *next;
} ccask_keydir_record_iter_t;

ccask_keydir_record_iter_t ccask_keydir_record_iter(void);
ccask_keydir_record_t* ccask_keydir_record_iter_next(ccask_keydir_record_iter_t *iter);
void ccask_keydir_record_iter_close(ccask_keydir_record_iter_t *iter);

#endif
