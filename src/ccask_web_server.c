
#include "ccask_web_server.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "microhttpd.h"

#include "log.h"

static struct MHD_Daemon* ccask_web_server_daemon;

static enum MHD_Result handler(
    void* cls,
    struct MHD_Connection* connection,
    const char* url,
    const char* method,
    const char* version,
    const char* upload_data,
    size_t* upload_data_size,
    void** ptr
) {
    static int dummy;
    const char* page = cls;
    struct MHD_Response * response;
    int ret;

    if (0 != strcmp(method, "GET"))
        return MHD_NO; /* unexpected method */

    if (&dummy != *ptr) {
        /* The first time only the headers are valid,
         do not respond in the first round... */
        *ptr = &dummy;
        return MHD_YES;
    }
    if (0 != *upload_data_size)
        return MHD_NO; /* upload data in a GET!? */
    
    *ptr = NULL; /* clear context pointer */
    response = MHD_create_response_from_buffer(
        strlen(page),
        (void*) page,
        MHD_RESPMEM_PERSISTENT
    );
    
    ret = MHD_queue_response(
        connection,
        MHD_HTTP_OK,
        response
    );
    
    MHD_destroy_response(response);
    return ret;
}

void ccask_web_server_start() {
    ccask_web_server_daemon = MHD_start_daemon(
        MHD_USE_THREAD_PER_CONNECTION,
        5000,
        NULL,
        NULL,
        &handler,
        "Hello, World!",
        MHD_OPTION_END
    );

    if (ccask_web_server_daemon == NULL) {
        log_fatal("Could not start ccask web-server");
        exit(-1);
    }
}

void ccask_web_server_shutdown() {
    MHD_stop_daemon(ccask_web_server_daemon);
}