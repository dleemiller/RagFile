#ifndef MINHASH_SCANNER_H
#define MINHASH_SCANNER_H

#include <Python.h>
#include <stdint.h>

#define MAGIC_NUMBER 0x4D484153  // "MHAS" in ASCII
#define MAX_MINHASH_SIZE 256

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t minhash_size;
    uint32_t embedding_size;
    uint32_t string_length;
    uint64_t string_offset;
    uint64_t embedding_offset;
} FileHeader;
#pragma pack(pop)

typedef struct {
    char* filename;
    float similarity;
} MatchResult;

// Function declarations

// Read the header from a file
FileHeader* read_file_header(const char* filename);

// Create a minhash from a string
uint32_t* create_minhash(const char* input, int num_hashes);

// Compute Jaccard similarity between two minhashes
float jaccard_similarity(uint32_t* minhash1, uint32_t* minhash2, int size);

// Scan files in a directory and return matches
MatchResult* scan_files(const char* directory, uint32_t* query_minhash, int minhash_size, float threshold, int* num_matches);

// Free the memory allocated for matches
void free_matches(MatchResult* matches, int num_matches);

// MurmurHash3 implementation (if you're using it)
uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed);

#endif // MINHASH_SCANNER_H
