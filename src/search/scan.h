#ifndef SCAN_H
#define SCAN_H

#include "../core/ragfile.h"
#include "../search/heap.h"

int process_file_jaccard(const char* path, const RagFile* query_rf, MinHeap* heap);
int process_file_hamming(const char* path, const RagFile* query_rf, MinHeap* heap);

#endif // SCAN_H

