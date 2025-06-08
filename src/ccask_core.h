
#ifndef CCASK_CORE_H
#define CCASK_CORE_H

#include "stdint.h"

typedef struct {
    char* data_dir;
} ccask_state_t;

int ccask_init();
void ccask_shutdown();

int ccask_get(void* key, uint32_t key_size, void** value);
int ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size);
void ccask_delete(void* key, uint32_t key_size);

#endif
