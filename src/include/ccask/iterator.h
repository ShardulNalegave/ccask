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

#ifndef CCASK_ITERATOR_H
#define CCASK_ITERATOR_H

#include "stdint.h"

#include "ccask/records.h"
#include "ccask/status.h"

typedef struct ccask_datafile_iter {
    uint64_t file_id;
    int fd;
    uint64_t offset;
    uint64_t total_size;
} ccask_datafile_iter_t;

ccask_status_e ccask_datafile_iter_open(uint64_t file_id, ccask_datafile_iter_t *iter);
int ccask_datafile_iter_next(ccask_datafile_iter_t *iter, ccask_datafile_record_t record, uint64_t *record_pos);
void ccask_datafile_iter_close(ccask_datafile_iter_t *iter);

typedef struct ccask_hintfile_iter {
    uint64_t file_id;
    int fd;
    uint64_t offset;
    uint64_t total_size;
} ccask_hintfile_iter_t;

ccask_status_e ccask_hintfile_iter_open(uint64_t file_id, ccask_hintfile_iter_t *iter);
int ccask_hintfile_iter_next(ccask_hintfile_iter_t *iter, ccask_hintfile_record_t record, uint64_t *record_pos);
void ccask_hintfile_iter_close(ccask_hintfile_iter_t *iter);

#endif
