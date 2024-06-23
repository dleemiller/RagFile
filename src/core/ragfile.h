#ifndef RAGFILE_H
#define RAGFILE_H

#include <stdint.h>
#include <stdbool.h>
#include "minhash.h"

#define RAGFILE_MAGIC 0x52414746 // "RAGF" in ASCII
#define RAGFILE_VERSION 1
#define METADATA_MAX_SIZE 1024
#define ID_HASH_SIZE 2 // Using CRC16

typedef enum {
    RAGFILE_SUCCESS = 0,
    RAGFILE_ERROR_IO,
    RAGFILE_ERROR_FORMAT,
    RAGFILE_ERROR_MEMORY,
    RAGFILE_ERROR_INVALID_ARGUMENT
} RagfileError;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint32_t minhash_size;
    uint32_t embedding_size;
    uint32_t metadata_size;
    uint16_t tokenizer_id_hash;
    uint16_t embedding_id_hash;
} RagfileHeader;

typedef struct {
    RagfileHeader header;
    MinHash* minhash;
    float* embedding;
    char* metadata;
} RagFile;

// Function prototypes
RagfileError ragfile_create(RagFile** rf, const uint32_t* token_ids, size_t token_count,
                            const float* embedding, uint32_t embedding_size,
                            const char* metadata, uint16_t tokenizer_id_hash,
                            uint16_t embedding_id_hash, size_t minhash_size);
RagfileError ragfile_load(RagFile** rf, const char* filename);
RagfileError ragfile_save(const RagFile* rf, const char* filename);
void ragfile_free(RagFile* rf);

// Utility functions
uint16_t ragfile_compute_id_hash(const char* id_string);

#endif // RAGFILE_H
