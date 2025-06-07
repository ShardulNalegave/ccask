
#include "ccask_core.h"

#include "fcntl.h"
#include "unistd.h"
#include "dirent.h"
#include "log.h"

#include "ccask_files.h"
#include "ccask_keydir.h"

static ccask_state_t state;

int ccask_init() {
    state.data_dir = "./test_data"; // TODO: this should be fetched from a config file
    
    if (ccask_files_init(state.data_dir) != 0)
        return -1;

    ccask_keydir_recover(&state);
    return 0;
}

void ccask_shutdown() {
    ccask_files_destroy();
    ccask_keydir_free_all();
}