
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"
#include "unistd.h"
#include "log.h"

#include "ccask_core.h"
#include "ccask_web_server.h"
#include "ccask_signal_handler.h"

int main() {
    if (ccask_signal_handler_init() != 0)
        return -1;

    if (ccask_init() != 0)
        goto cleanup;

    if (ccask_web_server_start() != 0)
        goto cleanup;

    while (!ccask_shutdown_signal) {
        sleep(1);
    }

cleanup:
    log_info("Shutting down...");
    ccask_web_server_shutdown();
    ccask_shutdown();

    return 0;
}