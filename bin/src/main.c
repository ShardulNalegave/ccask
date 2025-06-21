
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "ccask/core.h"

int main() {
    ccask_options_t opts = {
        .data_dir = "../test_data"
    };
    ccask_init(opts);

    char *key1 = "key1";
    char *key2 = "key2";
    char *key3 = "key3";
    char *key4 = "key4";
    char *key5 = "key5";

    char *value1 = "value1";
    char *value2 = "value2";
    char *value3 = "value3";
    char *value4 = "value4";
    char *value5 = "value5";
    
    ccask_put(key1, 5, value1, 7);
    ccask_put(key2, 5, value2, 7);
    ccask_put(key3, 5, value3, 7);
    ccask_put(key4, 5, value4, 7);
    ccask_put(key5, 5, value5, 7);
    sleep(1);

    int res;
    char *get_val;

    res = ccask_get(key3, 5, (void**)&get_val);
    if (res >= 0) {
        printf("%d - %s\n", res, get_val);
        free(get_val);
    }

    res = ccask_get(key5, 5, (void**)&get_val);
    if (res >= 0) {
        printf("%d - %s\n", res, get_val);
        free(get_val);
    }

    res = ccask_get(key1, 5, (void**)&get_val);
    if (res >= 0) {
        printf("%d - %s\n", res, get_val);
        free(get_val);
    }

    getchar();

    ccask_shutdown();
    return 0;
}
