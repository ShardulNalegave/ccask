
#include "ccask/records.h"

#include "stdlib.h"
#include "string.h"
#include "ccask/utils.h"

void ccask_create_datafile_record(
    ccask_datafile_record_t record,
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
) {
    uint32_t crc = calculate_crc32(timestamp, key_size, value_size, key, value);

    record[0].iov_base = malloc(12);
    record[0].iov_len = 12;

    record[1].iov_base = malloc(key_size);
    record[1].iov_len = key_size;

    record[2].iov_base = malloc(value_size);
    record[2].iov_len = value_size;

    write_be32(record[0].iov_base, timestamp);
    write_be32(record[0].iov_base + 4, key_size);
    write_be32(record[0].iov_base + 8, value_size);

    memcpy(record[1].iov_base, key, key_size);
    memcpy(record[2].iov_base, value, value_size);
}

void ccask_create_hintfile_record(
    ccask_hintfile_record_t record,
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint64_t record_pos,
    void *key
) {
    record[0].iov_base = malloc(20);
    record[0].iov_len = 20;

    record[1].iov_base = malloc(key_size);
    record[1].iov_len = key_size;

    write_be32(record[0].iov_base, timestamp);
    write_be32(record[0].iov_base + 4, key_size);
    write_be32(record[0].iov_base + 8, value_size);
    write_be64(record[0].iov_base + 12, record_pos);

    memcpy(record[1].iov_base, key, key_size);
}