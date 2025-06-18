
#include "ccask/reader.h"

#include "time.h"
#include "unistd.h"
#include "pthread.h"
#include "inttypes.h"
#include "ccask/utils.h"
#include "ccask/errors.h"
#include "ccask/log.h"

#define FD_INVALIDATE_DURATION 5 // seconds

void* datafile_fd_invalidator_thread(void* arg) {
    ccask_file_t* file = (ccask_file_t*)arg;
    while (true) {
        sleep(FD_INVALIDATE_DURATION);

        time_t now = time(NULL);
        if (!file->is_active && file->fd >= 0 && (now - file->last_accessed) >= FD_INVALIDATE_DURATION) {
            pthread_rwlock_wrlock(&file->rwlock);
            close(file->fd);
            file->fd = -1;
            file->is_fd_invalidator_running = false;
            pthread_rwlock_unlock(&file->rwlock);
            return NULL;
        }
    }
}

int ccask_read_datafile_record(ccask_file_t *file, ccask_datafile_record_t record, uint64_t record_pos) {
    pthread_rwlock_rdlock(&file->rwlock);
    if (file->fd < 0) {
        pthread_rwlock_unlock(&file->rwlock);
        pthread_rwlock_wrlock(&file->rwlock);

        if (file->fd < 0) {
            int fd; int retry_counter = 5;
            do {
                fd = ccask_files_get_datafile_fd(file->file_id);
            } while (fd == CCASK_RETRY && retry_counter++ <= 5);

            if (fd == CCASK_FAIL) {
                log_error("Couldn't acquire a file-descriptor for Datafile ID = %" PRIu64, file->file_id);
                pthread_rwlock_unlock(&file->rwlock);
                return CCASK_FAIL;
            }

            file->fd = fd;

            pthread_t invalidator_thread;
            pthread_create(&invalidator_thread, NULL, datafile_fd_invalidator_thread, file);
            pthread_detach(invalidator_thread);
            file->is_fd_invalidator_running = true;
        }

        pthread_rwlock_unlock(&file->rwlock);
        pthread_rwlock_rdlock(&file->rwlock);
    }

    int res = safe_preadv(file->fd, record, 3, record_pos);
    if (res != 0) {
        log_error("Couldn't read record from Datafile ID = %" PRIu64 "; Record-Pos = %" PRIu64, file->file_id, record_pos);
        pthread_rwlock_unlock(&file->rwlock);
        return CCASK_FAIL;
    }

    file->last_accessed = time(NULL);
    pthread_rwlock_unlock(&file->rwlock);
}
