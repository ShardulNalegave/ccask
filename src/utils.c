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

#include "ccask/utils.h"

#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "inttypes.h"
#include "endian.h"
#include "errno.h"
#include "zlib.h"
#include "ccask/status.h"

file_ext_e parse_filename(const char* name, uint64_t *id) {
    const char *dot = strrchr(name, '.');

    size_t flen = dot - name;
    char *fname = malloc(flen + 1);
    memcpy(fname, name, flen);
    fname[flen] = '\0';

    *id = atoi(fname);

    if (strcmp(dot, ".data.tmp") == 0) {
        return FILE_TEMP_DATA;
    } else if (strcmp(dot, ".data") == 0) {
        return FILE_DATA;
    } else if (strcmp(dot, ".hint") == 0) {
        return FILE_HINT;
    } else {
        return FILE_UNKNOWN;
    }
}

char* build_filepath(const char* dir, uint64_t file_id, file_ext_e ext) {
    char *ext_char;
    switch (ext) {
        case FILE_DATA:
            ext_char = ".data";
            break;
        case FILE_HINT:
            ext_char = ".hint";
            break;
        case FILE_TEMP_DATA:
            ext_char = ".data.tmp";
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
    crc = (uint32_t)crc32(crc, (unsigned char*)key, key_size);
    crc = (uint32_t)crc32(crc, (unsigned char*)value, value_size);
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

static inline size_t iov_total_len(const struct iovec *iov, int iovcnt) {
    size_t sum = 0;
    for (int i = 0; i < iovcnt; i++) sum += iov[i].iov_len;
    return sum;
}

int safe_writev(int fd, const struct iovec *iov, int iovcnt) {
    if (fd < 0 || iov == NULL || iovcnt <= 0) {
        return CCASK_FAIL;
    }

    struct iovec local_iov[iovcnt];
    memcpy(local_iov, iov, iovcnt * sizeof(struct iovec)); // shallow-copy

    int current_iov_idx = 0;
    ssize_t total_written = 0;
    
    while (current_iov_idx < iovcnt) {
        ssize_t nwritten = writev(fd, &local_iov[current_iov_idx], iovcnt - current_iov_idx);

        if (nwritten < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            ccask_errno = CCASK_ERR_WRITE_FAILED;
            return CCASK_FAIL;
        }

        total_written += nwritten;

        while (current_iov_idx < iovcnt && nwritten > 0) {
            if ((size_t)nwritten < local_iov[current_iov_idx].iov_len) {
                local_iov[current_iov_idx].iov_base = (char *)local_iov[current_iov_idx].iov_base + nwritten;
                local_iov[current_iov_idx].iov_len -= nwritten;
                nwritten = 0;
            } else {
                nwritten -= local_iov[current_iov_idx].iov_len;
                current_iov_idx++;
            }
        }
    }

    return CCASK_OK;
}

int safe_readv(int fd, struct iovec *iov, int iovcnt) {
    if (fd < 0 || iov == NULL || iovcnt <= 0) {
        return CCASK_FAIL;
    }

    struct iovec local_iov[iovcnt];
    memcpy(local_iov, iov, iovcnt * sizeof(struct iovec)); // shallow-copy

    int current_iov_idx = 0;
    ssize_t total_read = 0;

    while (current_iov_idx < iovcnt) {
        ssize_t nread = readv(fd, &local_iov[current_iov_idx], iovcnt - current_iov_idx);

        if (nread < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            ccask_errno = CCASK_ERR_READ_FAILED;
            return CCASK_FAIL;
        }
        
        if (nread == 0) {
            ccask_errno = CCASK_ERR_UNEXPECTED_EOF;
            return CCASK_FAIL;
        }

        total_read += nread;

        while (current_iov_idx < iovcnt && nread > 0) {
            if ((size_t)nread < local_iov[current_iov_idx].iov_len) {
                local_iov[current_iov_idx].iov_base = (char *)local_iov[current_iov_idx].iov_base + nread;
                local_iov[current_iov_idx].iov_len -= nread;
                nread = 0;
            } else {
                nread -= local_iov[current_iov_idx].iov_len;
                current_iov_idx++;
            }
        }
    }

    return CCASK_OK;
}

int safe_pwritev(int fd, const struct iovec *iov, int iovcnt, off_t offset) {
    if (fd < 0 || iov == NULL || iovcnt <= 0) {
        return CCASK_FAIL;
    }

    struct iovec local_iov[iovcnt];
    memcpy(local_iov, iov, iovcnt * sizeof(struct iovec)); // shallow-copy

    int current_iov_idx = 0;
    ssize_t total_written = 0;
    
    while (current_iov_idx < iovcnt) {
        ssize_t nwritten = pwritev(fd, &local_iov[current_iov_idx], iovcnt - current_iov_idx, offset + total_written);

        if (nwritten < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            ccask_errno = CCASK_ERR_WRITE_FAILED;
            return CCASK_FAIL;
        }

        total_written += nwritten;

        while (current_iov_idx < iovcnt && nwritten > 0) {
            if ((size_t)nwritten < local_iov[current_iov_idx].iov_len) {
                local_iov[current_iov_idx].iov_base = (char *)local_iov[current_iov_idx].iov_base + nwritten;
                local_iov[current_iov_idx].iov_len -= nwritten;
                nwritten = 0;
            } else {
                nwritten -= local_iov[current_iov_idx].iov_len;
                current_iov_idx++;
            }
        }
    }

    return CCASK_OK;
}

int safe_preadv(int fd, struct iovec *iov, int iovcnt, off_t offset) {
    if (fd < 0 || iov == NULL || iovcnt <= 0) {
        return CCASK_FAIL;
    }

    struct iovec local_iov[iovcnt];
    memcpy(local_iov, iov, iovcnt * sizeof(struct iovec)); // shallow-copy

    int current_iov_idx = 0;
    ssize_t total_read = 0;

    while (current_iov_idx < iovcnt) {
        ssize_t nread = preadv(fd, &local_iov[current_iov_idx], iovcnt - current_iov_idx, offset + total_read);

        if (nread < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            ccask_errno = CCASK_ERR_READ_FAILED;
            return CCASK_FAIL;
        }
        
        if (nread == 0) {
            ccask_errno = CCASK_ERR_UNEXPECTED_EOF;
            return CCASK_FAIL;
        }

        total_read += nread;

        while (current_iov_idx < iovcnt && nread > 0) {
            if ((size_t)nread < local_iov[current_iov_idx].iov_len) {
                local_iov[current_iov_idx].iov_base = (char *)local_iov[current_iov_idx].iov_base + nread;
                local_iov[current_iov_idx].iov_len -= nread;
                nread = 0;
            } else {
                nread -= local_iov[current_iov_idx].iov_len;
                current_iov_idx++;
            }
        }
    }

    return CCASK_OK;
}

int safe_pread(int fd, void *buf, ssize_t len, off_t offset) {
    if (len < 0 || !buf) {
        return CCASK_FAIL;
    }

    ssize_t to_read = len;
    char *p = buf;
    off_t pos = offset;

    while (to_read > 0) {
        ssize_t n = pread(fd, p, to_read, pos);
        if (n < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) continue;
            ccask_errno = CCASK_ERR_READ_FAILED;
            return CCASK_FAIL;
        } else if (n == 0) {
            ccask_errno = CCASK_ERR_UNEXPECTED_EOF;
            return CCASK_FAIL;
        }

        p += n;
        pos += n;
        to_read -= n;
    }

    return CCASK_OK;
}
