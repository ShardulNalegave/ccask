
#ifndef CCASK_ITERATOR_H
#define CCASK_ITERATOR_H

#include "stdint.h"
#include "ccask/records.h"

typedef struct ccask_datafile_iter_t {
    uint64_t file_id;
    int fd;
    uint64_t offset;
    uint64_t total_size;
} ccask_datafile_iter_t;

int ccask_datafile_iter_open(uint64_t file_id, ccask_datafile_iter_t *iter);
uint64_t ccask_datafile_iter_next(ccask_datafile_iter_t *iter, ccask_datafile_record_t record);
void ccask_datafile_iter_close(ccask_datafile_iter_t *iter);

typedef struct ccask_hintfile_iter_t {
    uint64_t file_id;
    int fd;
    uint64_t offset;
    uint64_t total_size;
} ccask_hintfile_iter_t;

int ccask_hintfile_iter_open(uint64_t file_id, ccask_hintfile_iter_t *iter);
uint64_t ccask_hintfile_iter_next(ccask_hintfile_iter_t *iter, ccask_hintfile_record_t record);
void ccask_hintfile_iter_close(ccask_hintfile_iter_t *iter);

#endif
