#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/core/ragfile.h"
#include "../include/config.h"
#include "../src/algorithms/jaccard.h"

// Function to print binary vectors for debugging
void print_binary_vector(const uint32_t* vector, size_t size) {
    printf("Binary Vector: ");
    for (size_t i = 0; i < size; i++) {
        // Print each uint32_t as a binary string
        for (int j = 31; j >= 0; j--) {
            printf("%d", (vector[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}

// Function to check the correctness of the ragfile header
void check_ragfile_header(const RagFile* rf, const char* text, const char* tokenizer_id, const char* embedding_id,
                          uint16_t scan_vector_dim, uint16_t dense_vector_dim, uint32_t embedding_size,
                          uint16_t num_embeddings, uint16_t embedding_dim) {
    assert(rf != NULL);
    assert(strcmp(rf->header.tokenizer_id, tokenizer_id) == 0);
    assert(strcmp(rf->header.embedding_id, embedding_id) == 0);
    assert(rf->header.scan_vector_dim == scan_vector_dim);
    assert(rf->header.dense_vector_dim == dense_vector_dim);
    assert(rf->header.embedding_size == embedding_size);
    assert(rf->header.text_size == strlen(text));
    assert(rf->header.num_embeddings == num_embeddings);
    assert(rf->header.embedding_dim == embedding_dim);

    // Add debug prints to understand the values
    printf("Expected text: %s\n", text);
    printf("Actual text: %s\n", rf->text);
    printf("Expected text length: %zu\n", strlen(text));
    printf("Actual text length: %zu\n", rf->text ? strlen(rf->text) : 0);

    assert(rf->text != NULL && strcmp(rf->text, text) == 0);
}

void test_ragfile_create_save_load() {
    RagFile* rf;
    const char* text = "Test text";
    uint32_t tokens[] = {1, 2, 3, 4, 5, 6, 7, 8};
    float16_t dense_vector[] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f, 0.7f, -0.8f};
    float embedding[] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f, 0.7f, -0.8f};
    const char* metadata = "Test metadata";

    // Create RagFile
    RagfileError create_result = ragfile_create(&rf, text, tokens, 8, dense_vector, 8, embedding, 8, metadata,
                          "test_tokenizer", "test_embedding", 1, 1, 8);
    assert(create_result == RAGFILE_SUCCESS);
    check_ragfile_header(rf, text, "test_tokenizer", "test_embedding", 8, 8, 8, 1, 8);

    // Print binary vector for debugging
    print_binary_vector(rf->header.scan_vector, 8);

    // Save to file
    FILE *file = fopen("test_ragfile.rag", "wb");
    assert(file != NULL && "Failed to open file for writing");

    if (file) {
        RagfileError save_result = ragfile_save(rf, file);
        assert(save_result == RAGFILE_SUCCESS);
        fclose(file);
    }

    // Load from file
    RagFile* loaded_rf;
    file = fopen("test_ragfile.rag", "rb");
    assert(file != NULL && "Failed to open file for reading");

    if (file) {
        RagfileError load_result = ragfile_load(&loaded_rf, file);
        assert(load_result == RAGFILE_SUCCESS);
        fclose(file);
    }

    // Print binary vector for debugging
    print_binary_vector(loaded_rf->header.scan_vector, 8);

    // Check the header after loading
    check_ragfile_header(loaded_rf, text, "test_tokenizer", "test_embedding", 8, 8, 8, 1, 8);

    ragfile_free(rf);
    ragfile_free(loaded_rf);

    remove("test_ragfile.rag");
}

void test_edge_cases() {
    RagFile* rf;
    const char* empty_text = "";
    uint32_t empty_tokens[] = {0};
    float16_t empty_dense_vector[] = {0};
    float empty_embedding[] = {0};
    const char* empty_metadata = ""; // Empty metadata

    // Create a RagFile with empty values
    RagfileError create_result = ragfile_create(&rf, empty_text, empty_tokens, 1, empty_dense_vector, 1, empty_embedding, 1, empty_metadata,
                          "empty_tokenizer", "empty_embedding", 1, 1, 1);
    assert(create_result == RAGFILE_SUCCESS);
    if (create_result != RAGFILE_SUCCESS) {
        printf("Failed to create ragfile: %d\n", create_result);
        return;
    }

    // Check the header with empty values
    check_ragfile_header(rf, empty_text, "empty_tokenizer", "empty_embedding", 1, 1, 1, 1, 1);

    // Save to file
    FILE *file = fopen("test_empty_ragfile.rag", "wb");
    assert(file != NULL && "Failed to open file for writing");

    if (file) {
        RagfileError save_result = ragfile_save(rf, file);
        assert(save_result == RAGFILE_SUCCESS);
        if (save_result != RAGFILE_SUCCESS) {
            printf("Failed to save ragfile: %d\n", save_result);
        }
        fclose(file);
    }

    // Load from file
    RagFile* loaded_rf;
    file = fopen("test_empty_ragfile.rag", "rb");
    assert(file != NULL && "Failed to open file for reading");

    if (file) {
        RagfileError load_result = ragfile_load(&loaded_rf, file);
        assert(load_result == RAGFILE_SUCCESS);
        if (load_result != RAGFILE_SUCCESS) {
            printf("Failed to load ragfile: %d\n", load_result);
            fclose(file);
            ragfile_free(rf);
            return;
        }
        fclose(file);
    }

    // Check the header after loading
    check_ragfile_header(loaded_rf, empty_text, "empty_tokenizer", "empty_embedding", 1, 1, 1, 1, 1);

    ragfile_free(rf);
    ragfile_free(loaded_rf);

    remove("test_empty_ragfile.rag");
}

int main() {
    test_ragfile_create_save_load();
    test_edge_cases();
    printf("All RagFile tests passed!\n");
    return 0;
}

