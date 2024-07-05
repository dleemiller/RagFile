#include "scan.h"
#include "math.h"
#include "../utils/file_io.h"
#include "../utils/strdup.h"
#include "../algorithms/jaccard.h"
#include "../algorithms/hamming.h"

VecInfo get_vec_info(const RagFile* rf, int vec_num) {
    VecInfo info;
    if (vec_num == 2) {
        info.vec = rf->header.vector2;
        info.dim = rf->header.vector2_dim;
        info.type = rf->header.vector2_type;
    } else {
        info.vec = rf->header.vector1;
        info.dim = rf->header.vector1_dim;
        info.type = rf->header.vector1_type;
    }
    return info;
}

int process_file_jaccard(const char* path, const RagFile* query_rf, MinHeap* heap, int vec_num) {
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

    VecInfo query_info = get_vec_info(query_rf, vec_num);
    VecInfo candidate_info = get_vec_info(candidate_rf, vec_num);

    if (query_info.type != MIN_HASH || candidate_info.type != MIN_HASH) {
        ragfile_free(candidate_rf);
        return -1;
    }

    float similarity = jaccard_similarity(query_info.vec, candidate_info.vec, fmin(query_info.dim, candidate_info.dim));
    add_to_heap(heap, (FileScore){strdup(path), similarity});

    ragfile_free(candidate_rf);
    return 0;
}

int process_file_hamming(const char* path, const RagFile* query_rf, MinHeap* heap, int vec_num) {
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

    VecInfo query_info = get_vec_info(query_rf, vec_num);
    VecInfo candidate_info = get_vec_info(candidate_rf, vec_num);

    if (query_info.type != BINARY_EMBEDDING || candidate_info.type != BINARY_EMBEDDING) {
        ragfile_free(candidate_rf);
        return -1;
    }

    float similarity = hamming_similarity((const uint8_t*)query_info.vec, (const uint8_t*)candidate_info.vec, fmin(query_info.dim, candidate_info.dim));
    add_to_heap(heap, (FileScore){strdup(path), similarity});

    ragfile_free(candidate_rf);
    return 0;
}

