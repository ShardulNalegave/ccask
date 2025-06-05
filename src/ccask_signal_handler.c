
#include "ccask_signal_handler.h"

#include "stdlib.h"
#include "log.h"

volatile sig_atomic_t ccask_shutdown_signal = 0;

static void ccask_signal_handler(int sig) {
    ccask_shutdown_signal = 1;
}

void ccask_signal_handler_init() {
    if (signal(SIGINT, ccask_signal_handler) == SIG_ERR) {
        log_fatal("Could not register handler for SIGINT");
        exit(-1);
    }

    if (signal(SIGTERM, ccask_signal_handler) == SIG_ERR) {
        log_fatal("Could not register handler for SIGTERM");
        exit(-1);
    }
}