
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "unistd.h"

#include "log.h"

#include "ccask_signal_handler.h"
#include "ccask_web_server.h"

int main() {
    ccask_signal_handler_init();

    ccask_web_server_start();

    while (!ccask_shutdown_signal) {
        sleep(1);
    }

    log_info("Shutting down...");
    ccask_web_server_shutdown();

    return 0;
}