
#include "ccask/compactor.h"
#include "ccask/keydir.h"

int ccask_compact() {
    ccask_keydir_record_iter_t iter = ccask_keydir_record_iter();
    ccask_keydir_record_t *record;
    while ((record = ccask_keydir_record_iter_next(&iter)) != NULL) {
        //
    }
    ccask_keydir_record_iter_close(&iter);
}
