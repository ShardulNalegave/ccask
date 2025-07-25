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

#include "ccask/files.h"

#include "stdio.h"
#include "time.h"
#include "unistd.h"
#include "fcntl.h"
#include "dirent.h"
#include "sys/stat.h"
#include "errno.h"
#include "inttypes.h"
#include "pthread.h"
#include "uthash.h"
#include "ccask/hint.h"
#include "ccask/utils.h"
#include "ccask/log.h"

size_t MAX_ACTIVE_FILE_SIZE = 50; // TODO: set this to a value that makes sense

static const int DATAFILE_OPEN_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
static const int DATAFILE_OPEN_FLAGS = O_RDONLY;
static const int HINTFILE_OPEN_FLAGS = O_CREAT | O_RDWR | O_APPEND;
static const int ACTIVE_DATAFILE_OPEN_FLAGS = O_CREAT | O_RDWR | O_APPEND;
static const int TEMP_DATAFILE_OPEN_FLAGS = O_CREAT | O_RDWR | O_APPEND;

static struct files_state {
    char* data_dir;
    ccask_file_t *hash_table;
    ccask_file_t *head;
    ccask_file_t *tail;
} files_state;

static void add_file_to_linked_list(ccask_file_t* file) {
    ccask_file_t *curr = files_state.head;

    if (!curr) {
        files_state.head = files_state.tail = file;
        file->next = file->previous = NULL;
        return;
    }

    if (file->file_id > curr->file_id) {
        file->next = curr;
        file->previous = NULL;
        curr->previous = file;
        files_state.head = file;
        return;
    }

    while (curr && curr->file_id > file->file_id) {
        curr = curr->next;
    }

    if (!curr) {
        ccask_file_t* tail = files_state.tail;
        tail->next = file;
        file->previous = tail;
        file->next = NULL;
        files_state.tail = file;
        return;
    }

    file->next = curr;
    file->previous = curr->previous;

    if (curr->previous)
        curr->previous->next = file;

    curr->previous = file;
}

static void add_file(ccask_file_t* file) {
    add_file_to_linked_list(file);
    HASH_ADD(hh, files_state.hash_table, file_id, sizeof(uint64_t), file);
}

inline ccask_file_t* ccask_files_get_active_file(void) {
    return files_state.head;
}

inline ccask_file_t* ccask_files_get_oldest_file(void) {
    return files_state.tail;
}

inline ccask_file_t* ccask_files_get_file(uint64_t file_id) {
    ccask_file_t* file = NULL;
    HASH_FIND(hh, files_state.hash_table, &file_id, sizeof(uint64_t), file);
    return file;
}

inline int ccask_files_get_active_datafile_fd(uint64_t id) {
    char* fpath = build_filepath(files_state.data_dir, id, FILE_DATA);
    int fd = open(fpath, ACTIVE_DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    if (fd >= 0) return fd;

    switch (errno) {
        case EACCES:
        case EPERM:
        case EISDIR:
        case ENAMETOOLONG:
            ccask_errno = CCASK_ERR_GET_FD_FAILED;
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

inline int ccask_files_get_datafile_fd(uint64_t file_id) {
    char* fpath = build_filepath(files_state.data_dir, file_id, FILE_DATA);
    int fd = open(fpath, DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    if (fd >= 0) return fd;

    switch (errno) {
        case EACCES:
        case EPERM:
        case EISDIR:
        case ENAMETOOLONG:
        case ENOENT:
            ccask_errno = CCASK_ERR_GET_FD_FAILED;
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

inline int ccask_files_get_hintfile_fd(uint64_t file_id) {
    char* fpath = build_filepath(files_state.data_dir, file_id, FILE_HINT);
    int fd = open(fpath, HINTFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    if (fd >= 0) return fd;

    switch (errno) {
        case EACCES:
        case EPERM:
        case EISDIR:
        case ENAMETOOLONG:
        case ENOENT:
            ccask_errno = CCASK_ERR_GET_FD_FAILED;
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

inline int ccask_files_get_temp_datafile_fd(uint64_t file_id) {
    char* fpath = build_filepath(files_state.data_dir, file_id, FILE_TEMP_DATA);
    int fd = open(fpath, TEMP_DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    if (fd >= 0) return fd;

    switch (errno) {
        case EACCES:
        case EPERM:
        case EISDIR:
        case ENAMETOOLONG:
        case ENOENT:
            ccask_errno = CCASK_ERR_GET_FD_FAILED;
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

static ccask_status_e create_new_active_datafile(uint64_t id, ccask_file_t *file) {
    int fd; int retry_counter = 0;
    CCASK_ATTEMPT(5, fd, ccask_files_get_active_datafile_fd(id));

    if (fd < 0) {
        log_error("Could not create a new Active Datafile ID = %" PRIu64, id);
        return CCASK_FAIL;
    }

    file->file_id = id;
    file->fd = fd;
    file->has_hint = false;
    file->is_active = true;
    file->is_fd_invalidator_running = false;
    file->last_accessed = time(NULL);

    file->next = NULL;
    file->previous = NULL;

    pthread_rwlock_init(&file->rwlock, NULL);

    log_info("Created new Active DataFile ID = %" PRIu64, id);
    return CCASK_OK;
}

static inline ccask_file_t* allocate_datafile_node(uint64_t id, bool has_hint) {
    ccask_file_t* file = malloc(sizeof(ccask_file_t));
    if (!file) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return NULL;
    }

    file->file_id = id;
    file->fd = -1;
    file->has_hint = has_hint;
    file->is_active = false;
    file->is_fd_invalidator_running = false;
    file->last_accessed = time(NULL);
    file->next = NULL;
    file->previous = NULL;

    pthread_rwlock_init(&file->rwlock, NULL);

    return file;
}

int ccask_files_init(const char *data_dir, size_t active_file_max_size) {
    MAX_ACTIVE_FILE_SIZE = active_file_max_size;
    DIR* dir = opendir(data_dir);
    if (!dir) {
        log_fatal("Error while opening data-directory\n\t%s", strerror(errno));
        switch (errno) {
            case EACCES:
            case ENOTDIR:
                return CCASK_FAIL;
            default:
                return CCASK_RETRY;
        }
    }

    size_t dir_len = strlen(data_dir);
    bool has_trailing_slash = (data_dir[dir_len - 1] == '/');

    // Directory length, slash if needed, Max 256 for filename, null terminator
    size_t total = dir_len + (has_trailing_slash ? 0 : 1) + 256 + 1;

    char *entry_path = malloc(total);
    if (!entry_path) {
        log_error("Failed to initialize ccask-files (No memory to allocate fpath for traversing)");
        closedir(dir);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_RETRY;
    }

    strcpy(entry_path, data_dir);
    if (!has_trailing_slash) {
        entry_path[dir_len] = '/';
        dir_len++;
    }

    files_state.data_dir = strdup(data_dir);
    files_state.head = files_state.tail = files_state.hash_table = NULL;

    struct dirent *entry = NULL;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        strcpy(&entry_path[dir_len], entry->d_name);

        struct stat entry_stat;
        if (stat(entry_path, &entry_stat) < 0) {
            log_error("Could not get stats for entry: %s\n\t%s", entry->d_name, strerror(errno));
            continue;
        }

        if (S_ISREG(entry_stat.st_mode)) {
            uint64_t file_id;
            file_ext_e ext = parse_filename(entry->d_name, &file_id);

            if (ext == FILE_DATA) {
                bool has_hint = access(build_filepath(files_state.data_dir, file_id, FILE_HINT), F_OK) == 0;
                log_info("Found Data File (ID=%d) %s", file_id, has_hint ? ":: Has Hints" : "");
                ccask_file_t* file = allocate_datafile_node(file_id, has_hint);
                add_file(file);
            }
        }
    }

    free(entry_path);
    closedir(dir);

    if (!files_state.head) {
        ccask_file_t *active_file = malloc(sizeof(ccask_file_t));
        if (!active_file) {
            ccask_errno = CCASK_ERR_NO_MEMORY;
            return CCASK_FAIL;
        }
        if (create_new_active_datafile(0, active_file) != CCASK_OK) {
            log_error("Failed to initialize ccask-files (Unable to create active datafile)");
            return CCASK_FAIL;
        }
        add_file(active_file);
    } else if (files_state.head->has_hint) {
        ccask_file_t *active_file = malloc(sizeof(ccask_file_t));
        if (!active_file) {
            ccask_errno = CCASK_ERR_NO_MEMORY;
            return CCASK_FAIL;
        }
        if (create_new_active_datafile(files_state.head->file_id + 1, active_file) != CCASK_OK) {
            log_error("Failed to initialize ccask-files (Unable to create active datafile)");
            return CCASK_FAIL;
        }
        add_file(active_file);
    } else {
        int fd;
        CCASK_ATTEMPT(5, fd, ccask_files_get_active_datafile_fd(files_state.head->file_id));
        if (fd < 0) {
            log_error("Could not load FD for Active Datafile ID = %" PRIu64, files_state.head->file_id);
            return CCASK_FAIL;
        }

        files_state.head->is_active = true;
        files_state.head->fd = fd;
    }

    log_info("Using Data-File ID=%" PRIu64 " as Active Data-File", files_state.head->file_id);
    return CCASK_OK;
}

void ccask_files_shutdown(void) {
    free(files_state.data_dir);

    ccask_file_t *curr, *tmp;
    HASH_ITER(hh, files_state.hash_table, curr, tmp) {
        pthread_rwlock_wrlock(&curr->rwlock);
        if (curr->fd >= 0) {
            close(curr->fd);
            curr->fd = -1;
        }
        pthread_rwlock_unlock(&curr->rwlock);
        pthread_rwlock_destroy(&curr->rwlock);

        // Unlink from linked list
        if (curr->previous) {
            curr->previous->next = curr->next;
        } else {
            files_state.head = curr->next;
        }

        if (curr->next) {
            curr->next->previous = curr->previous;
        } else {
            files_state.tail = curr->previous;
        }

        HASH_DEL(files_state.hash_table, curr);
        free(curr);
    }
}

int ccask_files_delete(uint64_t file_id, file_ext_e ext) {
    char* fpath = build_filepath(files_state.data_dir, file_id, ext);
    if (unlink(fpath) == 0) return CCASK_OK;

    switch (errno) {
        case EACCES:
        case EISDIR:
        case EFAULT:
        case ENAMETOOLONG:
        case ENOENT:
        case EPERM:
        case EROFS:
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

ccask_status_e ccask_files_change_ext(uint64_t file_id, file_ext_e from, file_ext_e to) {
    char* from_fpath = build_filepath(files_state.data_dir, file_id, from);
    char* to_fpath = build_filepath(files_state.data_dir, file_id, to);

    if (rename(from_fpath, to_fpath) == 0) return CCASK_OK;

    switch (errno) {
        case EACCES:
        case EISDIR:
        case EFAULT:
        case ENAMETOOLONG:
        case ENOENT:
        case EPERM:
        case EROFS:
            return CCASK_FAIL;
        default:
            return CCASK_RETRY;
    }
}

ccask_status_e ccask_files_rotate(void) {
    int res;
    ccask_file_t *file = malloc(sizeof(ccask_file_t));
    if (!file) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    res = create_new_active_datafile(files_state.head->file_id + 1, file);
    if (res != CCASK_OK) {
        log_error("Failed to perform rotation of Active Datafile");
        return CCASK_FAIL;
    }

    close(files_state.head->fd);
    files_state.head->fd = -1;
    files_state.head->is_active = false;

    add_file(file);
    CCASK_ATTEMPT(5, res, ccask_hintfile_generate(files_state.head->next));
    
    return CCASK_OK;
}
