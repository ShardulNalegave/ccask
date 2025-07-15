
#include "ccask/reader.h"

#include "time.h"
#include "unistd.h"
#include "pthread.h"
#include "inttypes.h"
#include "ccask/files.h"
#include "ccask/utils.h"
#include "ccask/log.h"

#define FD_INVALIDATE_DURATION 5 // seconds

static void* datafile_fd_invalidator_thread(void* arg) {
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

ccask_status_e ccask_read_datafile_record(uint64_t file_id, ccask_datafile_record_t record, uint64_t record_pos) {
    int ret = CCASK_OK;

    ccask_file_t *file = ccask_files_get_file(file_id);
    if (!file) {
        return CCASK_ERR_NO_SUCH_DATAFILE;
    }

    pthread_rwlock_rdlock(&file->rwlock);
    bool needs_open = (file->fd < 0);
    pthread_rwlock_unlock(&file->rwlock);

    if (needs_open) {
        pthread_rwlock_wrlock(&file->rwlock);
        if (file->fd < 0) {
            int fd;
            CCASK_RETRY(5, fd, ccask_files_get_datafile_fd(file->file_id));

            if (fd < 0) {
                log_error("Could not open Datafile ID=%" PRIu64, file->file_id);
                ret = CCASK_FAIL;
            } else {
                file->fd = fd;
                if (!file->is_fd_invalidator_running) {
                    pthread_t thr;
                    int res;
                    CCASK_RETRY(5, res, pthread_create(
                        &thr,
                        NULL,
                        datafile_fd_invalidator_thread,
                        file
                    ));

                    if (res != 0) {
                        log_error("Couldn't spawn FD invalidator thread for Datafile ID = %d" PRIu64, file->file_id);
                    } else {
                        pthread_detach(thr);
                        file->is_fd_invalidator_running = true;
                    }
                }
            }
        }
        pthread_rwlock_unlock(&file->rwlock);
        if (ret != CCASK_OK) return ret;
    }

    pthread_rwlock_rdlock(&file->rwlock);
    ssize_t n = safe_preadv(file->fd, record, 3, record_pos);
    if (n < 0) {
        log_error("Read failed on Datafile ID=%" PRIu64, file->file_id);
        ccask_errno = CCASK_ERR_READ_FAILED;
        ret = CCASK_FAIL;
    }

    file->last_accessed = time(NULL);
    pthread_rwlock_unlock(&file->rwlock);

    return ret;
}
