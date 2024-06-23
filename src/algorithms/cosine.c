#include "cosine.h"
#include <math.h>

float cosine_similarity(const float* vec1, const float* vec2, size_t size) {
    if (!vec1 || !vec2 || size == 0) {
        return 0.0f;
    }

    float dot_product = 0.0f;
    float magnitude1 = 0.0f;
    float magnitude2 = 0.0f;

    for (size_t i = 0; i < size; i++) {
        dot_product += vec1[i] * vec2[i];
        magnitude1 += vec1[i] * vec1[i];
        magnitude2 += vec2[i] * vec2[i];
    }

    magnitude1 = sqrtf(magnitude1);
    magnitude2 = sqrtf(magnitude2);

    if (magnitude1 == 0.0f || magnitude2 == 0.0f) {
        return 0.0f;
    }

    return dot_product / (magnitude1 * magnitude2);
}
