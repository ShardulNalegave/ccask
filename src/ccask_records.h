
#ifndef CCASK_RECORD_TYPES_H
#define CCASK_RECORD_TYPES_H

#include "stdint.h"

typedef struct {
    uint32_t crc; // 32-bit Cyclic Redundancy Check (CRC)
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint8_t* key;
    uint8_t* value;
} ccask_data_file_record_t;

typedef struct {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t value_pos;
    uint8_t* key;
} ccask_hint_file_record_t;

void data_file_record_serialize(ccask_data_file_record_t* record, uint8_t* buffer);
void data_file_record_deserialize(uint8_t* buffer, ccask_data_file_record_t* record);

void hint_file_record_serialize(ccask_hint_file_record_t* record, uint8_t* buffer);
void hint_file_record_deserialize(uint8_t* buffer, ccask_hint_file_record_t* record);

#endif