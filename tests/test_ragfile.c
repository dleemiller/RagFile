#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/core/ragfile.h"
#include "../src/algorithms/jaccard.h"

void test_ragfile_create_save_load() {
    RagFile* rf;
    const char* text = "Test text";
    uint32_t tokens[] = {1, 2, 3, 4, 5, 6, 7, 8};
    float embedding[] = {0.1f, 0.2f, 0.3f, 0.4f};
    const char* metadata = "Test metadata";
    
    assert(ragfile_create(&rf, text, tokens, 8, embedding, 4, metadata,
                          ragfile_compute_id_hash("test_tokenizer"),
                          ragfile_compute_id_hash("test_embedding"),
                          1) == RAGFILE_SUCCESS);
    
    // Open file for writing
    FILE *file = fopen("test_ragfile.rag", "wb");
    assert(file != NULL && "Failed to open file for writing");
    assert(ragfile_save(rf, file) == RAGFILE_SUCCESS);
    fclose(file); // Close the file after saving
    
    RagFile* loaded_rf;
    // Open file for reading
    file = fopen("test_ragfile.rag", "rb");
    assert(file != NULL && "Failed to open file for reading");
    assert(ragfile_load(&loaded_rf, file) == RAGFILE_SUCCESS);
    fclose(file); // Close the file after loading
    
    // Assertions to check loaded data
    assert(loaded_rf->header.embedding_size == 4);
    assert(loaded_rf->header.metadata_size == strlen(metadata));
    assert(strcmp(loaded_rf->metadata, metadata) == 0);
    
    float similarity = jaccard_similarity(rf->header.minhash_signature, loaded_rf->header.minhash_signature);
    assert(similarity == 1.0f);
    
    for (int i = 0; i < 4; i++) {
        assert(rf->embedding[i] == loaded_rf->embedding[i]);
    }
    
    ragfile_free(rf);
    ragfile_free(loaded_rf);
    
    remove("test_ragfile.rag");
}

void test_ragfile_id_hash() {
    uint16_t hash1 = ragfile_compute_id_hash("test_tokenizer");
    uint16_t hash2 = ragfile_compute_id_hash("test_tokenizer");
    uint16_t hash3 = ragfile_compute_id_hash("different_tokenizer");
    
    assert(hash1 == hash2);
    assert(hash1 != hash3);
}

int main() {
    test_ragfile_create_save_load();
    test_ragfile_id_hash();
    printf("All RagFile tests passed!\n");
    return 0;
}

