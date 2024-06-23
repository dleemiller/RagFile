#ifndef JACCARD_H
#define JACCARD_H

#include "../include/config.h"
#include <stddef.h>
#include <stdint.h>

/**
 * Compute Jaccard similarity between two MinHash signatures.
 *
 * @param mh1 Pointer to the first MinHash signature.
 * @param mh2 Pointer to the second MinHash signature.
 * @param size Size of the MinHash signatures.
 * @return The computed Jaccard similarity (between 0 and 1).
 */
float jaccard_similarity(const uint64_t* mh1, const uint64_t* mh2);

#endif // JACCARD_H
