#include "minhash.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// MurmurHash3 32-bit implementation
uint32_t murmurhash3_32(const void* key, int len, uint32_t seed) {
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;
    const int r1 = 15;
    const int r2 = 13;
    const uint32_t m = 5;
    const uint32_t n = 0xe6546b64;

    uint32_t hash = seed;
    const int nblocks = len / 4;
    const uint32_t *blocks = (const uint32_t *) key;
    int i;

    for (i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= c1;
        k = (k << r1) | (k >> (32 - r1));
        k *= c2;

        hash ^= k;
        hash = ((hash << r2) | (hash >> (32 - r2))) * m + n;
    }

    const uint8_t *tail = (const uint8_t*)(key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
            /* fall through */
        case 2:
            k1 ^= tail[1] << 8;
            /* fall through */
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << r1) | (k1 >> (32 - r1));
            k1 *= c2;
            hash ^= k1;
    }

    hash ^= len;
    hash ^= (hash >> 16);
    hash *= 0x85ebca6b;
    hash ^= (hash >> 13);
    hash *= 0xc2b2ae35;
    hash ^= (hash >> 16);

    return hash;
}

// Create a new MinHash object
MinHashError minhash_create(MinHash** mh, size_t num_hashes, uint32_t seed) {
    if (!mh || num_hashes == 0) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    *mh = (MinHash*)malloc(sizeof(MinHash));
    if (*mh == NULL) {
        return MINHASH_ERROR_MEMORY;
    }

    (*mh)->signature = (uint32_t*)malloc(num_hashes * sizeof(uint32_t));
    if ((*mh)->signature == NULL) {
        free(*mh);
        *mh = NULL;
        return MINHASH_ERROR_MEMORY;
    }

    for (size_t i = 0; i < num_hashes; i++) {
        (*mh)->signature[i] = UINT32_MAX;
    }

    (*mh)->num_hashes = num_hashes;
    (*mh)->seed = seed;

    return MINHASH_SUCCESS;
}

// Free memory associated with a MinHash object
void minhash_free(MinHash* mh) {
    if (mh) {
        free(mh->signature);
        free(mh);
    }
}

uint32_t minhash_hash(const void* data, size_t len, size_t index, uint32_t seed) {
    return murmurhash3_32(data, len, seed + index);
}

// Compute MinHash signature from token IDs
MinHashError minhash_compute_from_tokens(MinHash* mh, const uint32_t* token_ids, size_t token_count, size_t ngram_size) {
    if (!mh || !token_ids || token_count < ngram_size) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i <= token_count - ngram_size; i++) {
        for (size_t j = 0; j < mh->num_hashes; j++) {
            uint32_t hash = minhash_hash(&token_ids[i], ngram_size * sizeof(uint32_t), j, mh->seed);
            if (hash < mh->signature[j]) {
                mh->signature[j] = hash;
            }
        }
    }

    return MINHASH_SUCCESS;
}

// Merge two MinHash signatures
MinHashError minhash_merge(MinHash* dest, const MinHash* src) {
    if (!dest || !src || dest->num_hashes != src->num_hashes) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < dest->num_hashes; i++) {
        dest->signature[i] = (dest->signature[i] < src->signature[i]) ? dest->signature[i] : src->signature[i];
    }

    return MINHASH_SUCCESS;
}

// Clone a MinHash object
MinHashError minhash_clone(const MinHash* src, MinHash** dest) {
    if (!src || !dest) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    MinHashError err = minhash_create(dest, src->num_hashes, src->seed);
    if (err != MINHASH_SUCCESS) {
        return err;
    }

    memcpy((*dest)->signature, src->signature, src->num_hashes * sizeof(uint32_t));

    return MINHASH_SUCCESS;
}

