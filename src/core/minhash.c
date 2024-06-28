#include "minhash.h"
#include <string.h>
#include <stdio.h>
#include <limits.h>

// MurmurHash3 implementation
uint64_t murmurhash3_64(const void* key, int len, uint32_t seed) {
    const uint64_t m = 0xc6a4a7935bd1e995ULL;
    const int r = 47;

    uint64_t h = seed ^ (len * m);

    const uint64_t * data = (const uint64_t *)key;
    const uint64_t * end = data + (len/8);

    while(data != end) {
        uint64_t k = *data++;

        k *= m; 
        k ^= k >> r; 
        k *= m; 
		
        h ^= k;
        h *= m; 
    }

    const unsigned char * data2 = (const unsigned char*)data;

    switch(len & 7) {
    case 7: h ^= ((uint64_t)data2[6]) << 48;
        /* fallthrough */
    case 6: h ^= ((uint64_t)data2[5]) << 40;
        /* fallthrough */
    case 5: h ^= ((uint64_t)data2[4]) << 32;
        /* fallthrough */
    case 4: h ^= ((uint64_t)data2[3]) << 24;
        /* fallthrough */
    case 3: h ^= ((uint64_t)data2[2]) << 16;
        /* fallthrough */
    case 2: h ^= ((uint64_t)data2[1]) << 8;
        /* fallthrough */
    case 1: h ^= ((uint64_t)data2[0]);
            h *= m;
    };
 
    h ^= h >> r;
    h *= m;
    h ^= h >> r;

    return h;
}

MinHashError minhash_create(MinHash** mh, size_t num_hashes, uint32_t seed) {
    if (!mh || num_hashes == 0) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    *mh = (MinHash*)malloc(sizeof(MinHash));
    if (*mh == NULL) {
        return MINHASH_ERROR_MEMORY;
    }

    (*mh)->signature = (uint64_t*)malloc(num_hashes * sizeof(uint64_t));
    if ((*mh)->signature == NULL) {
        free(*mh);
        *mh = NULL;
        return MINHASH_ERROR_MEMORY;
    }

    for (size_t i = 0; i < num_hashes; i++) {
        (*mh)->signature[i] = UINT64_MAX;
    }

    (*mh)->num_hashes = num_hashes;
    (*mh)->seed = seed;

    return MINHASH_SUCCESS;
}

void minhash_free(MinHash* mh) {
    if (mh) {
        free(mh->signature);
        free(mh);
    }
}

uint64_t minhash_hash(const void* data, size_t len, size_t index, uint32_t seed) {
    return murmurhash3_64(data, len, seed + index);
}

MinHashError minhash_compute_from_tokens(MinHash* mh, const uint32_t* token_ids, size_t token_count, size_t ngram_size) {
    if (!mh || !token_ids || token_count < ngram_size) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i <= token_count - ngram_size; i++) {
        for (size_t j = 0; j < mh->num_hashes; j++) {
            uint64_t hash = minhash_hash(&token_ids[i], ngram_size * sizeof(uint32_t), j, mh->seed);
            if (hash < mh->signature[j]) {
                mh->signature[j] = hash;
            }
        }
    }

    return MINHASH_SUCCESS;
}

MinHashError minhash_merge(MinHash* dest, const MinHash* src) {
    if (!dest || !src || dest->num_hashes != src->num_hashes) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    for (size_t i = 0; i < dest->num_hashes; i++) {
        dest->signature[i] = (dest->signature[i] < src->signature[i]) ? dest->signature[i] : src->signature[i];
    }

    return MINHASH_SUCCESS;
}

MinHashError minhash_clone(const MinHash* src, MinHash** dest) {
    if (!src || !dest) {
        return MINHASH_ERROR_INVALID_ARGUMENT;
    }

    MinHashError err = minhash_create(dest, src->num_hashes, src->seed);
    if (err != MINHASH_SUCCESS) {
        return err;
    }

    memcpy((*dest)->signature, src->signature, src->num_hashes * sizeof(uint64_t));

    return MINHASH_SUCCESS;
}

