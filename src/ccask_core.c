
#include "ccask_core.h"

#include "unistd.h"
#include "ccask_keydir.h"

static ccask_state_t state;

void ccask_init() {
    state.data_dir = "~/.config/ccask/data"; // TODO: this should be fetched from a config file
    ccask_keydir_recover(&state);
}

void ccask_shutdown() {
    close(state.active_fd);
    ccask_keydir_free_all();
}