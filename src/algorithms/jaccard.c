#include "jaccard.h"

float jaccard_similarity(const uint32_t* mh1, const uint32_t* mh2, uint16_t vector_dim) {
    if (!mh1 || !mh2 || vector_dim == 0) {
        return 0.0f;
    }

    size_t matches = 0;
    for (size_t i = 0; i < vector_dim; i++) {
        if (mh1[i] == mh2[i]) {
            matches++;
        }
    }

    return (float)matches / vector_dim;
}

