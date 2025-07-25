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

#ifndef CCASK_STATUS_H
#define CCASK_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef enum ccask_error {
    CCASK_ERR_UNKNOWN                     = 0,
    CCASK_ERR_NO_MEMORY                   = 1,
    CCASK_ERR_CRC_INVALID                 = 2,
    CCASK_ERR_NO_KEY                      = 3,
    CCASK_ERR_ITER_END                    = 4,
    CCASK_ERR_GET_FD_FAILED               = 5,
    CCASK_ERR_MAX_FDS_OPENED              = 6,
    CCASK_ERR_NO_SUCH_DATAFILE            = 7,
    CCASK_ERR_READ_FAILED                 = 8,
    CCASK_ERR_WRITE_FAILED                = 9,
    CCASK_ERR_COULDNT_START_THREAD        = 10,
    CCASK_ERR_UNEXPECTED_EOF              = 11,
    CCASK_ERR_RINGBUFFER_FULL             = 12,
} ccask_error_e;

typedef enum ccask_status {
    CCASK_OK = 0,
    CCASK_FAIL = -1,
    CCASK_RETRY = -2,
} ccask_status_e;

#define CCASK_ATTEMPT(max_attempts, out_var, expr)              \
    do {                                                        \
        int _ccask_retry_cnt = 0;                               \
        for (; _ccask_retry_cnt < (max_attempts);               \
            ++_ccask_retry_cnt)                                 \
        {                                                       \
        (out_var) = (expr);                                     \
        if (out_var != CCASK_RETRY)                             \
            break;                                              \
        }                                                       \
    } while (0)


extern ccask_error_e ccask_errno;

#ifdef __cplusplus
}
#endif

#endif
