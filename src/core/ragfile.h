#ifndef RAGFILE_H
#define RAGFILE_H

#include "../include/config.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    RAGFILE_SUCCESS = 0,
    RAGFILE_ERROR_IO,
    RAGFILE_ERROR_FORMAT,
    RAGFILE_ERROR_MEMORY,
    RAGFILE_ERROR_INVALID_ARGUMENT
} RagfileError;

typedef enum {
    BINARY_EMBEDDING,
    MIN_HASH,
    TOKEN_ID
} VectorType;

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint32_t flags;
    uint16_t vector1_type;
    uint16_t vector2_type;
    uint16_t vector1_dim;
    uint16_t vector2_dim;
    uint32_t vector1[DIMENSION];
    uint32_t vector2[DIMENSION];
    uint16_t text_hash;
    uint32_t text_size;
    uint16_t metadata_version;
    uint32_t metadata_size;
    uint16_t num_embeddings;
    uint16_t embedding_dim;
    uint32_t embedding_size;
    char tokenizer_id[MODEL_ID_SIZE];
    char embedding_id[MODEL_ID_SIZE];
} RagfileHeader;
#pragma pack(pop)

typedef struct {
    RagfileHeader header;
    char* text;
    float* embeddings;
    char* extended_metadata;
} RagFile;

RagfileError ragfile_create(RagFile** rf, const char* text, 
                            const uint32_t* vector1, uint16_t vector1_dim, 
                            const uint32_t* vector2, uint16_t vector2_dim,
                            VectorType vector1_type, VectorType vector2_type,
                            const float* embeddings, uint32_t embedding_size, const char* extended_metadata,
                            const char* tokenizer_id, const char* embedding_id, 
                            uint16_t extended_metadata_version, uint16_t num_embeddings, uint16_t embedding_dim);

void ragfile_free(RagFile* rf);
RagfileError ragfile_load(RagFile** rf, FILE* file);
RagfileError ragfile_save(const RagFile* rf, FILE* file);

#endif // RAGFILE_H

