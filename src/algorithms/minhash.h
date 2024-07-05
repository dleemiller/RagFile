#ifndef MINHASH_H
#define MINHASH_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    MINHASH_SUCCESS = 0,
    MINHASH_ERROR_MEMORY,
    MINHASH_ERROR_INVALID_ARGUMENT
} MinHashError;

typedef struct {
    uint32_t* signature;
    size_t num_hashes;
    uint32_t seed;
} MinHash;

MinHashError minhash_create(MinHash** mh, size_t num_hashes, uint32_t seed);
void minhash_free(MinHash* mh);
MinHashError minhash_compute_char(MinHash* mh, const char* text, size_t ngram_size);
MinHashError minhash_compute_word(MinHash* mh, const char* text, size_t ngram_size);
MinHashError minhash_compute_tokens(MinHash* mh, const uint32_t* token_ids, size_t token_count, size_t ngram_size);
MinHashError minhash_merge(MinHash* dest, const MinHash* src);

#endif // MINHASH_H

