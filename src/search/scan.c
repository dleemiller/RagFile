#include "scan.h"
#include "math.h"
#include "../utils/file_io.h"
#include "../utils/strdup.h"
#include "../algorithms/jaccard.h"
#include "../algorithms/hamming.h"

int process_file_jaccard(const char* path, const RagFile* query_rf, MinHeap* heap) {
    FILE* file;
    if (file_open(&file, path, false) != FILE_IO_SUCCESS) {
        return -1;
    }
    
    RagFile* candidate_rf = NULL;
    if (ragfile_load(&candidate_rf, file) != RAGFILE_SUCCESS) {
        file_close(file);
        return -1;
    }
    file_close(file);

    uint16_t query_dim = query_rf->header.scan_vector_dim;
    uint16_t candidate_dim = candidate_rf->header.scan_vector_dim;

    if (query_dim != candidate_dim) {
        ragfile_free(candidate_rf);
        return -1;
    }

    float similarity = jaccard_similarity(query_rf->header.scan_vector, 
                                          candidate_rf->header.scan_vector, 
                                          query_dim);

    add_to_heap(heap, (FileScore){strdup(path), similarity});
    ragfile_free(candidate_rf);
    return 0;
}

int process_file_hamming(const char* path, const RagFile* query_rf, MinHeap* heap) {
    FILE* file;
    if (file_open(&file, path, false) != FILE_IO_SUCCESS) {
        return -1;
    }
    
    RagFile* candidate_rf = NULL;
    if (ragfile_load(&candidate_rf, file) != RAGFILE_SUCCESS) {
        file_close(file);
        return -1;
    }
    file_close(file);

    uint16_t query_dim = query_rf->header.scan_vector_dim;
    uint16_t candidate_dim = candidate_rf->header.scan_vector_dim;

    if (query_dim != candidate_dim) {
        ragfile_free(candidate_rf);
        return -1;
    }

    double similarity = hamming_similarity(query_rf->header.scan_vector, 
                                           candidate_rf->header.scan_vector, 
                                           query_dim);

    if (similarity < 0) {
        ragfile_free(candidate_rf);
        return -1;
    }

    add_to_heap(heap, (FileScore){strdup(path), (float)similarity});
    ragfile_free(candidate_rf);
    return 0;
}
