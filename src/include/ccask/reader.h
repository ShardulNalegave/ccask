
#ifndef CCASK_READER_H
#define CCASK_READER_H

#include "ccask/files.h"
#include "ccask/records.h"

int ccask_read_datafile_record(ccask_file_t *file, ccask_datafile_record_t record, uint64_t record_pos);

#endif
