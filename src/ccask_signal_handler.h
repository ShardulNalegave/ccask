
#ifndef CCASK_SIGNAL_HANDLER_H
#define CCASK_SIGNAL_HANDLER_H

#include "signal.h"

extern volatile sig_atomic_t ccask_shutdown_signal;

int ccask_signal_handler_init();

#endif