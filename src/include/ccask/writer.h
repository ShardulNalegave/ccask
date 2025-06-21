
#ifndef CCASK_WRITER_H
#define CCASK_WRITER_H

#include "ccask/records.h"

int ccask_writer_start(size_t capacity);
void ccask_writer_stop(void);
int ccask_write_record_blocking(ccask_datafile_record_t record);

#endif
