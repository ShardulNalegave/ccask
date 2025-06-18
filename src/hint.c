
#include "ccask/hint.h"

#include "stdlib.h"
#include "unistd.h"
#include "pthread.h"
#include "inttypes.h"
#include "ccask/files.h"
#include "ccask/iterator.h"
#include "ccask/utils.h"
#include "ccask/errors.h"
#include "ccask/log.h"

typedef struct thread_handle_t {
    pthread_t thread;
    struct thread_handle_t *next;
} thread_handle_t;

static thread_handle_t *threads;

int ccask_hintfile_generator_init(void) {
    threads = NULL;
}

void ccask_hintfile_generator_shutdown(void) {
    thread_handle_t* curr = threads;
    while (curr) {
        pthread_join(curr->thread, NULL);

        thread_handle_t *temp = curr->next;
        free(curr);
        curr = temp;
    }
}

void* hintfile_generator_thread(void* arg) {
    uint64_t file_id = *(uint64_t*)arg;
    int res; int retry_counter = 0;

    // get file-descriptor for new hintfile
    int hintfile_fd;
    do {
        hintfile_fd = ccask_files_get_hintfile_fd(file_id);
    } while (hintfile_fd == CCASK_RETRY && retry_counter++ <= 5);

    if (hintfile_fd == CCASK_FAIL) {
        log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
        return NULL;
    }

    // get iterator for datafile
    retry_counter = 0;
    ccask_datafile_iter_t iter;
    do {
        res = ccask_datafile_iter_open(file_id, &iter);
    } while (res == CCASK_RETRY && retry_counter++ <= 5);

    if (res == CCASK_FAIL) {
        log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
        return NULL;
    }

    int record_pos;
    ccask_datafile_record_t record;
    ccask_hintfile_record_t hint_record;
    while ((record_pos = ccask_datafile_iter_next(&iter, record)) >= 0) {
        ccask_datafile_record_header_t *header = ccask_get_datafile_record_header(record);
        void *key = ccask_get_datafile_record_key(record);
        ccask_create_hintfile_record(record, header->timestamp, header->key_size, header->value_size, record_pos, key);

        if (safe_writev(hintfile_fd, hint_record, 2) != CCASK_OK) {
            log_error("Hintfile generation failed (File ID = %" PRIu64 ")", file_id);
            retry_counter = 0;
            do {
                close(hintfile_fd);
                res = ccask_files_delete_hintfile(file_id); // delete the partially written hintfile
            } while (res == CCASK_RETRY && retry_counter++ <= 5);

            if (res != CCASK_OK) log_error("Couldn't delete partially written hintfile ID = %" PRIu64, file_id);
            return NULL;
        }
    }

    close(hintfile_fd);
    log_info("Hintfile generation completed (File ID = %" PRIu64 ")", file_id);
}

int ccask_hintfile_generate(uint64_t file_id) {
    thread_handle_t* handle = malloc(sizeof(thread_handle_t));
    pthread_create(&handle->thread, NULL, hintfile_generator_thread, &file_id);

    handle->next = threads;
    threads = handle;
}
