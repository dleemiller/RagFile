#include "jaccard.h"

float jaccard_similarity(const uint64_t* mh1, const uint64_t* mh2) {
    if (!mh1 || !mh2) {
        return 0.0f;
    }

    size_t matches = 0;
    for (size_t i = 0; i < MINHASH_SIZE; i++) {
        if (mh1[i] == mh2[i]) {
            matches++;
        }
    }

    return (float)matches / MINHASH_SIZE;
}
