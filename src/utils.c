
#include "ccask/utils.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "inttypes.h"
#include "endian.h"
#include "zlib.h"

file_ext_t parse_filename(const char* name, uint64_t *id) {
    const char *dot = strrchr(name, '.');

    size_t flen = dot - name;
    char *fname = malloc(flen + 1);
    memcpy(fname, name, flen);
    fname[flen] = '\0';

    *id = atoi(fname);

    if (strcmp(dot, ".data") == 0) {
        return FILE_DATA;
    } else if (strcmp(dot, ".hint") == 0) {
        return FILE_HINT;
    } else {
        return FILE_UNKNOWN;
    }
}

char* build_filepath(const char* dir, uint64_t file_id, file_ext_t ext) {
    char *ext_char;
    switch (ext) {
        case FILE_DATA:
            ext_char = ".data";
            break;
        case FILE_HINT:
            ext_char = ".hint";
            break;
        default:
            return NULL;
    }

    size_t dir_len = strlen(dir);
    bool has_trailing_slash = (dir[dir_len - 1] == '/');

    // Max ID digits (20 for uint64_t), dot, extension, slash if needed, null terminator
    size_t total = dir_len + (has_trailing_slash ? 0 : 1) + 20 + 1 + strlen(ext_char) + 1;

    char *path = malloc(total);
    if (!path) return NULL;

    if (has_trailing_slash) {
        sprintf(path, "%s%" PRIu64 "%s", dir, file_id, ext_char);
    } else {
        sprintf(path, "%s/%" PRIu64 "%s", dir, file_id, ext_char);
    }

    return path;
}

inline uint32_t calculate_crc32(
    uint32_t timestamp,
    uint32_t key_size,
    uint32_t value_size,
    void* key,
    void* value
) {
    uint32_t crc = (uint32_t)crc32(0, (unsigned char*)&timestamp, 4);
    crc = (uint32_t)crc32(crc, (unsigned char*)&key_size, 4);
    crc = (uint32_t)crc32(crc, (unsigned char*)&value_size, 4);
    crc = (uint32_t)crc32(crc, (unsigned char*)&key, key_size);
    crc = (uint32_t)crc32(crc, (unsigned char*)&value, value_size);
    return crc;
}

inline void write_be16(uint8_t *buf, uint16_t v) {
    uint16_t be = htobe16(v);
    memcpy(buf, &be, sizeof(be));
}

inline uint16_t read_be16(const uint8_t *buf) {
    uint16_t be;
    memcpy(&be, buf, 2);
    return be16toh(be);
}

inline void write_be32(uint8_t *buf, uint32_t v) {
    uint32_t be = htobe32(v);
    memcpy(buf, &be, sizeof(be));
}

inline uint32_t read_be32(const uint8_t *buf) {
    uint32_t be;
    memcpy(&be, buf, sizeof(be));
    return be32toh(be);
}

inline void write_be64(uint8_t *buf, uint64_t v) {
    uint64_t be = htobe64(v);
    memcpy(buf, &be, 8);
}

inline uint64_t read_be64(const uint8_t *buf) {
    uint64_t be;
    memcpy(&be, buf, 8);
    return be64toh(be);
}

ssize_t safe_writev(int fd, const struct iovec *iov, int iovcnt);
ssize_t safe_readv(int fd, struct iovec *iov, int iovcnt);

ssize_t safe_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset);
ssize_t safe_preadv(int fd, struct iovec *iov, int iovcnt, off_t offset);

ssize_t safe_pwrite(int fd, const void *buf, ssize_t len, off_t offset);
ssize_t safe_pread(int fd, const void *buf, ssize_t len, off_t offset);