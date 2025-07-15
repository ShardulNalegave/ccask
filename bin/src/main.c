
#include "stdlib.h"
#include "stdio.h"
#include "unistd.h"
#include "ccask.h"

int main() {
    ccask_options_t opts;
    opts.data_dir = "../test_data";
    opts.writer_ringbuf_capacity = 10;
    opts.datafile_rotate_threshold = 60;
    if (ccask_init(opts) != 0) {
        return -1;
    }

    ccask_keys_iter_t *iter = ccask_list_keys();
    void *key; uint32_t key_size;
    while (ccask_keys_iter_next(iter, &key, &key_size) == CCASK_OK) {
        printf("KEY: %s\n", key);
    }
    ccask_keys_iter_close(iter);

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
    ccask_put_blocking(key3, 5, value3, 7);
    ccask_put(key4, 5, value4, 7);
    ccask_put(key5, 5, value5, 7);
    sleep(1);

    int res;
    ccask_record_t record;

    res = ccask_get(key1, 5, &record);
    if (res == CCASK_OK) {
        printf("%s\n", record.value);
        ccask_free_record(record);
    }

    res = ccask_get(key2, 5, &record);
    if (res == CCASK_OK) {
        printf("%s\n", record.value);
        ccask_free_record(record);
    }

    res = ccask_get(key3, 5, &record);
    if (res == CCASK_OK) {
        printf("%s\n", record.value);
        ccask_free_record(record);
    }

    res = ccask_get(key4, 5, &record);
    if (res == CCASK_OK) {
        printf("%s\n", record.value);
        ccask_free_record(record);
    }

    res = ccask_get(key5, 5, &record);
    if (res == CCASK_OK) {
        printf("%s\n", record.value);
        ccask_free_record(record);
    }

    ccask_shutdown();
    return 0;
}
