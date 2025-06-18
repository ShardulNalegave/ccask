
#ifndef CCASK_WRITER_H
#define CCASK_WRITER_H

#include "sys/uio.h"
#include "stdint.h"

int ccasK_writer_start(const char *data_dir, size_t rb_capacity);
void ccask_writer_stop(void);

#endif
