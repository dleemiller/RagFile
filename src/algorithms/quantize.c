#include "quantize.h"
#include "../include/config.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void compute_average_embedding(const float* flattened, size_t num_embeddings, size_t embedding_dim, float* average_embedding) {
    assert(embedding_dim % 8 == 0); // Ensure dimension is divisible by 8
    assert(BINARY_EMBEDDING_DIM <= embedding_dim); // Ensure binary dimension does not exceed total dimension

    size_t effective_dim = BINARY_EMBEDDING_DIM; // Use binary dimension from config
    if (effective_dim > embedding_dim) {
        fprintf(stderr, "Error: Effective dimension exceeds embedding dimension.\n");
        return;
    }

    memset(average_embedding, 0, effective_dim * sizeof(float));
    for (size_t i = 0; i < num_embeddings; i++) {
        for (size_t j = 0; j < effective_dim; j++) { // Only iterate up to the truncated dimension
            average_embedding[j] += flattened[i * embedding_dim + j];
        }
    }
    for (size_t j = 0; j < effective_dim; j++) {
        average_embedding[j] /= num_embeddings;
    }
}

void quantize_and_pack(float* average_embedding, uint8_t* packed_bits) {
    memset(packed_bits, 0, BINARY_EMBEDDING_BYTE_DIM);
    for (size_t i = 0; i < BINARY_EMBEDDING_DIM; i++) {
        if (average_embedding[i] > 0) {
            size_t byte_index = i / 8;
            size_t bit_index = i % 8;
            packed_bits[byte_index] |= (1 << bit_index);
        }
    }
}
