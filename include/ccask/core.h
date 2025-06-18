
#ifndef CCASK_CORE_H
#define CCASK_CORE_H

#include "stdint.h"

typedef struct ccask_options_t {
    char* data_dir;
} ccask_options_t;

int ccask_init(ccask_options_t opts);
int ccask_shutdown(void);

int ccask_flush(void);
int ccask_get(void* key, uint32_t key_size, void** value);
int ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size);
int ccask_delete(void* key, uint32_t key_size);

#endif
