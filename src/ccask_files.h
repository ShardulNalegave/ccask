
#ifndef CCASK_FILES_H
#define CCASK_FILES_H

#include "stdint.h"
#include "stdbool.h"

typedef enum {
    ERR_LIST_NOT_EMPTY = -1,
    ERR_DIR_NOT_FOUND = -2,
} ccask_files_errors;

typedef struct ccask_file_t {
    uint64_t id;
    int fd;
    bool active;
    bool has_hint;

    struct ccask_file_t* next;
    struct ccask_file_t* previous;
} ccask_file_t;

int ccask_files_load_directory(char* path, ccask_file_t* head);
void ccask_files_destroy(ccask_file_t* head);

#endif