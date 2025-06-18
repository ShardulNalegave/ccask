
#ifndef CCASK_UTILS_H
#define CCASK_UTILS_H

#include "stdint.h"
#include "sys/uio.h"

typedef enum file_ext_t {
    FILE_UNKNOWN,
    FILE_DATA,
    FILE_HINT
} file_ext_t;

file_ext_t parse_filename(const char* name, uint64_t *id);
char* build_filepath(const char* dir, uint64_t file_id, file_ext_t ext);

ssize_t safe_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t safe_readv(int fd, struct iovec *iov, int iovcnt);

ssize_t safe_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t safe_preadv(int fd, struct iovec *iov, int iovcnt, off_t offset);

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

#endif
