
#ifndef CCASK_WRITER_H
#define CCASK_WRITER_H

#include "ccask/records.h"
#include "ccask/status.h"

ccask_status_e ccask_writer_start(size_t capacity);
void ccask_writer_stop(void);
ccask_status_e ccask_write_record_blocking(ccask_datafile_record_t record);

#endif
