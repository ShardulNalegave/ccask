
#ifndef CCASK_KEYDIR_H
#define CCASK_KEYDIR_H

#include "stdint.h"
#include "uthash.h"

#include "ccask_core.h"

typedef struct {
    uint8_t* key;
    uint32_t key_size;

    uint64_t file_id;
    uint64_t record_pos;
    uint32_t value_size;
    uint32_t timestamp;

    UT_hash_handle hh;
} ccask_keydir_record_t;

void ccask_keydir_recover(ccask_state_t* state);
void ccask_keydir_free_all();

ccask_keydir_record_t* ccask_keydir_find(uint8_t* key, uint32_t key_size);
int ccask_keydir_delete(uint8_t* key, uint32_t key_size);

int ccask_keydir_upsert(
    uint8_t* key,
    uint32_t key_size,
    uint64_t file_id,
    uint64_t record_pos,
    uint32_t value_size,
    uint32_t timestamp
);

#endif