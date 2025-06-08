
#include "ccask_files.h"

#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "sys/stat.h"
#include "string.h"
#include "errno.h"
#include "log.h"

#define ACTIVE_DATAFILE_MAX_SIZE 60 // bytes
#define FD_INVALIDATE_DURATION 5 // seconds

static const int DATAFILE_OPEN_MODE = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
static const int DATAFILE_OPEN_FLAGS = O_RDONLY;
static const int ACTIVE_DATAFILE_OPEN_FLAGS = O_CREAT | O_RDWR | O_APPEND;

static char* files_dirpath = NULL;
static ccask_file_t *files_head = NULL, *files_tail = NULL;
static ccask_file_t* files_hash_table = NULL;

ccask_file_t* ccask_files_get_active_datafile() {
    return files_head;
}

ccask_file_t* ccask_files_get_oldest_datafile() {
    return files_tail;
}

ccask_file_t* ccask_files_get_file(uint64_t id) {
    ccask_file_t* file = NULL;
    HASH_FIND(hh, files_hash_table, &id, sizeof(uint64_t), file);
    return file;
}

void add_file_to_linked_list(ccask_file_t* file) {
    if (!files_head) {
        files_head = file;
        files_tail = file;
        file->next = NULL;
        file->previous = NULL;
        return;
    }

    ccask_file_t* prev = NULL;
    ccask_file_t* curr = files_head;

    // insert at head
    if (file->id > files_head->id) {
        file->previous = NULL;
        file->next = files_head;
        files_head->previous = file;
        files_head = file;
        return;
    }

    // insert in middle
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

    // insert at tail
    prev->next = file;
    file->previous = prev;
    file->next = NULL;
    files_tail = file;
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

int get_datafile_fd(char* fpath) {
    int fd = open(fpath, DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    return fd;
}

int get_active_datafile_fd(char* fpath) {
    int fd = open(fpath, ACTIVE_DATAFILE_OPEN_FLAGS, DATAFILE_OPEN_MODE);
    return fd;
}

ccask_file_t* allocate_datafile_node(char* dirpath, uint64_t id) {
    ccask_file_t* datafile = malloc(sizeof(ccask_file_t));
    datafile->id = id;
    datafile->fd = -1;
    datafile->active = false;
    datafile->has_hint = false;
    datafile->datafile_path = get_datafile_path(dirpath, id);
    datafile->readers = 0;
    datafile->is_invalidator_running = false;
    datafile->last_accessed = time(NULL);
    datafile->next = NULL;
    datafile->previous = NULL;

    pthread_mutex_init(&datafile->mutex, NULL);

    return datafile;
}

ccask_file_t* create_new_active_datafile(char* dirpath, uint64_t id) {
    char* fpath = get_datafile_path(dirpath, id);

    int fd = get_active_datafile_fd(fpath);
    if (fd < 0) return NULL;

    ccask_file_t* active_file = malloc(sizeof(ccask_file_t));
    active_file->id = id;
    active_file->fd = fd;
    active_file->active = true;
    active_file->has_hint = false;
    active_file->datafile_path = fpath;
    active_file->readers = 0;
    active_file->is_invalidator_running = false;
    active_file->last_accessed = time(NULL);
    active_file->next = NULL;
    active_file->previous = NULL;

    pthread_mutex_init(&active_file->mutex, NULL);

    log_info("Created new Active Data-File (ID=%d)", id);
    return active_file;
}

int ccask_files_init(char* dirpath) {
    files_dirpath = malloc(strlen(dirpath) + 1);
    strcpy(files_dirpath, dirpath);

    files_head = NULL;
    files_tail = NULL;
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
        int fd = get_active_datafile_fd(files_head->datafile_path);
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
    free(files_dirpath);
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

        HASH_DEL(files_hash_table, curr);

        pthread_mutex_destroy(&curr->mutex);
        free(curr->datafile_path);
        free(curr);
    }

    files_head = NULL;
    files_hash_table = NULL;
}

void* fd_invalidate_thread(void* arg) {
    ccask_file_t* file = (ccask_file_t*)arg;
    while (true) {
        sleep(FD_INVALIDATE_DURATION);
        pthread_mutex_lock(&file->mutex);

        time_t now = time(NULL);
        if (!file->active && file->readers == 0 && file->fd >= 0 && (now - file->last_accessed) >= FD_INVALIDATE_DURATION) {
            close(file->fd);
            file->fd = -1;
            file->is_invalidator_running = false;
            pthread_mutex_unlock(&file->mutex);
            return NULL;
        }

        pthread_mutex_unlock(&file->mutex);
    }
}

uint8_t* ccask_files_read_chunk(uint64_t id, uint64_t pos, uint32_t len) {
    ccask_file_t* file = ccask_files_get_file(id);
    if (!file) return NULL;

    uint8_t* chunk = malloc(len);
    if (!chunk) return NULL;

    pthread_mutex_lock(&file->mutex);
    if (file->fd < 0) {
        file->fd = get_datafile_fd(file->datafile_path);
        if (file->fd < 0) {
            log_error("Couldn't acquire lock on Data-File ID=%d for reading chunk", id);
            pthread_mutex_unlock(&file->mutex);
            free(chunk);
            return NULL;
        }
    }

    file->readers++;
    file->last_accessed = time(NULL);
    if (!file->active && !file->is_invalidator_running) {
        pthread_t invalidator;
        pthread_create(&invalidator, NULL, fd_invalidate_thread, (void*)file);
        pthread_detach(invalidator);
    }
    pthread_mutex_unlock(&file->mutex);

    size_t to_read = len;
    off_t offset = pos;
    uint8_t* p = chunk;

    while (to_read > 0) {
        ssize_t got = pread(file->fd, p, to_read, offset);
        if (got < 0) {
            if (errno == EINTR) continue;  // retry on signal
            log_error("Couldn't read chunk from Data-File ID=%d\n\t%s", id, strerror(errno));
            free(chunk);
            return NULL;
        } else if (got == 0) {
            log_error("Unexpected EOF while reading chunk from Data-File ID=%d", id);
            free(chunk);
            return NULL;
        }

        to_read -= got;
        p += got;
        offset += got;
    }

    pthread_mutex_lock(&file->mutex);
    file->readers--;
    pthread_mutex_unlock(&file->mutex);
    return chunk;
}

int rotate_active_datafile() {
    ccask_file_t* new_active_file = create_new_active_datafile(files_dirpath, files_head->id + 1);
    if (!new_active_file) {
        log_fatal("Could not create new active file\n\t%s", strerror(errno));
        return -1;
    }

    close(files_head->fd);
    files_head->fd = -1;
    files_head->active = false;
    add_file(new_active_file);
    return 0;
}

off_t ccask_files_write_chunk(void* buff, uint32_t len) {
    ccask_file_t* file = files_head;
    if (!file) {
        log_error("Active Data-File was NULL during write chunk");
        return -1;
    }

    pthread_mutex_lock(&file->mutex);

    // write are allowed only on active data-files, thus an fd should already be present
    if (!file->active || file->fd < 0) {
        log_error("Write chunk requested on non-active or closed active Data-File ID=%d", file->id);
        pthread_mutex_unlock(&file->mutex);
        return -1;
    }

    off_t write_pos = lseek(file->fd, 0, SEEK_END); // also denotes current file size (as active file is append only)
    size_t to_write = len;

    if (!file->active || write_pos + to_write > ACTIVE_DATAFILE_MAX_SIZE) {
        if(rotate_active_datafile() < 0) {
            log_error("Write chunk cancelled as rotation of active data-file failed");
            pthread_mutex_unlock(&file->mutex);
            return -1;
        }

        pthread_mutex_unlock(&file->mutex);
        
        file = files_head;
        write_pos = 0;

        pthread_mutex_lock(&file->mutex);
        log_info("Rotation successful: Using new Active Data-File ID=%d", file->id);
    }

    uint8_t* p = buff;

    while (to_write > 0) {
        ssize_t got = write(file->fd, p, to_write);
        if (got < 0) {
            if (errno == EINTR) continue;  // retry on signal
            ftruncate(file->fd, write_pos);
            log_error("Couldn't write chunk to Active Data-File ID=%d\n\t%s", file->id, strerror(errno));
            pthread_mutex_unlock(&file->mutex);
            return -1;
        }

        to_write -= got;
        p += got;
    }

    pthread_mutex_unlock(&file->mutex);
    return write_pos;
}

int ccask_files_read_entire_datafile(ccask_file_t* file, uint8_t** buffer) {
    int fd = get_datafile_fd(file->datafile_path);
    if (fd < 0) {
        log_error("Couldn't get a file-descriptor when reading datafile ID=%d", file->id);
        *buffer = NULL;
        return -1;
    }

    size_t file_size = lseek(fd, 0, SEEK_END);
    size_t to_read = file_size;
    off_t offset = 0;

    *buffer = malloc(to_read);
    uint8_t* p = *buffer;

    while (to_read > 0) {
        ssize_t got = pread(fd, p, to_read, offset);
        if (got < 0) {
            if (errno == EINTR) continue;  // retry on signal
            log_error("Couldn't read Data-File ID=%d\n\t%s", file->id, strerror(errno));
            free(*buffer);
            *buffer = NULL;
            return -1;
        } else if (got == 0) {
            log_error("Unexpected EOF while reading chunk from Data-File ID=%d", file->id);
            free(*buffer);
            *buffer = NULL;
            return -1;
        }

        to_read -= got;
        p += got;
        offset += got;
    }

    close(fd);
    return file_size;
}