
#ifndef CCASK_FILES_H
#define CCASK_FILES_H

#include "stdint.h"
#include "stdbool.h"
#include "pthread.h"
#include "uthash.h"

#define MAX_ACTIVE_FILE_SIZE 50 // bytes

typedef struct ccask_file_t {
    uint64_t file_id;
    int fd;
    time_t last_accessed;
    bool has_hint;
    bool is_active;
    bool is_fd_invalidator_running;
    pthread_rwlock_t rwlock;

    struct ccask_file_t* next;
    struct ccask_file_t* previous;
    UT_hash_handle hh;
} ccask_file_t;

int ccask_files_init(const char *data_dir);
void ccask_files_shutdown(void);

ccask_file_t* ccask_files_get_active_file(void);
ccask_file_t* ccask_files_get_oldest_file(void);
ccask_file_t* ccask_files_get_file(uint64_t file_id);

int ccask_files_get_active_datafile_fd(uint64_t file_id);
int ccask_files_get_datafile_fd(uint64_t file_id);
int ccask_files_get_hintfile_fd(uint64_t file_id);

int ccask_files_rotate(void);

int ccask_files_delete_hintfile(uint64_t file_id);

#endif
