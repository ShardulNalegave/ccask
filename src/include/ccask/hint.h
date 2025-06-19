
#ifndef CCASK_HINT_H
#define CCASK_HINT_H

#include "stdint.h"

void ccask_hintfile_generator_init(void);
void ccask_hintfile_generator_shutdown(void);
int ccask_hintfile_generate(uint64_t file_id);

#endif
