#!/bin/bash

# Define common flags
INCLUDES="-I../src/core -I../src/algorithms -I../src/utils -I../src/search"
CFLAGS="-Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L $INCLUDES"
LFLAGS="-lm"

# Function for compiling and running tests
compile_and_run() {
    echo "Compiling $1"
    gcc $CFLAGS -o $1 ${@:2} $LFLAGS
    if [ $? -eq 0 ]; then
        echo "Running $1"
        ./$1
        echo "$1 completed"
    else
        echo "Failed to compile $1"
    fi
    echo ""
}

# List of tests and their dependencies
compile_and_run test_minhash ../src/core/minhash.c ../src/algorithms/jaccard.c test_minhash.c
compile_and_run test_ragfile ../src/core/ragfile.c ../src/core/minhash.c ../src/algorithms/jaccard.c ../src/utils/file_io.c test_ragfile.c
compile_and_run test_jaccard ../src/core/minhash.c ../src/algorithms/jaccard.c test_jaccard.c
compile_and_run test_minhash_jaccard ../src/core/minhash.c ../src/algorithms/jaccard.c test_minhash_jaccard.c
compile_and_run test_cosine ../src/algorithms/cosine.c test_cosine.c
compile_and_run test_heap ../src/search/heap.c test_heap.c
compile_and_run test_scan ../src/search/scan.c ../src/search/heap.c ../src/core/ragfile.c ../src/core/minhash.c ../src/utils/file_io.c ../src/algorithms/jaccard.c test_scan.c

echo "All tests completed."

