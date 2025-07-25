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

#ifndef CCASK_RECORDS_H
#define CCASK_RECORDS_H

#include "stdint.h"
#include "sys/uio.h"

#include "ccask/status.h"

#define DATAFILE_RECORD_HEADER_SIZE 16
#define HINTFILE_RECORD_HEADER_SIZE 20

typedef struct ccask_datafile_record_header {
    uint32_t crc; // 32-bit Cyclic Redundancy Check (CRC)
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
} ccask_datafile_record_header_t;

typedef struct iovec ccask_datafile_record_t[3];

ccask_status_e ccask_allocate_datafile_record(ccask_datafile_record_t record, uint32_t key_size, uint32_t value_size);

ccask_status_e ccask_create_datafile_record(
    ccask_datafile_record_t record,
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
);

void free_datafile_record(ccask_datafile_record_t record);
ccask_datafile_record_header_t ccask_get_datafile_record_header(ccask_datafile_record_t record);

static inline void* ccask_get_datafile_record_key(ccask_datafile_record_t record) {
    return record[1].iov_base;
}

static inline void* ccask_get_datafile_record_value(ccask_datafile_record_t record) {
    return record[2].iov_base;
}

static inline size_t ccask_get_datafile_record_total_size(ccask_datafile_record_t record) {
    return record[0].iov_len + record[1].iov_len + record[2].iov_len;
}

typedef struct ccask_hintfile_record_header {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t record_pos;
} ccask_hintfile_record_header_t;

typedef struct iovec ccask_hintfile_record_t[2];

ccask_status_e ccask_allocate_hintfile_record(ccask_datafile_record_t record, uint32_t key_size);

ccask_status_e ccask_create_hintfile_record(
    ccask_hintfile_record_t record,
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint64_t record_pos,
    void *key
);

void free_hintfile_record(ccask_hintfile_record_t record);
ccask_hintfile_record_header_t ccask_get_hintfile_record_header(ccask_hintfile_record_t record);

static inline void* ccask_get_hintfile_record_key(ccask_hintfile_record_t record) {
    return record[1].iov_base;
}

static inline size_t ccask_get_hintfile_record_total_size(ccask_hintfile_record_t record) {
    return record[0].iov_len + record[1].iov_len;
}

#endif
