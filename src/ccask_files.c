
#include "ccask_files.h"

#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "sys/stat.h"
#include "string.h"
#include "errno.h"
#include "log.h"

void append_file_to_list(ccask_file_t* head, ccask_file_t* item) {
    if (!head) {
        head = item;
        item->next = NULL;
        item->previous = NULL;
        return;
    }

    ccask_file_t* curr = head;

    while (curr->next) {
        if (curr->id < item->id) {
            item->next = curr;
            item->previous = curr->previous;
            curr->previous->next = item;
            curr->previous = item;
            return;
        }

        curr = curr->next;
    }

    if (curr->id < item->id) {
        item->next = curr;
        item->previous = curr->previous;
        curr->previous->next = item;
        curr->previous = item;
        return;
    }

    curr->next = item;
    item->previous = curr;
    item->next = NULL;
    return;
}

int open_active_file(char* dir_path, uint64_t id) {
    int dir_len = strlen(dir_path);
    char* fpath = malloc(dir_len + 1 + 256 + 2); // directory path length + 1 for '/' + 256 for max file name length + 1 for '\0'

    strcpy(fpath, dir_path);
    if (fpath[dir_len - 1] != '/') {
        fpath[dir_len] = '/';
        dir_len++;
    }

    sprintf(&fpath[dir_len], "%d.data", id);

    int fd = open(
        fpath,
        O_CREAT | O_RDWR | O_APPEND,
        S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
    );

    if (fd < 0) {
        log_error("Could not create/open active file (ID=%d, Path=%s)", id, fpath);
        free(fpath);
        return -1;
    }

    free(fpath);
    return fd;
}

ccask_file_t* create_new_active_file(char* dir_path, uint64_t id) {
    int fd = open_active_file(dir_path, id);
    if (fd < 0) return NULL;

    ccask_file_t* active_file = malloc(sizeof(ccask_file_t));
    active_file->id = id;
    active_file->fd = fd;
    active_file->active = true;
    active_file->has_hint = false;
    active_file->next = NULL;
    active_file->previous = NULL;

    return active_file;
}

int ccask_files_load_directory(char* dir_path, ccask_file_t* head) {
    if (head != NULL) return ERR_LIST_NOT_EMPTY;
    
    DIR* dir = opendir(dir_path);
    int dir_len = strlen(dir_path);

    if (!dir) return ERR_DIR_NOT_FOUND;
    
    struct dirent* entry;
    char* fpath = malloc(dir_len + 1 + 256 + 2); // directory path length + 1 for '/' + 256 for max file name length + 1 for '\0'

    strcpy(fpath, dir_path);

    if (fpath[dir_len - 1] != '/') {
        fpath[dir_len] = '/';
        dir_len++;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") || strcmp(entry->d_name, "..")) {
            continue;
        }

        strcpy(&fpath[dir_len], entry->d_name);

        struct stat entry_stat;
        if (stat(fpath, &entry_stat) < 0) {
            log_error("Could not get stats for entry: %s", entry->d_name);
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

            if (strcmp(ext, ".data")) {
                log_info("Data File (ID=%d)", f_id);

                ccask_file_t* file = malloc(sizeof(ccask_file_t));
                file->id = f_id;
                file->fd = -1;
                file->active = false;
                file->has_hint = false;
                file->next = NULL;
                file->previous = NULL;

                append_file_to_list(head, file);
            }
        }
    }

    if (!head) {
        create_new_active_file(dir_path, 0);
    } else if (head->has_hint) {
        ccask_file_t* active_file = create_new_active_file(dir_path, head->id + 1);
        active_file->next = head;
        head = active_file;
    } else {
        int fd = open_active_file(dir_path, head->id);
        // TODO: DO something if file was not successfully opened

        head->active = true;
        head->fd = fd;
    }

    free(fpath);
    closedir(dir);
    return 0;
}

void ccask_files_destroy(ccask_file_t* head) {
    ccask_file_t* curr = head;
    while (curr) {
        if (curr->fd != -1) {
            close(curr->fd);
            curr->fd = -1;
        }

        ccask_file_t* tmp = curr;
        curr = curr->next;
        free(tmp);
    }
}