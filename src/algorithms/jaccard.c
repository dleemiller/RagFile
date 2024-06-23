#include "jaccard.h"

float jaccard_similarity(const MinHash* mh1, const MinHash* mh2) {
    if (!mh1 || !mh2 || mh1->num_hashes != mh2->num_hashes) {
        return 0.0f;
    }

    size_t matches = 0;
    for (size_t i = 0; i < mh1->num_hashes; i++) {
        if (mh1->signature[i] == mh2->signature[i]) {
            matches++;
        }
    }

    return (float)matches / mh1->num_hashes;
}
