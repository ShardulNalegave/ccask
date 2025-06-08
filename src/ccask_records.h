
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
} ccask_datafile_record_t;

typedef struct {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t record_pos;
    uint8_t* key;
} ccask_hintfile_record_t;

uint64_t ccask_datafile_record_serialize(uint8_t** buffer, ccask_datafile_record_t* record);
uint64_t ccask_datafile_record_deserialize(uint8_t* buffer, ccask_datafile_record_t** record);

uint64_t ccask_hintfile_record_serialize(uint8_t** buffer, ccask_hintfile_record_t* record);
uint64_t ccask_hintfile_record_deserialize(uint8_t* buffer, ccask_hintfile_record_t** record);

#endif