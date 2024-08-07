#include <stdio.h>
#include <math.h>
#include "../src/algorithms/hamming.h"

void testHammingDistance() {
    uint8_t vec1[] = {0b11010010, 0b01101001};
    uint8_t vec2[] = {0b11011000, 0b01101101};
    size_t size = sizeof(vec1) / sizeof(vec1[0]);

    int expected = 3; // Manually count the differing bits
    int result = hamming_distance(vec1, vec2, size);
    if (result == expected) {
        printf("Test passed. Hamming Distance: %d\n", result);
    } else {
        printf("Test failed. Expected %d, got %d\n", expected, result);
    }
}

void testHammingSimilarity() {
    uint8_t vec1[] = {0b11010010, 0b01101001};
    uint8_t vec2[] = {0b11011000, 0b01101101};
    size_t size = sizeof(vec1) / sizeof(vec1[0]);

    double expected = 0.81250; // 1 - (3 / (2 * 8))
    double result = hamming_similarity(vec1, vec2, size);
    if (result == expected) {
        printf("Test passed. Hamming Similarity: %.5f\n", result);
    } else {
        printf("Test failed. Expected %.5f, got %.5f\n", expected, result);
    }
}

int testHammingSimilarityExt() {
    uint8_t query[2] = {180, 232};
    uint8_t docs[4][2] = {
        {129, 105},
        {145, 107},
        {77, 195},
        {235, 230}
    };
    double expected[4] = {0.625000, 0.625000, 0.375000, 0.437500};
    double similarity;
    int success = 1;

    for (int i = 0; i < 4; i++) {
        similarity = hamming_similarity(query, docs[i], sizeof(query));
        if (fabs(similarity - expected[i]) > 0.01) {
            printf("Test failed for doc %d: Expected %.6f, got %.6f\n", i+1, expected[i], similarity);
            success = 0;
        }
    }

    if (success) {
        printf("All tests passed successfully.\n");
    }

    return !success; // Return 0 on success, 1 on failure
}

int main() {
    testHammingDistance(); // Existing function call
    testHammingSimilarity(); // New function call
    testHammingSimilarityExt();
    return 0;
}
