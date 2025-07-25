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

#include "ccask/iterator.h"

#include "unistd.h"
#include "ccask/files.h"
#include "ccask/status.h"
#include "ccask/utils.h"

ccask_status_e ccask_datafile_iter_open(uint64_t file_id, ccask_datafile_iter_t *iter) {
    int fd;
    CCASK_ATTEMPT(5, fd, ccask_files_get_datafile_fd(file_id));
    if (fd < 0) return CCASK_FAIL;

    iter->file_id = file_id;
    iter->fd = fd;
    iter->offset = 0;
    iter->total_size = lseek(iter->fd, 0, SEEK_END);
    return CCASK_OK;
}

int ccask_datafile_iter_next(ccask_datafile_iter_t *iter, ccask_datafile_record_t record, uint64_t *record_pos) {
    if (iter->offset >= iter->total_size) {
        ccask_errno = CCASK_ERR_ITER_END;
        return CCASK_FAIL;
    }

    *record_pos = iter->offset;

    record[0].iov_base = malloc(DATAFILE_RECORD_HEADER_SIZE);
    if (!record[0].iov_base) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    int res = safe_pread(iter->fd, record[0].iov_base, DATAFILE_RECORD_HEADER_SIZE, iter->offset);
    if (res != CCASK_OK) {
        if (ccask_errno == CCASK_ERR_UNEXPECTED_EOF) ccask_errno = CCASK_ERR_ITER_END;
        return CCASK_FAIL;
    }

    ccask_datafile_record_header_t header = ccask_get_datafile_record_header(record);
    record[1].iov_len = header.key_size;
    record[2].iov_len = header.value_size;
    
    record[1].iov_base = malloc(record[1].iov_len);
    if (!record[1].iov_base) {
        free(record[0].iov_base);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    record[2].iov_base = malloc(record[2].iov_len);
    if (!record[2].iov_base) {
        free(record[0].iov_base);
        free(record[1].iov_base);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    res = safe_preadv(iter->fd, record + 1, 2, iter->offset + DATAFILE_RECORD_HEADER_SIZE);
    if (res != CCASK_OK) {
        return CCASK_FAIL;
    }
    
    iter->offset += DATAFILE_RECORD_HEADER_SIZE + header.key_size + header.value_size;
    return CCASK_OK;
}

void ccask_datafile_iter_close(ccask_datafile_iter_t *iter) {
    close(iter->fd);
}

ccask_status_e ccask_hintfile_iter_open(uint64_t file_id, ccask_hintfile_iter_t *iter) {
    int fd;
    CCASK_ATTEMPT(5, fd, ccask_files_get_hintfile_fd(file_id));
    if (fd < 0) return CCASK_FAIL;

    iter->file_id = file_id;
    iter->fd = fd;
    iter->offset = 0;
    iter->total_size = lseek(iter->fd, 0, SEEK_END);
    return CCASK_OK;
}

int ccask_hintfile_iter_next(ccask_hintfile_iter_t *iter, ccask_hintfile_record_t record, uint64_t *record_pos) {
    if (iter->offset >= iter->total_size) {
        ccask_errno = CCASK_ERR_ITER_END;
        return CCASK_FAIL;
    }

    *record_pos = iter->offset;

    record[0].iov_base = malloc(HINTFILE_RECORD_HEADER_SIZE);
    if (!record[0].iov_base) {
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    int res = safe_pread(iter->fd, record[0].iov_base, HINTFILE_RECORD_HEADER_SIZE, iter->offset);
    if (res < 0) {
        if (ccask_errno == CCASK_ERR_UNEXPECTED_EOF) ccask_errno = CCASK_ERR_ITER_END;
        return CCASK_FAIL;
    }

    ccask_hintfile_record_header_t header = ccask_get_hintfile_record_header(record);
    record[1].iov_len = header.key_size;
    
    record[1].iov_base = malloc(record[1].iov_len);
    if (!record[1].iov_base) {
        free(record[0].iov_base);
        ccask_errno = CCASK_ERR_NO_MEMORY;
        return CCASK_FAIL;
    }

    res = safe_preadv(iter->fd, record + 1, 1, iter->offset + HINTFILE_RECORD_HEADER_SIZE);
    if (res != CCASK_OK) {
        return CCASK_FAIL;
    }

    iter->offset += HINTFILE_RECORD_HEADER_SIZE + header.key_size;
    return CCASK_OK;
}

void ccask_hintfile_iter_close(ccask_hintfile_iter_t *iter) {
    close(iter->fd);
}
