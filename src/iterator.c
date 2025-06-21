
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
        return CCASK_FAIL;
    }

    uint64_t record_pos = iter->offset;

    int retry_counter = 0;
    do {
        record[0].iov_base = malloc(DATAFILE_RECORD_HEADER_SIZE);
    } while (!record[0].iov_base && retry_counter++ <= 5);

    if (!record[0].iov_base) {
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    int res = safe_pread(iter->fd, record[0].iov_base, DATAFILE_RECORD_HEADER_SIZE, iter->offset);
    if (res < 0) {
        if (ccask_errno == ERR_UNEXPECTED_EOF) ccask_errno = ERR_ITERATOR_END;
        return CCASK_FAIL;
    }

    ccask_datafile_record_header_t *header = ccask_get_datafile_record_header(record);
    record[1].iov_len = header->key_size;
    record[2].iov_len = header->value_size;
    
    retry_counter = 0;
    do {
        record[1].iov_base = malloc(record[1].iov_len);
    } while (!record[1].iov_base && retry_counter++ <= 5);

    if (!record[1].iov_base) {
        free(record[0].iov_base);
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    retry_counter = 0;
    do {
        record[2].iov_base = malloc(record[2].iov_len);
    } while (!record[2].iov_base && retry_counter++ <= 5);

    if (!record[2].iov_base) {
        free(record[0].iov_base);
        free(record[1].iov_base);
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    ssize_t size = safe_preadv(iter->fd, record + 1, 2, iter->offset + DATAFILE_RECORD_HEADER_SIZE);
    if (size == 0) {
        ccask_errno = ERR_UNEXPECTED_EOF;
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
        return CCASK_FAIL;
    }

    uint64_t record_pos = iter->offset;

    int retry_counter = 0;
    do {
        record[0].iov_base = malloc(HINTFILE_RECORD_HEADER_SIZE);
    } while (!record[0].iov_base && retry_counter++ <= 5);

    if (!record[0].iov_base) {
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    int res = safe_pread(iter->fd, record[0].iov_base, HINTFILE_RECORD_HEADER_SIZE, iter->offset);
    if (res < 0) {
        if (ccask_errno == ERR_UNEXPECTED_EOF) ccask_errno = ERR_ITERATOR_END;
        return CCASK_FAIL;
    }

    ccask_hintfile_record_header_t *header = ccask_get_hintfile_record_header(record);
    record[1].iov_len = header->key_size;
    
    retry_counter = 0;
    do {
        record[1].iov_base = malloc(record[1].iov_len);
    } while (!record[1].iov_base && retry_counter++ <= 5);

    if (!record[1].iov_base) {
        free(record[0].iov_base);
        ccask_errno = ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    ssize_t size = safe_preadv(iter->fd, record + 1, 2, iter->offset + HINTFILE_RECORD_HEADER_SIZE);
    if (size == 0) {
        ccask_errno = ERR_UNEXPECTED_EOF;
        return CCASK_FAIL;
    } else if (size < 0) return size; // value of size variable is the status code

    size = safe_preadv(iter->fd, record, 2, iter->offset);
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
