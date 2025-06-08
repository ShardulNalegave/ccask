
#ifndef CCASK_FILES_H
#define CCASK_FILES_H

#include "stdint.h"
#include "stdbool.h"
#include "pthread.h"
#include "uthash.h"

#include "ccask_records.h"

typedef struct ccask_file_t {
    uint64_t id;
    int fd;
    bool active;
    bool has_hint;
    char* datafile_path;

    int readers;
    bool is_invalidator_running;
    time_t last_accessed;

    pthread_mutex_t mutex;

    struct ccask_file_t* next;
    struct ccask_file_t* previous;

    UT_hash_handle hh;
} ccask_file_t;

int ccask_files_init(char* dirpath);
void ccask_files_destroy();

ccask_file_t* ccask_files_get_active_file();
ccask_file_t* ccask_files_get_file(uint64_t id);

uint8_t* ccask_files_read_chunk(uint64_t id, uint64_t pos, uint32_t len);
off_t ccask_files_write_chunk(void* buff, uint32_t len);

#endif