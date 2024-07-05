#include "hamming.h"
#include "../include/config.h"
#include <stdio.h> // for NULL definition

// Compute Hamming distance for arrays of uint8_t
int hamming_distance(const uint8_t *vec1, const uint8_t *vec2, size_t size) {
    if (vec1 == NULL || vec2 == NULL) {
        fprintf(stderr, "Null pointer passed to hamming_distance\n");
        return -1; // Error code for null pointers
    }
    
    int distance = 0;
    for (size_t i = 0; i < size; i++) {
        distance += __builtin_popcount(vec1[i] ^ vec2[i]);
    }
    return distance;
}

// Compute Hamming similarity for arrays of uint8_t
double hamming_similarity(const uint8_t *vec1, const uint8_t *vec2, size_t size) {
    if (vec1 == NULL || vec2 == NULL) {
        fprintf(stderr, "Null pointer passed to hamming_similarity\n");
        return -1.0; // Error code for null pointers
    }
    
    int distance = hamming_distance(vec1, vec2, size);
    if (distance == -1) { // Check if hamming_distance returned an error
        return -1.0; // Propagate the error code
    }

    int vector_dim = BINARY_EMBEDDING_DIM;
    return (double)(vector_dim - distance) / (double)vector_dim;
}

