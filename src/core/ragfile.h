#ifndef RAGFILE_H
#define RAGFILE_H

#include "../include/config.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    RAGFILE_SUCCESS = 0,
    RAGFILE_ERROR_IO,
    RAGFILE_ERROR_FORMAT,
    RAGFILE_ERROR_MEMORY,
    RAGFILE_ERROR_INVALID_ARGUMENT,
    RAGFILE_ERROR_INVALID_CONFIG
} RagfileError;

typedef enum {
    CONFIG_SMALL,
    CONFIG_MEDIUM,
} ConfigType;

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint16_t tokenizer_id_hash;
    uint16_t embedding_id_hash;
    ConfigType config_type; // Indicates the type of configuration
} BaseHeader;

typedef struct {
    uint8_t binary_embedding[CONFIG_SMALL_BINARY_EMBEDDING_BYTE_DIM];
    uint32_t minhash_signature[CONFIG_SMALL_MINHASH_SIZE];
} ConfigSmall;

typedef struct {
    uint8_t binary_embedding[CONFIG_MEDIUM_BINARY_EMBEDDING_BYTE_DIM];
    uint32_t minhash_signature[CONFIG_MEDIUM_MINHASH_SIZE];
} ConfigMedium;

typedef union {
    ConfigSmall config_small;
    ConfigMedium config_medium;
} HeaderConfig;

typedef struct {
    BaseHeader base;
    HeaderConfig config;
} RagfileHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
    uint16_t text_hash;
    uint32_t text_size;
    uint16_t metadata_version;
    uint32_t metadata_size;
    uint16_t num_embeddings;
    uint16_t embedding_dim;
    uint32_t embedding_size;
    char tokenizer_id[MODEL_ID_SIZE];
    char embedding_id[MODEL_ID_SIZE];
} FileMetadata;
#pragma pack(pop)

typedef struct {
    RagfileHeader header;
    FileMetadata file_metadata;
    char* text;
    float* embeddings;
    char* extended_metadata;
} RagFile;

RagfileError ragfile_compute_minhash(const uint32_t* token_ids, size_t token_count, size_t half_minhash_size, void* minhash_signature);
RagfileError compute_binary_embedding(const float* embeddings, uint32_t num_embeddings, uint16_t embedding_dim, size_t binary_embedding_size, void* binary_embedding);
RagfileError ragfile_create(RagFile** rf, const char* text, const uint32_t* token_ids, size_t token_count,
                            const float* embeddings, uint32_t embedding_size, const char* extended_metadata,
                            const char* tokenizer_id, const char* embedding_id, 
                            uint16_t extended_metadata_version, uint16_t num_embeddings, uint16_t embedding_dim, ConfigType config_type);
void ragfile_free(RagFile* rf);

/**
 * Load a RagFile from disk.
 *
 * @param rf Pointer to a RagFile pointer where the loaded object will be stored.
 * @param file FILE pointer to the file to be loaded.
 * @return RAGFILE_SUCCESS on success, or an error code on failure.
 */
RagfileError ragfile_load(RagFile** rf, FILE* file);

/**
 * Save a RagFile to disk.
 *
 * @param rf Pointer to the RagFile to be saved.
 * @param file FILE pointer to where the file will be saved.
 * @return RAGFILE_SUCCESS on success, or an error code on failure.
 */
RagfileError ragfile_save(const RagFile* rf, FILE* file);

/**
 * Free memory associated with a RagFile object.
 *
 * @param rf Pointer to the RagFile to be freed.
 */
void ragfile_free(RagFile* rf);

/**
 * Compute a hash for an identifier string.
 *
 * @param id_string The identifier string to be hashed.
 * @return The computed hash value.
 */
uint16_t crc16(const char* input_string);

#endif // RAGFILE_H

