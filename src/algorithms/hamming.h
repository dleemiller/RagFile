#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>
#include <stddef.h>

int hammingDistance(uint8_t *vec1, uint8_t *vec2, size_t size);
double hammingSimilarity(uint8_t *vec1, uint8_t *vec2, size_t size);

#endif // HAMMING_H

