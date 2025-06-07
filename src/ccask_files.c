
#include "ccask_files.h"

#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "sys/stat.h"
#include "string.h"
#include "errno.h"
#include "log.h"

static const int DATAFILE_OPEN_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
static const int DATAFILE_OPEN_FLAGS = O_RDONLY;
static const int ACTIVE_DATAFILE_OPEN_FLAGS = O_CREAT | O_RDWR | O_APPEND;

static ccask_file_t *files_head = NULL;
static ccask_file_t* files_hash_table = NULL;

ccask_file_t* ccask_files_get_active_file() {
    return files_head;
}

ccask_file_t* ccask_files_get_file(uint64_t id) {
    ccask_file_t* file = NULL;
    HASH_FIND(hh, files_hash_table, &id, sizeof(uint64_t), file);
    return file;
}

void add_file_to_linked_list(ccask_file_t* file) {
    if (!files_head) {
        files_head = file;
        file->next = NULL;
        file->previous = NULL;
        return;
    }

    ccask_file_t* prev = NULL;
    ccask_file_t* curr = files_head;

    while (curr) {
        if (curr->id < file->id) {
            file->next = curr;
            file->previous = prev;
            curr->previous = file;
            if (prev) prev->next = file;
            return;
        }

        prev = curr;
        curr = curr->next;
    }

    prev->next = file;
    file->previous = prev;
    file->next = NULL;
    return;
}

void add_file(ccask_file_t* file) {
    add_file_to_linked_list(file);
    HASH_ADD(hh, files_hash_table, id, sizeof(uint64_t), file);
}

char* get_datafile_path(char* dirpath, uint64_t id) {
    int dir_len = strlen(dirpath);
    char* fpath = malloc(dir_len + 1 + 256 + 2); // directory path length + 1 for '/' + 256 for max file name length + 1 for '\0'

    strcpy(fpath, dirpath);
    if (fpath[dir_len - 1] != '/') {
        fpath[dir_len] = '/';
        dir_len++;
    }

    sprintf(&fpath[dir_len], "%d.data", id);
    return fpath;
}

int get_datafile_fd(char* dirpath, uint64_t id) {
    char* fpath = get_datafile_path(dirpath, id);
    int fd = open(fpath, DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    free(fpath);
    return fd;
}

int get_active_datafile_fd(char* dirpath, uint64_t id) {
    char* fpath = get_datafile_path(dirpath, id);
    int fd = open(fpath, ACTIVE_DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    free(fpath);
    return fd;
}

ccask_file_t* allocate_datafile_node(char* dirpath, uint64_t id) {
    ccask_file_t* datafile = malloc(sizeof(ccask_file_t));
    datafile->id = id;
    datafile->fd = -1;
    datafile->active = false;
    datafile->has_hint = false;
    datafile->next = NULL;
    datafile->previous = NULL;

    pthread_mutex_init(&datafile->mutex, NULL);

    return datafile;
}

ccask_file_t* create_new_active_datafile(char* dirpath, uint64_t id) {
    int fd = get_active_datafile_fd(dirpath, id);
    if (fd < 0) return NULL;

    ccask_file_t* active_file = malloc(sizeof(ccask_file_t));
    active_file->id = id;
    active_file->fd = fd;
    active_file->active = true;
    active_file->has_hint = false;
    active_file->next = NULL;
    active_file->previous = NULL;

    pthread_mutex_init(&active_file->mutex, NULL);

    log_info("Created new Active Data-File (ID=%d)", id);
    return active_file;
}

int ccask_files_init(char* dirpath) {
    files_head = NULL;
    files_hash_table = NULL;

    DIR* dir = opendir(dirpath);
    int dir_len = strlen(dirpath);
    if (!dir) {
        log_fatal("Error while opening data-directory\n\t%s", strerror(errno));
        return -1;
    }

    struct dirent* entry;
    char* fpath = malloc(dir_len + 1 + 256 + 2); // directory path length + 1 for '/' + 256 for max file name length + 1 for '\0'

    strcpy(fpath, dirpath);

    if (fpath[dir_len - 1] != '/') {
        fpath[dir_len] = '/';
        dir_len++;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        strcpy(&fpath[dir_len], entry->d_name);

        struct stat entry_stat;
        if (stat(fpath, &entry_stat) < 0) {
            log_error("Could not get stats for entry: %s\n\t%s", entry->d_name, strerror(errno));
            continue;
        }

        if (S_ISREG(entry_stat.st_mode)) {
            char* ext = strrchr(entry->d_name, '.');

            size_t fname_len = ext - entry->d_name;
            char* fname = malloc(fname_len + 1);
            memcpy(fname, entry->d_name, fname_len);
            fname[fname_len] = '\0';

            int f_id = atoi(fname);
            free(fname);

            if (strcmp(ext, ".data") == 0) {
                log_info("Data File (ID=%d)", f_id);
                ccask_file_t* file = allocate_datafile_node(dirpath, f_id);
                add_file(file);
            }
        }
    }

    free(fpath);
    closedir(dir);

    if (!files_head) {
        ccask_file_t* active_file = create_new_active_datafile(dirpath, 0);
        if (!active_file) {
            log_fatal("Could not create new active file\n\t%s", strerror(errno));
            return -1;
        }
        add_file(active_file);
    } else if (files_head->has_hint) {
        ccask_file_t* active_file = create_new_active_datafile(dirpath, files_head->id + 1);
        if (!active_file) {
            log_fatal("Could not create new active file\n\t%s", strerror(errno));
            return -1;
        }
        add_file(active_file);
    } else {
        int fd = get_active_datafile_fd(dirpath, files_head->id);
        files_head->active = true;
        files_head->fd = fd;
        if (fd < 0) {
            log_fatal("Could not get active file fd\n\t%s", strerror(errno));
            return -1;
        }
    }

    log_info("Using Data-File ID=%d as Active Data-File", files_head->id);
    return 0;
}

void ccask_files_destroy() {
    ccask_file_t *curr, *tmp;

    HASH_ITER(hh, files_hash_table, curr, tmp) {
        if (curr->fd >= 0) {
            close(curr->fd);
            curr->fd = -1;
        }

        // Unlink from linked list
        if (curr->previous) {
            curr->previous->next = curr->next;
        } else {
            files_head = curr->next;
        }
        if (curr->next) {
            curr->next->previous = curr->previous;
        }

        pthread_mutex_destroy(&curr->mutex);

        HASH_DEL(files_hash_table, curr);
        free(curr);
    }

    files_head = NULL;
    files_hash_table = NULL;
}