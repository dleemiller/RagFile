#include <string.h>
#include <assert.h>
#include <time.h>
#include "../src/search/heap.h"
#include "../src/utils/strdup.h"


void test_heap_basic_operations() {
    MinHeap* heap = create_min_heap(5);
    assert(heap->size == 0 && "Heap should be initialized with size 0");

    add_to_heap(heap, (FileScore){strdup("file1.txt"), 0.9});
    add_to_heap(heap, (FileScore){strdup("file2.txt"), 0.85});
    add_to_heap(heap, (FileScore){strdup("file3.txt"), 0.95});

    assert(heap->size == 3 && "Heap should contain 3 elements");
    assert(heap->heap[0].score == 0.85 && "Root should be the minimum element");

    free_min_heap(heap);
    printf("Test Heap Basic Operations passed.\n");
}

void test_heap_replacement() {
    MinHeap* heap = create_min_heap(3);
    add_to_heap(heap, (FileScore){strdup("file1.txt"), 0.9});
    add_to_heap(heap, (FileScore){strdup("file2.txt"), 0.85});
    add_to_heap(heap, (FileScore){strdup("file3.txt"), 0.95});
    add_to_heap(heap, (FileScore){strdup("file4.txt"), 0.99});

    assert(heap->size == 3 && "Heap should not exceed its capacity");
    assert(heap->heap[0].score != 0.85 && "Root should have been replaced");
    assert(heap->heap[0].score == 0.9 && "Root should now be the next minimum element");

    free_min_heap(heap);
    printf("Test Heap Replacement passed.\n");
}

void test_heap_performance() {
    MinHeap* heap = create_min_heap(10000);
    clock_t start, end;
    double cpu_time_used;

    start = clock();
    for (int i = 0; i < 10000; i++) {
        char* path = malloc(20);
        sprintf(path, "file%d.txt", i);
        double score = (double) rand() / RAND_MAX;
        add_to_heap(heap, (FileScore){path, score});
    }
    end = clock();

    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Test Heap Performance: Inserted 10000 elements in %f seconds.\n", cpu_time_used);

    free_min_heap(heap);
}

void test_heap_remove_root() {
    MinHeap* heap = create_min_heap(5);
    add_to_heap(heap, (FileScore){strdup("file5.txt"), 0.75});
    add_to_heap(heap, (FileScore){strdup("file4.txt"), 0.80});
    add_to_heap(heap, (FileScore){strdup("file3.txt"), 0.85});
    add_to_heap(heap, (FileScore){strdup("file2.txt"), 0.90});
    add_to_heap(heap, (FileScore){strdup("file1.txt"), 0.95});

    // Heap is full at this point: [0.75, 0.80, 0.85, 0.90, 0.95]
    assert(heap->size == 5 && "Heap should initially contain 5 elements");
    assert(heap->heap[0].score == 0.75 && "Root should be the minimum element");

    // Remove the root and check the heap's new state
    remove_root(heap);
    assert(heap->size == 4 && "Heap size should decrease by one after removing root");
    assert(heap->heap[0].score == 0.80 && "New root should now be the next minimum element");

    // Continue removing and checking
    remove_root(heap);
    assert(heap->size == 3 && "Heap size should decrease by one after removing root");
    assert(heap->heap[0].score == 0.85 && "New root should now be the next minimum element");

    remove_root(heap);
    assert(heap->size == 2 && "Heap size should decrease by one after removing root");
    assert(heap->heap[0].score == 0.90 && "New root should now be the next minimum element");

    remove_root(heap);
    assert(heap->size == 1 && "Heap size should decrease by one after removing root");
    assert(heap->heap[0].score == 0.95 && "New root should now be the next minimum element");

    // Final removal: heap should be empty
    remove_root(heap);
    assert(heap->size == 0 && "Heap should be empty after removing all elements");

    // check behavior on empty heap
    remove_root(heap);
    assert(heap->size == 0 && "Removing from an already empty heap should not change size");

    free_min_heap(heap);
    printf("Test Heap Remove Root passed.\n");
}

int main() {
    test_heap_basic_operations();
    test_heap_replacement();
    test_heap_performance();
    test_heap_remove_root();
    return 0;
}

