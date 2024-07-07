#ifndef JACCARD_H
#define JACCARD_H

#include <stddef.h>
#include <stdint.h>

/**
 * Compute Jaccard similarity between two MinHash signatures.
 *
 * @param mh1 Pointer to the first MinHash signature.
 * @param mh2 Pointer to the second MinHash signature.
 * @param vector_dim Dimension of the vectors.
 * @return The computed Jaccard similarity (between 0 and 1).
 */
float jaccard_similarity(const uint32_t* mh1, const uint32_t* mh2, uint16_t vector_dim);

#endif // JACCARD_H

