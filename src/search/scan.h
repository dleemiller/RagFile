#ifndef SCAN_H
#define SCAN_H

#include "../core/ragfile.h"
#include "../search/heap.h"

typedef struct {
    const uint32_t* vec;
    uint16_t dim;
    VectorType type;
} VecInfo;

VecInfo get_vec_info(const RagFile* rf, int vec_num);

int process_file_jaccard(const char* path, const RagFile* query_rf, MinHeap* heap, int vec_num);
int process_file_hamming(const char* path, const RagFile* query_rf, MinHeap* heap, int vec_num);

#endif // SCAN_H

