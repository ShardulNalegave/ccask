/**
 * Copyright (C) 2025  Shardul Nalegave
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

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
