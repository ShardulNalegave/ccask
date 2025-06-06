
#ifndef CCASK_CORE_H
#define CCASK_CORE_H

typedef struct {
    char* data_dir;
    int active_fd;
} ccask_state_t;

void ccask_init();
void ccask_shutdown();

void* ccask_get(void* key);
void ccask_put(void* key, void* value);
void ccask_delete(void* key);

#endif
