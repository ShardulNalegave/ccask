
#ifndef CCASK_KEYDIR_H
#define CCASK_KEYDIR_H

#include "stdint.h"
#include "uthash.h"

typedef struct ccask_keydir_record_t {
    void *key;
    uint32_t key_size;

    uint64_t file_id;
    uint64_t record_pos;
    uint32_t value_size;
    uint32_t timestamp;

    UT_hash_handle hh;
} ccask_keydir_record_t;

void ccask_keydir_init(void);
void ccask_keydir_shutdown(void);

ccask_keydir_record_t* ccask_keydir_find(void *key, uint32_t key_size);
int ccask_keydir_delete(void *key, uint32_t key_size);

int ccask_keydir_upsert(
    void *key,
    uint32_t key_size,
    uint64_t file_id,
    uint64_t record_pos,
    uint32_t value_size,
    uint32_t timestamp
);

#endif
