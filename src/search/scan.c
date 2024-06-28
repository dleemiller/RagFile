#include <stdio.h>
#include <string.h>
#include "scan.h"
#include "../utils/file_io.h"
#include "../algorithms/jaccard.h"

char* strdup(const char* str) {
    size_t len = strlen(str) + 1;    // Include space for the null-terminator
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

int process_file(const char* file_path, const RagFile* referenceRagFile, MinHeap* heap) {
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        perror("Failed to open file");
        return -1;  // File opening failed
    }

    RagfileHeader header;
    if (read_ragfile_header(file, &header) != FILE_IO_SUCCESS) {
        fclose(file);
        return -2;  // Header reading failed
    }

    double score = jaccard_similarity(referenceRagFile->header.minhash_signature, header.minhash_signature);
    add_to_heap(heap, (FileScore){strdup(file_path), score});

    fclose(file);
    return 0;  // Success
}

