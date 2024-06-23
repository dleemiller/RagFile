#include <stdio.h>
#include <assert.h>
#include "../src/core/minhash.h"
#include "../src/algorithms/jaccard.h"

void test_jaccard_similarity() {
    MinHash* mh1;
    MinHash* mh2;
    MinHash* mh3;
    
    assert(minhash_create(&mh1, 256, 42) == MINHASH_SUCCESS);
    assert(minhash_create(&mh2, 256, 42) == MINHASH_SUCCESS);
    assert(minhash_create(&mh3, 256, 42) == MINHASH_SUCCESS);
    
    uint32_t tokens1[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens2[] = {1, 2, 3, 4, 5, 6, 7, 8};
    uint32_t tokens3[] = {8, 7, 6, 5, 1, 2, 3, 4, 3, 2, 1};
    
    assert(minhash_compute_from_tokens(mh1, tokens1, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_from_tokens(mh2, tokens2, 8, 3) == MINHASH_SUCCESS);
    assert(minhash_compute_from_tokens(mh3, tokens3, 8, 3) == MINHASH_SUCCESS);
    
    float similarity1 = jaccard_similarity(mh1, mh2);
    float similarity2 = jaccard_similarity(mh1, mh3);
    float similarity3 = jaccard_similarity(mh1, mh1);
    
    printf("Similarity between identical token sequences: %f\n", similarity1);
    printf("Similarity between reversed token sequences: %f\n", similarity2);
    printf("Similarity with itself: %f\n", similarity3);
    
    assert(similarity1 == 1.0f);
    assert(similarity2 < 1.0f && similarity2 > 0.0f);
    assert(similarity3 == 1.0f);
    
    minhash_free(mh1);
    minhash_free(mh2);
    minhash_free(mh3);
}

int main() {
    test_jaccard_similarity();
    printf("All Jaccard similarity tests passed!\n");
    return 0;
}
