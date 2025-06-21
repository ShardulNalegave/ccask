
#ifndef CCASK_RECORDS_H
#define CCASK_RECORDS_H

#include "stdint.h"
#include "sys/uio.h"

#define DATAFILE_RECORD_HEADER_SIZE 16
#define HINTFILE_RECORD_HEADER_SIZE 20

typedef struct __attribute__((__packed__)) ccask_datafile_record_header_t {
    uint32_t crc; // 32-bit Cyclic Redundancy Check (CRC)
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
} ccask_datafile_record_header_t;

typedef struct iovec ccask_datafile_record_t[3];

int ccask_allocate_datafile_record(ccask_datafile_record_t record, uint32_t key_size, uint32_t value_size);

int ccask_create_datafile_record(
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

typedef struct __attribute__((__packed__)) ccask_hintfile_record_header_t {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t record_pos;
} ccask_hintfile_record_header_t;

typedef struct iovec ccask_hintfile_record_t[2];

int ccask_allocate_hintfile_record(ccask_datafile_record_t record, uint32_t key_size);

int ccask_create_hintfile_record(
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
