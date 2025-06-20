
#ifndef CCASK_ERRORS_H
#define CCASK_ERRORS_H

#include "stdint.h"

#define CCASK_OK         0
#define CCASK_FAIL      -1
#define CCASK_RETRY     -2

extern uint8_t ccask_errno;

typedef enum ccask_error_t {
    ERR_UNKNOWN = 0,
    ERR_NO_MEMORY = 1,
    ERR_ITERATOR_END = 2,
    ERR_WRITER_RINGBUF_FULL = 3,
    ERR_COULDNT_CREATE_THREAD = 4,
    ERR_INVALID_FILE_ID = 5,
    ERR_UNEXPECTED_EOF = 6
} ccask_error_t;

char* ccask_strerror(uint8_t eno);

#endif
