#ifndef QUANTIZE_H
#define QUANTIZE_H

#include <stddef.h>
#include <stdint.h>

void computeAverageEmbedding(float* flattened, size_t num_embeddings, size_t embedding_dim, float* average_embedding);
void quantizeAndPack(float* average_embedding, uint8_t* packed_bits);

#endif // QUANTIZE_H

