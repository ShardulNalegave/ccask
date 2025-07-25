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

#ifndef CCASK_UTILS_H
#define CCASK_UTILS_H

#include "stdint.h"
#include "sys/uio.h"

typedef enum file_ext {
    FILE_UNKNOWN,
    FILE_DATA,
    FILE_HINT,
    FILE_TEMP_DATA
} file_ext_e;

file_ext_e parse_filename(const char* name, uint64_t *id);
char* build_filepath(const char* dir, uint64_t file_id, file_ext_e ext);

uint32_t calculate_crc32(
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    void* key,
    void* value
);

void write_be16(uint8_t *buf, uint16_t v);
void write_be32(uint8_t *buf, uint32_t v);
void write_be64(uint8_t *buf, uint64_t v);

uint16_t read_be16(const uint8_t *buf);
uint32_t read_be32(const uint8_t *buf);
uint64_t read_be64(const uint8_t *buf);

int safe_writev(int fd, const struct iovec *iov, int iovcnt);
int safe_readv(int fd, struct iovec *iov, int iovcnt);

int safe_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
int safe_preadv(int fd, struct iovec *iov, int iovcnt, off_t offset);

int safe_pread(int fd, void *buf, ssize_t len, off_t offset);

#endif
