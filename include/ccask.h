
#ifndef CCASK_H
#define CCASK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stddef.h"
#include "stdint.h"

#include "ccask/status.h"

/**
 * Configuration options that can be passed to `ccask`
 */
typedef struct ccask_options {
    char* data_dir;                         /* Directory where all datafiles are stored */
    size_t writer_ringbuf_capacity;         /* Capacity of the Writer Ring-Buffer */

    /**
     * Size after which datafiles must be rotated.
     * Note: This doesn't have any effect on existing datafiles.
     */
    size_t datafile_rotate_threshold;
} ccask_options_t;

/**
 * Initialises `ccask` by staring all required sub-systems.
 * @param opts options passed to `ccask`
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_init(ccask_options_t opts);

/**
 * Shuts down `ccask` by gracefully stopping all sub-systems.
 */
void ccask_shutdown(void);

typedef struct ccask_record {
    uint32_t timestamp;
    uint32_t key_size;
    uint32_t value_size;
    const void *value;
} ccask_record_t;

/**
 * Fetch the record corresponding to the provided key
 * 
 * @param key The pointer to key whose value to search for
 * @param key_size The size of key
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_get(void *key, uint32_t key_size, ccask_record_t *record);

/**
 * Store a new record with the provided key and value by pushing it to the Writer-Ringbuffer.
 * Note: This is a non-blocking operation.
 * 
 * 
 * @param key Pointer to key
 * @param key_size Size of key
 * @param value Pointer to value
 * @param value_size Size of value
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_put(void* key, uint32_t key_size, void* value, uint32_t value_size);

/**
 * Store a new record with the provided key and value by immediately writing to the active datafile.
 * Note: This is a blocking operation.
 * 
 * 
 * @param key Pointer to key
 * @param key_size Size of key
 * @param value Pointer to value
 * @param value_size Size of value
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_put_blocking(void *key, uint32_t key_size, void *value, uint32_t value_size);

/**
 * Delete a stored Key-Value pair
 * This is done by adding a tombstone record for the key.
 * Note: This is a non-blocking operation.
 * 
 * @param key Pointer to key
 * @param key_size Size of key
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_delete(void *key, uint32_t key_size);

/**
 * Delete a stored Key-Value pair
 * This is done by adding a tombstone record for the key.
 * Note: This is a blocking operation.
 * 
 * @param key Pointer to key
 * @param key_size Size of key
 * @return CCASK_OK if successful, else the error code
 */
ccask_status_e ccask_delete_blocking(void *key, uint32_t key_size);

// Opaque Forward-declaration
typedef struct ccask_keys_iter ccask_keys_iter_t;

/**
 * Get a iterator for currently stored keys.
 * This acquires a read-lock over the key-directory. Thus there can be no write operations run while the iter is alive.
 * @return Iterator instance for currently stored keys, acquires a read-lock on key-directory.
 */
ccask_keys_iter_t* ccask_list_keys(void);

/**
 * Get the next key from the provided key iterator
 * @return CCASK_OK if next exists, CCASK_ERR_ITER_END if at end
 */
ccask_status_e ccask_keys_iter_next(ccask_keys_iter_t *iter, void **key, uint32_t key_size);

/**
 * Close the keys iterator and release the read-lock.
 */
void ccask_keys_iter_close(ccask_keys_iter_t *iter);

#ifdef __cplusplus
}
#endif

#endif
