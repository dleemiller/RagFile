#!/bin/bash

# Define common flags
INCLUDES="-I../src/core -I../src/algorithms -I../src/utils -I../src/search"
CFLAGS="-Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L $INCLUDES"
LFLAGS="-lm"

# Function for compiling and running tests
compile_and_run() {
    local test_name=$1
    shift
    local sources=("${@:1:$#-1}")  # Get all arguments except the last one
    local custom_cflags="${!#}"    # Get the last argument (custom CFLAGS)
    echo "Compiling $test_name with $custom_cflags"
    gcc $CFLAGS $custom_cflags -o $test_name ${sources[@]} $LFLAGS
    if [ $? -eq 0 ]; then
        echo "Running $test_name"
        ./$test_name
        echo "$test_name completed"
    else
        echo "Failed to compile $test_name"
    fi
    echo ""
}

# List of tests and their dependencies
compile_and_run test_minhash "../src/core/minhash.c" "../src/algorithms/jaccard.c" "test_minhash.c"
compile_and_run test_ragfile "../src/core/ragfile.c" "../src/core/minhash.c" "../src/algorithms/jaccard.c" "../src/algorithms/quantize.c" "../src/utils/file_io.c" "test_ragfile.c" "-DBINARY_EMBEDDING_DIM=8" 
compile_and_run test_jaccard "../src/core/minhash.c" "../src/algorithms/jaccard.c" "test_jaccard.c"
compile_and_run test_minhash_jaccard "../src/core/minhash.c" "../src/algorithms/jaccard.c" "test_minhash_jaccard.c"
compile_and_run test_cosine "../src/algorithms/cosine.c" "test_cosine.c"
compile_and_run test_hamming "../src/algorithms/hamming.c" "test_hamming.c"
compile_and_run test_quantize "../src/algorithms/quantize.c" "test_quantize.c" "-DBINARY_EMBEDDING_DIM=16"
compile_and_run test_heap "../src/search/heap.c" "../src/utils/strdup.h" "test_heap.c"
compile_and_run test_scan "../src/search/scan.c" "../src/search/heap.c" "../src/core/ragfile.c" "../src/core/minhash.c" "../src/utils/file_io.c" "../src/algorithms/jaccard.c" "../src/algorithms/quantize.c" "test_scan.c"

echo "All tests completed."

