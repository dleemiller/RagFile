#ifndef MINHASH_H
#define MINHASH_H

#include "../include/config.h"
#include <stdint.h>
#include <stdlib.h>


typedef enum {
    MINHASH_SUCCESS = 0,
    MINHASH_ERROR_MEMORY,
    MINHASH_ERROR_INVALID_ARGUMENT
} MinHashError;

typedef struct {
    uint64_t* signature;
    size_t num_hashes;
    uint32_t seed;
} MinHash;

/**
 * Create a new MinHash object.
 *
 * @param mh Pointer to a MinHash pointer where the new object will be stored.
 * @param num_hashes Number of hash functions to use in the MinHash signature.
 * @param seed Seed value for the hash functions.
 * @return MINHASH_SUCCESS on success, or an error code on failure.
 */
MinHashError minhash_create(MinHash** mh, size_t num_hashes, uint32_t seed);

/**
 * Free memory associated with a MinHash object.
 *
 * @param mh Pointer to the MinHash object to be freed.
 */
void minhash_free(MinHash* mh);

/**
 * Compute MinHash signature from token IDs.
 *
 * @param mh Pointer to the MinHash object.
 * @param token_ids Array of token IDs.
 * @param token_count Number of tokens in the array.
 * @param ngram_size Size of n-grams to use for hashing.
 * @return MINHASH_SUCCESS on success, or an error code on failure.
 */
MinHashError minhash_compute_from_tokens(MinHash* mh, const uint32_t* token_ids, size_t token_count, size_t ngram_size);

/**
 * Merge two MinHash signatures.
 *
 * @param dest Destination MinHash object.
 * @param src Source MinHash object to merge into dest.
 * @return MINHASH_SUCCESS on success, or an error code on failure.
 */
MinHashError minhash_merge(MinHash* dest, const MinHash* src);

/**
 * Clone a MinHash object.
 *
 * @param src Source MinHash object to clone.
 * @param dest Pointer to a MinHash pointer where the cloned object will be stored.
 * @return MINHASH_SUCCESS on success, or an error code on failure.
 */
MinHashError minhash_clone(const MinHash* src, MinHash** dest);

/**
 * Compute a hash value for a given data and index.
 *
 * @param data Pointer to the data to hash.
 * @param len Length of the data in bytes.
 * @param index Index of the hash function.
 * @param seed Seed value for the hash function.
 * @return The computed hash value.
 */
uint64_t minhash_hash(const void* data, size_t len, size_t index, uint32_t seed);

#endif // MINHASH_H
