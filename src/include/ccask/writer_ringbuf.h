
#ifndef CCASK_WRITER_RINGBUF_H
#define CCASK_WRITER_RINGBUF_H

#include "stddef.h"
#include "stdbool.h"
#include "ccask/records.h"

ccask_status_e ccask_writer_ringbuf_init(size_t capacity);
void ccask_writer_ringbuf_start_shutdown(void);
void ccask_writer_ringbuf_destroy(void);

size_t ccask_writer_ringbuf_count(void);

ccask_status_e ccask_writer_ringbuf_push(
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
);

ccask_status_e ccask_writer_ringbuf_pop(ccask_datafile_record_t record);

#endif
