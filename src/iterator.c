
#include "ccask/iterator.h"

#include "unistd.h"
#include "ccask/files.h"
#include "ccask/errors.h"
#include "ccask/utils.h"

int ccask_datafile_iter_open(uint64_t file_id, ccask_datafile_iter_t *iter) {
    int fd = ccask_files_get_datafile_fd(file_id);
    if (fd < 0) return fd; // value of fd variable is the status code

    iter->file_id = file_id;
    iter->fd = fd;
    iter->offset = 0;
    iter->total_size = lseek(iter->fd, 0, SEEK_END);
}

uint64_t ccask_datafile_iter_next(ccask_datafile_iter_t *iter, ccask_datafile_record_t record) {
    if (iter->offset >= iter->total_size) {
        ccask_errno = ERR_ITERATOR_END;
        return -1;
    }

    uint64_t record_pos = iter->offset;

    ssize_t size = safe_preadv(iter->fd, record, 3, iter->offset);
    if (size == 0) {
        ccask_errno = ERR_ITERATOR_END;
        return CCASK_FAIL;
    } else if (size < 0) return size; // value of size variable is the status code
    
    iter->offset += size;

    return record_pos;
}

void ccask_datafile_iter_close(ccask_datafile_iter_t *iter) {
    close(iter->fd);
}

int ccask_hintfile_iter_open(uint64_t file_id, ccask_hintfile_iter_t *iter) {
    int fd = ccask_files_get_hintfile_fd(file_id);
    if (fd < 0) return fd; // value of fd variable is the status code

    iter->file_id = file_id;
    iter->fd = fd;
    iter->offset = 0;
    iter->total_size = lseek(iter->fd, 0, SEEK_END);
}

uint64_t ccask_hintfile_iter_next(ccask_hintfile_iter_t *iter, ccask_hintfile_record_t record) {
    if (iter->offset >= iter->total_size) {
        ccask_errno = ERR_ITERATOR_END;
        return -1;
    }

    uint64_t record_pos = iter->offset;

    ssize_t size = safe_preadv(iter->fd, record, 2, iter->offset);
    if (size == 0) {
        ccask_errno = ERR_ITERATOR_END;
        return CCASK_FAIL;
    } else if (size < 0) return size; // value of size variable is the status code

    iter->offset += size;

    return record_pos;
}

void ccask_hintfile_iter_close(ccask_hintfile_iter_t *iter) {
    close(iter->fd);
}
