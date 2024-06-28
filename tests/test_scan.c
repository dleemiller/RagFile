#include "../src/search/scan.h"
#include "../src/search/heap.h"
#include "../src/core/ragfile.h"
#include <assert.h>
#include <stdio.h>

void test_process_file() {
    // Setup
    RagFile testRagFile = {0};  // Setup a mock reference RagFile with minhash
    MinHeap* heap = create_min_heap(5);

    // Test
    int status = process_file("test.rag", &testRagFile, heap);
    assert(status == 0 && "Process file should succeed");
    assert(heap->size > 0 && "Heap should have at least one entry");

    // Cleanup
    free_min_heap(heap);
    printf("Test process_file passed.\n");
}

int main() {
    test_process_file();
    return 0;
}

