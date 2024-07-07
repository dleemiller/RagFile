#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../src/core/ragfile.h"
#include "../src/algorithms/jaccard.h"

void print_binary_vector(const uint32_t* vector, size_t size) {
    printf("Binary Vector: ");
    for (size_t i = 0; i < size; i++) {
        for (int j = 31; j >= 0; j--) {
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
    float16_t dense_vector[] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f, 0.7f, -0.8f};
    float embedding[] = {0.1f, -0.2f, 0.3f, -0.4f, 0.5f, -0.6f, 0.7f, -0.8f};
    const char* metadata = "Test metadata";

    assert(ragfile_create(&rf, text, tokens, 8, dense_vector, 8, embedding, 8, metadata,
                          "test_tokenizer", "test_embedding", 1, 1, 8) == RAGFILE_SUCCESS);

    print_binary_vector(rf->header.scan_vector, 8);

    FILE *file = fopen("test_ragfile.rag", "wb");
    assert(file != NULL && "Failed to open file for writing");

    if (file) {
        assert(ragfile_save(rf, file) == RAGFILE_SUCCESS);
        fclose(file);
    }

    RagFile* loaded_rf;
    file = fopen("test_ragfile.rag", "rb");
    assert(file != NULL && "Failed to open file for reading");

    if (file) {
        assert(ragfile_load(&loaded_rf, file) == RAGFILE_SUCCESS);
        fclose(file);
    }

    print_binary_vector(loaded_rf->header.scan_vector, 8);

    assert(loaded_rf->header.embedding_size == 8);
    assert(loaded_rf->header.metadata_size == strlen(metadata));
    assert(strcmp(loaded_rf->extended_metadata, metadata) == 0);

    float similarity = jaccard_similarity(rf->header.scan_vector, loaded_rf->header.scan_vector, SCAN_VEC_DIM);
    assert(similarity == 1.0f);

    for (int i = 0; i < 8; i++) {
        assert(rf->embeddings[i] == loaded_rf->embeddings[i]);
    }

    ragfile_free(rf);
    ragfile_free(loaded_rf);

    remove("test_ragfile.rag");
}


int main() {
    test_ragfile_create_save_load();
    printf("All RagFile tests passed!\n");
    return 0;
}

