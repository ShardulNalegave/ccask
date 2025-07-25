/**
 * Copyright (C) 2025  Shardul Nalegave
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#ifndef CCASK_FILES_H
#define CCASK_FILES_H

#include "stdint.h"
#include "stdbool.h"
#include "pthread.h"
#include "uthash.h"

#include "ccask/utils.h"
#include "ccask/status.h"

extern size_t MAX_ACTIVE_FILE_SIZE;

typedef struct ccask_file {
    uint64_t file_id;
    int fd;
    time_t last_accessed;
    bool has_hint;
    bool is_active;
    bool is_fd_invalidator_running;
    pthread_rwlock_t rwlock;

    struct ccask_file* next;
    struct ccask_file* previous;
    UT_hash_handle hh;
} ccask_file_t;

ccask_status_e ccask_files_init(const char *data_dir, size_t active_file_max_size);
void ccask_files_shutdown(void);

ccask_file_t* ccask_files_get_active_file(void);
ccask_file_t* ccask_files_get_oldest_file(void);
ccask_file_t* ccask_files_get_file(uint64_t file_id);

int ccask_files_get_active_datafile_fd(uint64_t file_id);
int ccask_files_get_datafile_fd(uint64_t file_id);
int ccask_files_get_hintfile_fd(uint64_t file_id);
int ccask_files_get_temp_datafile_fd(uint64_t file_id);

ccask_status_e ccask_files_rotate(void);

ccask_status_e ccask_files_delete(uint64_t file_id, file_ext_e ext);
ccask_status_e ccask_files_change_ext(uint64_t file_id, file_ext_e from, file_ext_e to);

#endif
