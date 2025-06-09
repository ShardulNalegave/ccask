
#ifndef CCASK_FILES_H
#define CCASK_FILES_H

#include "stdint.h"
#include "stdbool.h"
#include "fcntl.h"
#include "pthread.h"
#include "uthash.h"

typedef struct ccask_file_t {
    uint64_t id;
    int fd;
    bool active;

    char* datafile_path;
    char* hintfile_path;

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

ccask_file_t* ccask_files_get_active_datafile();
ccask_file_t* ccask_files_get_oldest_datafile();
ccask_file_t* ccask_files_get_file(uint64_t id);

uint8_t* ccask_files_read_chunk(uint64_t id, uint64_t pos, uint32_t len);
off_t ccask_files_write_chunk(void* buff, uint32_t len);

int ccask_files_read_entire_datafile(ccask_file_t* file, uint8_t** buffer);
int ccask_files_read_entire_hintfile(ccask_file_t* file, uint8_t** buffer);

char* ccask_get_hintfile_path(uint64_t id);
int ccask_get_hintfile_fd(char* fpath);
off_t ccask_files_write_chunk_to_fd(int fd, void* buff, uint32_t len);

#endif