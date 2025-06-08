
#ifndef CCASK_CRC_CHECK_H
#define CCASK_CRC_CHECK_H

#include "stdint.h"
#include "ccask_records.h"

uint32_t ccask_crc_calculate_with_header(uint8_t* header, uint8_t* key, uint32_t key_size, uint8_t* value, uint32_t value_size);
uint32_t ccask_crc_calculate_with_datafile_record(ccask_datafile_record_t* record);

uint32_t ccask_crc_calculate_with_values(
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    uint8_t* key,
    uint8_t* value
);

#endif