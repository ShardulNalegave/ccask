
#ifndef CCASK_ERRORS_H
#define CCASK_ERRORS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

typedef enum ccask_status {
    /* Success */
    CCASK_OK = 0,

    /* Errors (Retryable) */

    /* Errors */
    CCASK_ERR_CRC_INVALID               = -51,
    CCASK_ERR_NO_KEY                    = -52,
    CCASK_ERR_ITER_END                  = -53,
} ccask_status_e;

#ifdef __cplusplus
}
#endif

#endif
