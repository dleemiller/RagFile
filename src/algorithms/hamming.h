#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>
#include <stddef.h>

int hamming_distance(uint8_t *vec1, uint8_t *vec2, size_t size);
double hamming_similarity(uint8_t *vec1, uint8_t *vec2, size_t size);

#endif // HAMMING_H

