
#ifndef CCASK_RECORDS_ITER_H
#define CCASK_RECORDS_ITER_H

#include "stdint.h"
#include "ccask_records.h"
#include "ccask_files.h"

typedef struct {
    uint64_t file_id;
    uint64_t offset;
    uint8_t* buffer;
    uint64_t buffer_size;
} ccask_datafile_iter_t;

int ccask_datafile_record_iter_open(ccask_file_t* file, ccask_datafile_iter_t** iter);
int ccask_datafile_record_iter_next(ccask_datafile_iter_t* iter, ccask_datafile_record_t** record);
void ccask_datafile_record_iter_close(ccask_datafile_iter_t* iter);

#endif