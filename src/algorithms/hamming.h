#ifndef HAMMING_H
#define HAMMING_H

#include <stdint.h>
#include <stddef.h>

int hamming_distance(const uint32_t *vec1, const uint32_t *vec2, uint16_t size);
double hamming_similarity(const uint32_t *vec1, const uint32_t *vec2, uint16_t size);

#endif // HAMMING_H

