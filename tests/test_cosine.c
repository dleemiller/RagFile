#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../src/algorithms/cosine.h"

#define EPSILON 1e-6

void test_cosine_similarity() {
    float vec1[] = {1.0f, 2.0f, 3.0f};
    float vec2[] = {2.0f, 4.0f, 6.0f};
    float vec3[] = {-1.0f, -2.0f, -3.0f};
    float vec4[] = {0.0f, 0.0f, 0.0f};

    float similarity1 = cosine_similarity(vec1, vec2, 3);
    float similarity2 = cosine_similarity(vec1, vec3, 3);
    float similarity3 = cosine_similarity(vec1, vec4, 3);
    float similarity4 = cosine_similarity(vec1, vec1, 3);

    printf("Cosine similarity between vec1 and vec2: %f\n", similarity1);
    printf("Cosine similarity between vec1 and vec3: %f\n", similarity2);
    printf("Cosine similarity between vec1 and vec4: %f\n", similarity3);
    printf("Cosine similarity between vec1 and vec1: %f\n", similarity4);

    assert(fabsf(similarity1 - 1.0f) < EPSILON);
    assert(fabsf(similarity2 + 1.0f) < EPSILON);
    assert(fabsf(similarity3 - 0.0f) < EPSILON);
    assert(fabsf(similarity4 - 1.0f) < EPSILON);
}

int main() {
    test_cosine_similarity();
    printf("All cosine similarity tests passed!\n");
    return 0;
}
