/**
 * Copyright (C) 2025  Shardul Nalegave
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the Lesser GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * Lesser GNU General Public License for more details.
 * 
 * You should have received a copy of the Lesser GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 * 
 */

#include "ccask/compactor.h"

#include "unistd.h"
#include "inttypes.h"
#include "ccask/reader.h"
#include "ccask/keydir.h"
#include "ccask/files.h"
#include "ccask/records.h"
#include "ccask/utils.h"
#include "ccask/log.h"

ccask_status_e ccask_compactor_dump_keydir() {
    uint64_t curr_temp_id = 0;
    int temp_file_fd;
    CCASK_ATTEMPT(5, temp_file_fd, ccask_files_get_temp_datafile_fd(curr_temp_id));
    if (temp_file_fd < 0) {
        log_error("Couldn't get FD for %" PRIu64 ".data.tmp", curr_temp_id);
        return CCASK_FAIL;
    }

    ccask_keydir_record_iter_t iter = ccask_keydir_record_iter();
    ccask_keydir_record_t *kd_record;
    while ((kd_record = ccask_keydir_record_iter_next(&iter)) != NULL) {
        ccask_datafile_record_t df_record;
        if (ccask_allocate_datafile_record(df_record, kd_record->key_size, kd_record->value_size) != CCASK_OK)
            return CCASK_FAIL;
        
        int res;
        CCASK_ATTEMPT(5, res, ccask_read_datafile_record(kd_record->file_id, df_record, kd_record->record_pos));
        if (res != CCASK_OK) {
            log_error("Failed to read datafile record, cancelling compaction");
            close(temp_file_fd);
            goto cancel_compaction;
        }

        off_t pos = lseek(temp_file_fd, 0, SEEK_END);
        if (pos < 0) {
            log_error("lseek failed during write record to temp-datafile ID = %" PRIu64, temp_file_fd);
            close(temp_file_fd);
            goto cancel_compaction;
        }

        size_t record_size = ccask_get_datafile_record_total_size(df_record);
        if ((size_t)pos + record_size > MAX_ACTIVE_FILE_SIZE) {
            close(temp_file_fd);
            curr_temp_id++;
            CCASK_ATTEMPT(5, temp_file_fd, ccask_files_get_temp_datafile_fd(curr_temp_id));
            if (temp_file_fd < 0) {
                log_error("Couldn't get FD for %" PRIu64 ".data.tmp, cancelling compaction", curr_temp_id);
                goto cancel_compaction;
            }
        }

        if (safe_writev(temp_file_fd, df_record, 3) != CCASK_OK) {
            log_error("Failed to write datafile record, cancelling compaction");
            close(temp_file_fd);
            goto cancel_compaction;
        }

        free_datafile_record(df_record);
    }

    close(temp_file_fd);
    ccask_keydir_record_iter_close(&iter);

    ccask_status_e delete_res = CCASK_OK;
    ccask_file_t *curr_file = ccask_files_get_oldest_file();
    while (curr_file != NULL) {
        int res;

        CCASK_ATTEMPT(5, res, ccask_files_delete(curr_file->file_id, FILE_DATA));
        if (res != CCASK_OK) {
            log_error("Couldn't delete existing datafile ID = %" PRIu64 " during compaction. Please replace all existing datafiles and hintfiles with temp (compacted) ones.", curr_file->file_id);
            delete_res = CCASK_FAIL;
        }

        if (curr_file->has_hint) {
            CCASK_ATTEMPT(5, res, ccask_files_delete(curr_file->file_id, FILE_HINT));
            if (res != CCASK_OK) {
                log_error("Couldn't delete existing hintfile ID = %" PRIu64 " during compaction. Please replace all existing datafiles and hintfiles with temp (compacted) ones.", curr_file->file_id);
                delete_res = CCASK_FAIL;
            }
        }

        curr_file = curr_file->previous;
    }

    if (delete_res != CCASK_OK) return delete_res;

    for (int i = 0; i <= curr_temp_id; i++) {
        int res;
        CCASK_ATTEMPT(5, res, ccask_files_change_ext(i, FILE_TEMP_DATA, FILE_DATA));

        if (res != CCASK_OK) {
            log_error("Couldn't rename compacted datafiles (temp extension) to actual datafiles. Please do so manually");
        }
    }

    return CCASK_OK;

cancel_compaction:
    for (int i = curr_temp_id; i >= 0; i--) {
        int res;
        CCASK_ATTEMPT(5, res, ccask_files_delete(i, FILE_TEMP_DATA));
        if (res != CCASK_OK) {
            log_error("Couldn't delete temporary datafile ID = %" PRIu64 " when cancelling compaction. Please delete it yourself.", i);
        }
    }
    return CCASK_FAIL;
}
