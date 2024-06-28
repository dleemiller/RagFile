// heap.h
#ifndef HEAP_H
#define HEAP_H

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char* path;       // File path
    double score;     // Jaccard similarity score
} FileScore;

typedef struct {
    FileScore* heap;  // Array of FileScore
    int size;         // Current number of elements in the heap
    int capacity;     // Maximum capacity of the heap
} MinHeap;

// Function declarations
MinHeap* create_min_heap(int capacity);
void add_to_heap(MinHeap* minHeap, FileScore fileScore);
void remove_root(MinHeap* minHeap);
void free_min_heap(MinHeap* minHeap);

#endif // HEAP_H

