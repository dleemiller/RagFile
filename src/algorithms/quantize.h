#ifndef QUANTIZE_H
#define QUANTIZE_H

#include <stddef.h>
#include <stdint.h>

void compute_average_embedding(const float* flattened, size_t num_embeddings, size_t embedding_dim, float* average_embedding);
void quantize_and_pack(float* average_embedding, uint8_t* packed_bits);

#endif // QUANTIZE_H

