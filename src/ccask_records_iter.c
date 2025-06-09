
#include "ccask_records_iter.h"

#include "log.h"

int ccask_datafile_record_iter_open(ccask_file_t* file, ccask_datafile_iter_t** iter) {
    uint8_t* buffer;
    uint64_t buffer_size = ccask_files_read_entire_datafile(file, &buffer);
    if (buffer_size < 0) {
        *iter = NULL;
        return -1;
    }

    *iter = malloc(sizeof(ccask_datafile_iter_t));
    (*iter)->file_id = file->id;
    (*iter)->offset = 0;
    (*iter)->buffer = buffer;
    (*iter)->buffer_size = buffer_size;
    return 0;
}

void ccask_datafile_record_iter_close(ccask_datafile_iter_t* iter) {
    free(iter->buffer);
    free(iter);
}

int ccask_datafile_record_iter_next(ccask_datafile_iter_t* iter, ccask_datafile_record_t** record) {
    if (iter->offset >= iter->buffer_size) {
        *record = NULL;
        return 0;
    }
    
    uint64_t read = ccask_datafile_record_deserialize(iter->buffer + iter->offset, record);
    iter->offset += read;
    return 0;
}

int ccask_hintfile_record_iter_open(ccask_file_t* file, ccask_hintfile_iter_t** iter) {
    uint8_t* buffer;
    uint64_t buffer_size = ccask_files_read_entire_hintfile(file, &buffer);
    if (buffer_size < 0) {
        *iter = NULL;
        return -1;
    }

    *iter = malloc(sizeof(ccask_hintfile_iter_t));
    (*iter)->file_id = file->id;
    (*iter)->offset = 0;
    (*iter)->buffer = buffer;
    (*iter)->buffer_size = buffer_size;
    return 0;
}

void ccask_hintfile_record_iter_close(ccask_hintfile_iter_t* iter) {
    free(iter->buffer);
    free(iter);
}

int ccask_hintfile_record_iter_next(ccask_hintfile_iter_t* iter, ccask_hintfile_record_t** record) {
    if (iter->offset >= iter->buffer_size) {
        *record = NULL;
        return 0;
    }
    
    uint64_t read = ccask_hintfile_record_deserialize(iter->buffer + iter->offset, record);
    iter->offset += read;
    return 0;
}