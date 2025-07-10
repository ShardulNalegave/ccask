
#ifndef CCASK_HINT_H
#define CCASK_HINT_H

#include "stdint.h"

#include "ccask/files.h"
#include "ccask/status.h"

void ccask_hintfile_generator_init(void);
void ccask_hintfile_generator_shutdown(void);
ccask_status_e ccask_hintfile_generate(ccask_file_t *file);

#endif
