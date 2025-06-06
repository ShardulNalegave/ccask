
#ifndef CCASK_CORE_H
#define CCASK_CORE_H

#include "stdint.h"
#include "ccask_files.h"

typedef struct {
    char* data_dir;
    ccask_file_t* data_files_head;
} ccask_state_t;

void ccask_init();
void ccask_shutdown();

void* ccask_get(void* key);
void ccask_put(void* key, void* value);
void ccask_delete(void* key);

#endif
