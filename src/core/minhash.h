#ifndef MINHASH_H
#define MINHASH_H

#include <stdint.h>
#include <stdlib.h>

#define MINHASH_DEFAULT_SIZE 256

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

MinHashError minhash_create(MinHash** mh, size_t num_hashes, uint32_t seed);
void minhash_free(MinHash* mh);
MinHashError minhash_compute_from_tokens(MinHash* mh, const uint32_t* token_ids, size_t token_count, size_t ngram_size);
MinHashError minhash_merge(MinHash* dest, const MinHash* src);
MinHashError minhash_clone(const MinHash* src, MinHash** dest);

uint64_t minhash_hash(const void* data, size_t len, size_t index, uint32_t seed);

#endif // MINHASH_H
