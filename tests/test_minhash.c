#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/algorithms/minhash.h"
#include "../src/algorithms/jaccard.h"

#define MINHASH_SIZE 256

void test_minhash_create() {
    MinHash* mh;
    assert(minhash_create(&mh, MINHASH_SIZE, 42) == MINHASH_SUCCESS);
    assert(mh != NULL);
    assert(mh->num_hashes == MINHASH_SIZE);
    assert(mh->seed == 42);
    minhash_free(mh);
}

void test_minhash_compute_from_tokens() {
    MinHash* mh1;
    MinHash* mh2;
    MinHash* mh3;
    assert(minhash_create(&mh1, MINHASH_SIZE, 42) == MINHASH_SUCCESS);
    assert(minhash_create(&mh2, MINHASH_SIZE, 42) == MINHASH_SUCCESS);
    assert(minhash_create(&mh3, MINHASH_SIZE, 42) == MINHASH_SUCCESS);
    
    uint32_t tokens1[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens2[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens3[] = {8, 7, 6, 5, 1, 2, 3, 4, 3, 2, 1};
    
    assert(minhash_compute_tokens(mh1, tokens1, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_tokens(mh2, tokens2, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_tokens(mh3, tokens3, 11, 3) == MINHASH_SUCCESS); // Updated size to 11 for tokens3
    
    float similarity1 = jaccard_similarity(mh1->signature, mh2->signature, MINHASH_SIZE);
    float similarity2 = jaccard_similarity(mh1->signature, mh3->signature, MINHASH_SIZE);
    
    printf("Similarity between identical token sequences: %f\n", similarity1);
    printf("Similarity between different token sequences: %f\n", similarity2);
    
    assert(similarity1 == 1.0f);
    assert(similarity2 < 1.0f && similarity2 > 0.0f);
    
    minhash_free(mh1);
    minhash_free(mh2);
    minhash_free(mh3);
}

int main() {
    test_minhash_create();
    test_minhash_compute_from_tokens();
    printf("All MinHash tests passed!\n");
    return 0;
}

