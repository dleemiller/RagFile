#include "../src/algorithms/quantize.h"
#include <stdio.h>
#include <math.h> // Include for fabs function

#define EPSILON 0.01 // Tolerance for floating-point comparison

int main() {
    float embeddings[7][16] = {
        {-0.7, -0.1, 0.3, -0.4, 0.5, -0.6, 0.7, -0.8, 0.9, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {-0.6, -0.2, 0.9, -0.4, 0.5, -0.6, 0.7, -0.4, 0.6, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {-0.5, -0.3, 0.3, -0.4, 0.5, -0.6, 0.7, -0.4, 0.6, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {0.4, -0.4, 0.9, -0.4, 0.5, -0.6, 0.7, -0.4, 0.9, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {0.3, -0.5, 0.3, -0.4, 0.5, -0.6, 0.7, -0.4, 0.6, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {0.2, -0.6, 0.9, -0.4, 0.5, -0.6, 0.7, -0.8, 0.6, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
        {0.1, -0.7, 0.3, -0.4, 0.5, -0.6, 0.7, -0.8, 0.9, -1.0, 1.1, -1.2, 1.3, -1.4, 1.5, -1.6},
    };
    size_t num_embeddings = 7, embedding_dim = 16;
    float average_embedding[16];
    uint8_t packed_bits[2]; // As 16 bits fit into 2 uint8s

    float expected_average[16] = {-0.11, -0.40, 0.56, -0.40, 0.50, -0.60, 0.70, -0.57, 0.73, -1.00, 1.10, -1.20, 1.30, -1.40, 1.50, -1.60};
    uint8_t expected_bits[2] = {84, 85}; // Based on the provided output

    computeAverageEmbedding(&embeddings[0][0], num_embeddings, embedding_dim, average_embedding);
    quantizeAndPack(average_embedding, packed_bits);

    int success = 1;

    // Check average embeddings
    for (size_t j = 0; j < embedding_dim; j++) {
        if (fabs(average_embedding[j] - expected_average[j]) > EPSILON) {
            printf("Average embedding mismatch at index %zu: Expected %.2f, got %.2f\n", j, expected_average[j], average_embedding[j]);
            success = 0;
        }
    }

    // Check packed bits
    for (size_t i = 0; i < sizeof(packed_bits); i++) {
        if (packed_bits[i] != expected_bits[i]) {
            success = 0;
            printf("Packed bits mismatch at index %zu: Expected %d, got %d\n", i, expected_bits[i], packed_bits[i]);
        }
    }

    if (success) {
        printf("All tests passed successfully.\n");
    } else {
        printf("Some tests failed.\n");
    }

    return success ? 0 : 1;
}

