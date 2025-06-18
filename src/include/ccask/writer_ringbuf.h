
#ifndef CCASK_WRITER_RINGBUF_H
#define CCASK_WRITER_RINGBUF_H

#include "stddef.h"
#include "stdbool.h"

typedef struct ccask_writer_ringbuf_t ccask_writer_ringbuf_t;

ccask_writer_ringbuf_t* ccask_writer_ringbuf_create(size_t capacity);
void ccask_writer_ringbuf_destroy(ccask_writer_ringbuf_t *rb);

bool ccask_writer_ringbuf_push(ccask_writer_ringbuf_t *rb, void *record);
void* ccask_writer_ringbuf_pop(ccask_writer_ringbuf_t *rb);

void ccask_writer_ringbuf_shutdown(ccask_writer_ringbuf_t *rb);
size_t ccask_writer_ringbuf_count(const ccask_writer_ringbuf_t *rb);

#endif
