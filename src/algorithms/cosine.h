#ifndef COSINE_H
#define COSINE_H

#include <stddef.h>

/**
 * Compute cosine similarity between two vectors.
 *
 * @param vec1 Pointer to the first vector.
 * @param vec2 Pointer to the second vector.
 * @param size Size of the vectors.
 * @return The computed cosine similarity (between -1 and 1).
 */
float cosine_similarity(const float* vec1, const float* vec2, size_t size);

#endif // COSINE_H
