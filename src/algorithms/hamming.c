#include "hamming.h"
#include <stdio.h> // for NULL definition

// Compute Hamming distance for arrays of uint8_t
int hammingDistance(uint8_t *vec1, uint8_t *vec2, size_t size) {
    if (vec1 == NULL || vec2 == NULL) {
        fprintf(stderr, "Null pointer passed to hammingDistance\n");
        return -1; // Error code for null pointers
    }
    
    int distance = 0;
    for (size_t i = 0; i < size; i++) {
        distance += __builtin_popcount(vec1[i] ^ vec2[i]);
    }
    return distance;
}

// Compute Hamming similarity for arrays of uint8_t
double hammingSimilarity(uint8_t *vec1, uint8_t *vec2, size_t size) {
    if (vec1 == NULL || vec2 == NULL) {
        fprintf(stderr, "Null pointer passed to hammingSimilarity\n");
        return -1.0; // Error code for null pointers
    }
    
    int distance = hammingDistance(vec1, vec2, size);
    if (distance == -1) { // Check if hammingDistance returned an error
        return -1.0; // Propagate the error code
    }
    return 1.0 - ((double)distance / (size * 8));
}

