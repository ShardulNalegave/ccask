
#include "ccask_crc_check.h"
#include "zlib.h"
#include "string.h"

uint32_t ccask_crc_calculate_with_header(uint8_t* header, uint8_t* key, uint32_t key_size, uint8_t* value, uint32_t value_size) {
    uint32_t crc = (uint32_t)crc32(0L, header, 12);
    crc = (uint32_t)crc32(crc, key, key_size);
    crc = (uint32_t)crc32(crc, value, value_size);

    return crc;
}

uint32_t ccask_crc_calculate_with_values(
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint8_t* key,
    uint8_t* value
) {
    uint8_t header[12];
    memcpy(header, &timestamp, 4);
    memcpy(header + 4, &key_size, 4);
    memcpy(header + 8, &value_size, 4);

    return ccask_crc_calculate_with_header(header, key, key_size, value, value_size);
}

uint32_t ccask_crc_calculate_with_datafile_record(ccask_datafile_record_t* record) {
    return ccask_crc_calculate_with_values(
        record->timestamp,
        record->key_size,
        record->value_size,
        record->key,
        record->value
    );
}