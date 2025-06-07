
#include "ccask_records.h"

#include "stdlib.h"
#include "string.h"

int datafile_record_serialize(uint8_t* buffer, ccask_datafile_record_t* record) {
    int record_size = 16 + record->key_size + record->value_size;
    buffer = malloc(record_size);

    memcpy(buffer, &record->crc, 4);
    memcpy(&buffer[4], &record->timestamp, 4);
    memcpy(&buffer[8], &record->key_size, 4);
    memcpy(&buffer[12], &record->value_size, 4);

    memcpy(&buffer[16], record->key, record->key_size);
    memcpy(&buffer[16 + record->key_size], record->value, record->value_size);

    return record_size;
}

ccask_datafile_record_t* datafile_record_deserialize(uint8_t* buffer) {
    ccask_datafile_record_t* record = malloc(sizeof(ccask_datafile_record_t));

    record->crc = ((uint32_t*)buffer)[0];
    record->timestamp = ((uint32_t*)buffer)[1];
    record->key_size = ((uint32_t*)buffer)[2];
    record->value_size = ((uint32_t*)buffer)[3];

    record->key = malloc(record->key_size);
    record->value = malloc(record->value_size);

    memcpy(record->key, &buffer[16], record->key_size);
    memcpy(record->value, &buffer[16 + record->key_size], record->value_size);

    return record;
}

int hintfile_record_serialize(uint8_t* buffer, ccask_hintfile_record_t* record) {
    int record_size = 20 + record->key_size;
    buffer = malloc(record_size);

    memcpy(buffer, &record->timestamp, 4);
    memcpy(&buffer[4], &record->key_size, 4);
    memcpy(&buffer[8], &record->value_size, 4);
    memcpy(&buffer[12], &record->value_pos, 8);

    memcpy(&buffer[20], record->key, record->key_size);

    return record_size;
}

ccask_hintfile_record_t* hintfile_record_deserialize(uint8_t* buffer) {
    ccask_hintfile_record_t* record = malloc(sizeof(ccask_hintfile_record_t));

    record->timestamp = ((uint32_t*)buffer)[0];
    record->key_size = ((uint32_t*)buffer)[1];
    record->value_size = ((uint32_t*)buffer)[2];
    record->value_pos = ((uint64_t*)(&buffer[12]))[0];

    record->key = malloc(record->key_size);
    memcpy(record->key, &buffer[20], record->key_size);

    return record;
}