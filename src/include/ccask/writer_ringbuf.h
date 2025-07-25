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

#ifndef CCASK_WRITER_RINGBUF_H
#define CCASK_WRITER_RINGBUF_H

#include "stddef.h"
#include "stdbool.h"
#include "ccask/records.h"

ccask_status_e ccask_writer_ringbuf_init(size_t capacity);
void ccask_writer_ringbuf_start_shutdown(void);
void ccask_writer_ringbuf_destroy(void);

size_t ccask_writer_ringbuf_count(void);

ccask_status_e ccask_writer_ringbuf_push(
    uint32_t timestamp,
    void *key,
    uint32_t key_size,
    void *value,
    uint32_t value_size
);

ccask_status_e ccask_writer_ringbuf_pop(ccask_datafile_record_t record);

#endif
