
#ifndef CCASK_SIGNAL_HANDLER_H
#define CCASK_SIGNAL_HANDLER_H

#include "signal.h"

extern volatile sig_atomic_t ccask_shutdown_signal;

void ccask_signal_handler_init();

#endif