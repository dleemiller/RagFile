#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>
#include <stddef.h>

int hamming_distance(const uint8_t *vec1, const uint8_t *vec2, size_t size);
double hamming_similarity(const uint8_t *vec1, const uint8_t *vec2, size_t size);

#endif // HAMMING_H

