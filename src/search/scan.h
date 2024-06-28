// scan.h
#ifndef SCAN_H
#define SCAN_H

#include "../core/ragfile.h"
#include "../search/heap.h"
#include <stdbool.h>

/**
 * Processes a single file and potentially adds it to the min heap.
 *
 * @param file_path Path to the .rag file to process.
 * @param referenceRagFile Pointer to a RagFile containing the reference minhash signature.
 * @param heap MinHeap structure to store top k results.
 * @return int Status code (0 for success, non-zero for errors).
 */
int process_file(const char* file_path, const RagFile* referenceRagFile, MinHeap* heap);

#endif // SCAN_H

