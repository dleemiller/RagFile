#include <stdio.h>
#include <math.h>
#include "../src/algorithms/hamming.h"

void testHammingDistance() {
    uint32_t vec1[] = {0b11010010011010010000000000000000, 0b01101001000000000000000000000000}; // 2 x 16-bit combined to 32-bit
    uint32_t vec2[] = {0b11011000000000000000000000000000, 0b01101101000000000000000000000000}; // 2 x 16-bit combined to 32-bit
    uint16_t size = sizeof(vec1) / sizeof(vec1[0]);

    int expected = 7; // Manually count the differing bits
    int result = hamming_distance(vec1, vec2, size);
    if (result == expected) {
        printf("Test passed. Hamming Distance: %d\n", result);
    } else {
        printf("Test failed. Expected %d, got %d\n", expected, result);
    }
}

void testHammingSimilarity() {
    uint32_t vec1[] = {0b11010010011010010000000000000000, 0b01101001000000000000000000000000}; // 2 x 16-bit combined to 32-bit
    uint32_t vec2[] = {0b11011000000000000000000000000000, 0b01101101000000000000000000000000}; // 2 x 16-bit combined to 32-bit
    uint16_t size = sizeof(vec1) / sizeof(vec1[0]);

    double expected = 1.0 - (7.0 / (2.0 * 32.0));
    double result = hamming_similarity(vec1, vec2, size);
    if (fabs(result - expected) < 0.00001) { // Allow small floating-point errors
        printf("Test passed. Hamming Similarity: %.5f\n", result);
    } else {
        printf("Test failed. Expected %.5f, got %.5f\n", expected, result);
    }
}

void testHammingSimilarityExt() {
    uint32_t query[] = {0b10110100111010000000000000000000, 0b00000000000000000000000000000000}; // 2 x 16-bit combined to 32-bit
    uint32_t docs[][2] = {
        {0b10000001101101010000000000000000, 0b11111111111111111111111111111111},
        {0b01001101100000110000000000000000, 0b11111111111110000000000000000000},
        {0b10010001011010110000000000000000, 0b00000000000000011111111111111111},
        {0b11101011111001100000000000000000, 0b00000000000000000000000000000000}
    };
    double expected[] = {0.359375, 0.625000, 0.640625, 0.859375};
    double similarity;
    int success = 1;

    for (int i = 0; i < 4; i++) {
        similarity = hamming_similarity(query, docs[i], sizeof(query) / sizeof(query[0]));
        if (fabs(similarity - expected[i]) > 0.01) {
            printf("Test failed for doc %d: Expected %.6f, got %.6f\n", i + 1, expected[i], similarity);
            success = 0;
        }
    }

    if (success) {
        printf("All extended similarity tests passed.\n");
    }
}

int main() {
    testHammingDistance();
    testHammingSimilarity();
    testHammingSimilarityExt();
    return 0;
}

