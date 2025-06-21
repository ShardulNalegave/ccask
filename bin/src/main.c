
#include "stdio.h"
#include "ccask/core.h"

int main() {
    ccask_options_t opts = {
        .data_dir = "../test_data"
    };
    ccask_init(opts);

    getchar();

    ccask_shutdown();
    return 0;
}
