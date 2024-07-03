#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/core/ragfile.h"
#include "../src/algorithms/jaccard.h"


void print_binary_vector(const uint8_t* vector, size_t size) {
    printf("Binary Vector: ");
    for (size_t i = 0; i < size; i++) {
        // Print each byte as a binary string
        for (int j = 7; j >= 0; j--) {
            printf("%d", (vector[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}

void test_ragfile_create_save_load() {
    RagFile* rf;
    const char* text = "Test text";
    uint32_t tokens[] = {1, 2, 3, 4, 5, 6, 7, 8};
    float embedding[] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f, 0.7f, -0.8f};
    const char* metadata = "Test metadata";

    // Ensure embedding_size is correctly interpreted (should likely be sizeof(float) * 8)
    assert(ragfile_create(&rf, text, tokens, 8, embedding, 8, metadata,
                          "test_tokenizer", "test_embedding", 1, 1, 8 * sizeof(float)) == RAGFILE_SUCCESS);


    // Print the binary vector right after creation
    print_binary_vector(rf->header.binary_embedding, BINARY_EMBEDDING_BYTE_DIM);

    // Open file for writing
    FILE *file = fopen("test_ragfile.rag", "wb");
    assert(file != NULL && "Failed to open file for writing");

    if (file) {
        assert(ragfile_save(rf, file) == RAGFILE_SUCCESS);
        fclose(file); // Close the file after saving
    }

    RagFile* loaded_rf;
    // Open file for reading
    file = fopen("test_ragfile.rag", "rb");
    assert(file != NULL && "Failed to open file for reading");

    if (file) {
        assert(ragfile_load(&loaded_rf, file) == RAGFILE_SUCCESS);
        fclose(file); // Close the file after loading
    }


    // Print the binary vector after loading
    print_binary_vector(loaded_rf->header.binary_embedding, BINARY_EMBEDDING_BYTE_DIM);

    // Debugging output
    printf("Expected Embedding Size: %d\n", 8);
    printf("Actual Embedding Size: %u\n", loaded_rf->file_metadata.embedding_size);

    // Assertions to check loaded data
    assert(loaded_rf->file_metadata.embedding_size == 8);
    assert(loaded_rf->file_metadata.metadata_size == strlen(metadata));
    assert(strcmp(loaded_rf->extended_metadata, metadata) == 0);

    float similarity = jaccard_similarity(rf->header.minhash_signature, loaded_rf->header.minhash_signature);
    assert(similarity == 1.0f);

    for (int i = 0; i < 8; i++) {
        assert(rf->embeddings[i] == loaded_rf->embeddings[i]);
    }

    ragfile_free(rf);
    ragfile_free(loaded_rf);

    remove("test_ragfile.rag");
}

void test_ragfile_id_hash() {
    uint16_t hash1 = crc16("test_tokenizer");
    uint16_t hash2 = crc16("test_tokenizer");
    uint16_t hash3 = crc16("different_tokenizer");
    
    assert(hash1 == hash2);
    assert(hash1 != hash3);
}

int main() {
    test_ragfile_create_save_load();
    test_ragfile_id_hash();
    printf("All RagFile tests passed!\n");
    return 0;
}

