
#include "ccask_hintfile_gen.h"

#include "string.h"
#include "pthread.h"
#include "errno.h"
#include "log.h"

#include "ccask_crc_check.h"
#include "ccask_records.h"
#include "ccask_records_iter.h"

typedef struct hintfile_gen_thread_t {
    pthread_t thread;
    struct hintfile_gen_thread_t* next;
} hintfile_gen_thread_t;

static hintfile_gen_thread_t* gen_threads_head = NULL;

void add_gen_thread(hintfile_gen_thread_t* gen_thread) {
    gen_thread->next = gen_threads_head;
    gen_threads_head = gen_thread;
}

void ccask_hintfile_generator_init() {
    gen_threads_head = NULL;
}

void ccask_hintfile_generator_shutdown() {
    hintfile_gen_thread_t* curr = gen_threads_head;
    while (curr) {
        pthread_join(curr->thread, NULL);
        free(curr);
    }
}

void* hintfile_generator_thread(void* arg) {
    ccask_file_t* file = arg;
    pthread_mutex_lock(&file->mutex);
    file->hintfile_path = ccask_get_hintfile_path(file->id);
    pthread_mutex_unlock(&file->mutex);

    int hintfile_fd = ccask_get_hintfile_fd(file->hintfile_path);
    if (hintfile_fd < 0) {
        log_error("Couldn't get FD for hintfile ID=%d\n\t%s", file->id, strerror(errno));
        return NULL;
    }

    ccask_datafile_iter_t* iter;
    if (ccask_datafile_record_iter_open(file, &iter) < 0) {
        log_error("Couldn't read datafile ID=%d while generating hintfile", file->id);
        return NULL;
    }

    int num_saved = 0;
    ccask_datafile_record_t* record;

    uint64_t record_pos = iter->offset;
    ccask_datafile_record_iter_next(iter, &record);

    while (record) {
        uint32_t crc = ccask_crc_calculate_with_datafile_record(record);
        if (crc != record->crc) {
            log_error("CRC check failed for record from datafile ID=%s while generating hintfile", file->id);
            free(record);
            uint64_t record_pos = iter->offset;
            ccask_datafile_record_iter_next(iter, &record);
            continue;
        }

        ccask_hintfile_record_t hintfile_record = {
            .timestamp = record->timestamp,
            .value_size = record->value_size,
            .key = record->key,
            .key_size = record->key_size,
            .record_pos = record_pos
        };

        uint8_t* buffer;
        uint64_t size = ccask_hintfile_record_serialize(&buffer, &hintfile_record);

        if (ccask_files_write_chunk_to_fd(hintfile_fd, buffer, size) < 0) {
            log_error("Failed to write record to hintfile ID=%d", file->id);
        } else {
            num_saved++;
        }

        uint64_t record_pos = iter->offset;
        free(buffer);
        free(record);
        ccask_datafile_record_iter_next(iter, &record);
    }

    log_info("Generated Hintfile ID=%d (saved %d records)", file->id, num_saved);
    ccask_datafile_record_iter_close(iter);
    return 0;
}

void ccask_hintfile_generate(ccask_file_t* file) {
    if (file->active) {
        log_error("Cannot generate hintfile for active files");
        return;
    }

    pthread_t thread;
    hintfile_gen_thread_t* gen_thread = malloc(sizeof(hintfile_gen_thread_t));
    gen_thread->thread = thread;
    pthread_create(&gen_thread->thread, NULL, hintfile_generator_thread, file);
    add_gen_thread(gen_thread);
}
