
#include "ccask/records.h"

#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "ccask/utils.h"
#include "ccask/errors.h"

int ccask_allocate_datafile_record(ccask_datafile_record_t record, uint32_t key_size, uint32_t value_size) {
    record[0].iov_len = DATAFILE_RECORD_HEADER_SIZE;
    record[1].iov_len = key_size;
    record[2].iov_len = value_size;

    bool done = true;
    int i = 0;
    for (; i < 3; i++) {
        int retry_num = 0;
        do {
            record[i].iov_base = malloc(record[i].iov_len);
        } while (!record[i].iov_base && retry_num++ <= 5);

        if (!record[i].iov_base) {
            done = false;
            break;
        }
    }

    if (!done) {
        for (; i > 0; i--) {
            free(record[i].iov_base);
        }
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

int ccask_create_datafile_record(
    ccask_datafile_record_t record,
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
) {
    uint32_t crc = calculate_crc32(timestamp, key_size, value_size, key, value);

    int res = ccask_allocate_datafile_record(record, key_size, value_size);
    if (res != CCASK_OK) return res;

    write_be32(record[0].iov_base, crc);
    write_be32(record[0].iov_base + 4, timestamp);
    write_be32(record[0].iov_base + 8, key_size);
    write_be32(record[0].iov_base + 12, value_size);

    memcpy(record[1].iov_base, key, key_size);
    memcpy(record[2].iov_base, value, value_size);
    return CCASK_OK;
}

void free_datafile_record(ccask_datafile_record_t record) {
    if (record[0].iov_base) free(record[0].iov_base);
    if (record[1].iov_base) free(record[1].iov_base);
    if (record[2].iov_base) free(record[2].iov_base);
}

int ccask_allocate_hintfile_record(ccask_datafile_record_t record, uint32_t key_size) {
    record[0].iov_len = HINTFILE_RECORD_HEADER_SIZE;
    record[1].iov_len = key_size;

    bool done = true;
    int i = 0;
    for (; i < 3; i++) {
        int retry_num = 0;
        do {
            record[i].iov_base = malloc(record[i].iov_len);
        } while (!record[i].iov_base && retry_num++ <= 5);

        if (!record[i].iov_base) {
            done = false;
            break;
        }
    }

    if (!done) {
        for (; i > 0; i--) {
            free(record[i].iov_base);
        }
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    return CCASK_OK;
}

int ccask_create_hintfile_record(
    ccask_hintfile_record_t record,
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint64_t record_pos,
    void *key
) {
    int res = ccask_allocate_hintfile_record(record, key_size);
    if (res != CCASK_OK) return res;

    write_be32(record[0].iov_base, timestamp);
    write_be32(record[0].iov_base + 4, key_size);
    write_be32(record[0].iov_base + 8, value_size);
    write_be64(record[0].iov_base + 12, record_pos);

    memcpy(record[1].iov_base, key, key_size);
    return CCASK_OK;
}

void free_hintfile_record(ccask_hintfile_record_t record) {
    if (record[0].iov_base) free(record[0].iov_base);
    if (record[1].iov_base) free(record[1].iov_base);
}
