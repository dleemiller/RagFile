#include "heap.h"

int parent(int i) { return (i - 1) / 2; }
int left(int i) { return (2 * i + 1); }
int right(int i) { return (2 * i + 2); }

void swap(FileScore *x, FileScore *y) {
    FileScore temp = *x;
    *x = *y;
    *y = temp;
}

void heapify_down(MinHeap* minHeap, int i) {
    int l = left(i);
    int r = right(i);
    int smallest = i;
    if (l < minHeap->size && minHeap->heap[l].score < minHeap->heap[i].score)
        smallest = l;
    if (r < minHeap->size && minHeap->heap[r].score < minHeap->heap[smallest].score)
        smallest = r;
    if (smallest != i) {
        swap(&minHeap->heap[i], &minHeap->heap[smallest]);
        heapify_down(minHeap, smallest);
    }
}

void heapify_up(MinHeap* minHeap, int i) {
    while (i != 0 && minHeap->heap[parent(i)].score > minHeap->heap[i].score) {
        swap(&minHeap->heap[i], &minHeap->heap[parent(i)]);
        i = parent(i);
    }
}

MinHeap* create_min_heap(int capacity) {
    MinHeap* minHeap = (MinHeap*)malloc(sizeof(MinHeap));
    minHeap->heap = (FileScore*)malloc(sizeof(FileScore) * capacity);
    minHeap->size = 0;
    minHeap->capacity = capacity;
    return minHeap;
}

void add_to_heap(MinHeap* minHeap, FileScore fileScore) {
    if (minHeap->size < minHeap->capacity) {
        minHeap->heap[minHeap->size] = fileScore;
        heapify_up(minHeap, minHeap->size);
        minHeap->size++;
    } else if (fileScore.score > minHeap->heap[0].score) {
        minHeap->heap[0] = fileScore;
        heapify_down(minHeap, 0);
    }
}

void free_min_heap(MinHeap* minHeap) {
    for (int i = 0; i < minHeap->size; i++) {
        free(minHeap->heap[i].path);
    }
    free(minHeap->heap);
    free(minHeap);
}

