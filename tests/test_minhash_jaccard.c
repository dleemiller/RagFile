#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "../src/algorithms/minhash.h"
#include "../src/algorithms/jaccard.h"
#include "../src/include/config.h"

#define MINHASH_SIZE 256

void test_minhash_creation() {
    MinHash* mh;
    assert(minhash_create(&mh, MINHASH_SIZE, 42) == MINHASH_SUCCESS);
    assert(mh != NULL);
    assert(mh->num_hashes == MINHASH_SIZE);
    assert(mh->seed == 42);
    minhash_free(mh);
}

void test_minhash_computation() {
    MinHash* mh1;
    MinHash* mh2;
    MinHash* mh3;
    minhash_create(&mh1, MINHASH_SIZE, 42);
    minhash_create(&mh2, MINHASH_SIZE, 42);
    minhash_create(&mh3, MINHASH_SIZE, 42);
    
    uint32_t tokens1[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens2[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens3[] = {8, 7, 6, 5, 1, 2, 3, 4, 3, 2, 1};
    
    assert(minhash_compute_tokens(mh1, tokens1, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_tokens(mh2, tokens2, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_tokens(mh3, tokens3, 11, 3) == MINHASH_SUCCESS);
    
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

void test_minhash_merge() {
    MinHash* mh1;
    MinHash* mh2;
    MinHash* mh_merged;
    minhash_create(&mh1, MINHASH_SIZE, 42);
    minhash_create(&mh2, MINHASH_SIZE, 42);
    minhash_create(&mh_merged, MINHASH_SIZE, 42);
    
    uint32_t tokens1[] = {1, 2, 3, 4};
    uint32_t tokens2[] = {5, 6, 7, 8};
    
    minhash_compute_tokens(mh1, tokens1, 4, 2);
    minhash_compute_tokens(mh2, tokens2, 4, 2);
    
    memcpy(mh_merged->signature, mh1->signature, MINHASH_SIZE * sizeof(uint32_t));
    assert(minhash_merge(mh_merged, mh2) == MINHASH_SUCCESS);
    
    float similarity1 = jaccard_similarity(mh1->signature, mh_merged->signature, MINHASH_SIZE);
    float similarity2 = jaccard_similarity(mh2->signature, mh_merged->signature, MINHASH_SIZE);
    
    printf("Similarity between mh1 and merged: %f\n", similarity1);
    printf("Similarity between mh2 and merged: %f\n", similarity2);
    
    assert(similarity1 > 0.0f && similarity1 < 1.0f);
    assert(similarity2 > 0.0f && similarity2 < 1.0f);
    
    minhash_free(mh1);
    minhash_free(mh2);
    minhash_free(mh_merged);
}

int main() {
    test_minhash_creation();
    test_minhash_computation();
    test_minhash_merge();
    printf("All MinHash and Jaccard similarity tests passed!\n");
    return 0;
}

