
#ifndef CCASK_RECORDS_H
#define CCASK_RECORDS_H

#include "stdint.h"
#include "sys/uio.h"

typedef struct __attribute__((__packed__)) ccask_datafile_record_header_t {
    uint32_t crc; // 32-bit Cyclic Redundancy Check (CRC)
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
} ccask_datafile_record_header_t;

typedef struct iovec ccask_datafile_record_t[3];

void ccask_create_datafile_record(
    ccask_datafile_record_t record,
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
);

static inline ccask_datafile_record_header_t* ccask_get_datafile_record_header(ccask_datafile_record_t record) {
    return (ccask_datafile_record_header_t*)record[0].iov_base;
}

static inline void* ccask_get_datafile_record_key(ccask_datafile_record_t record) {
    return record[1].iov_base;
}

static inline void* ccask_get_datafile_record_value(ccask_datafile_record_t record) {
    return record[2].iov_base;
}

typedef struct __attribute__((__packed__)) ccask_hintfile_record_header_t {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t record_pos;
} ccask_hintfile_record_header_t;

typedef struct iovec ccask_hintfile_record_t[2];

void ccask_create_hintfile_record(
    ccask_hintfile_record_t record,
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint64_t record_pos,
    void *key
);

static inline ccask_hintfile_record_header_t* ccask_get_hintfile_record_header(ccask_hintfile_record_t record) {
    return (ccask_hintfile_record_header_t*)record[0].iov_base;
}

static inline void* ccask_get_hintfile_record_key(ccask_hintfile_record_t record) {
    return record[1].iov_base;
}

#endif
