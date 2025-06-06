
#include "ccask_core.h"

#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "log.h"

#include "ccask_keydir.h"

static ccask_state_t state;

void ccask_init() {
    state.data_dir = "./test_data"; // TODO: this should be fetched from a config file
    if (ccask_files_load_directory(state.data_dir, state.data_files_head) < 0) {
        log_error("Something went wrong");
    }
    ccask_keydir_recover(&state);
}

void ccask_shutdown() {
    ccask_files_destroy(state.data_files_head);
    ccask_keydir_free_all();
}