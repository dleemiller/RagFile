#include "hamming.h"

// Compute Hamming distance for arrays of uint8_t
int hammingDistance(uint8_t *vec1, uint8_t *vec2, size_t size) {
    int distance = 0;
    for (size_t i = 0; i < size; i++) {
        distance += __builtin_popcount(vec1[i] ^ vec2[i]);
    }
    return distance;
}

// Compute Hamming similarity for arrays of uint8_t
double hammingSimilarity(uint8_t *vec1, uint8_t *vec2, size_t size) {
    int distance = hammingDistance(vec1, vec2, size);
    return 1.0 - ((double)distance / (size * 8));
}

