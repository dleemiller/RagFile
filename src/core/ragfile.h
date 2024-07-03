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

#pragma pack(push, 1)
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t flags;
    uint16_t tokenizer_id_hash;
    uint16_t embedding_id_hash;
    uint8_t  binary_embedding[BINARY_EMBEDDING_BYTE_DIM];
    uint32_t minhash_signature[MINHASH_SIZE];
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

/**
 * Create a new RagFile object.
 *
 * @param rf Pointer to a RagFile pointer where the new object will be stored.
 * @param text The text content of the RagFile.
 * @param token_ids Array of token IDs used for MinHash computation.
 * @param token_count Number of tokens in the token_ids array.
 * @param embedding Array of embedding values.
 * @param embedding_size Number of values in the embedding array.
 * @param metadata Optional metadata string (can be NULL).
 * @param tokenizer_id_hash Hash of the tokenizer identifier.
 * @param embedding_id_hash Hash of the embedding model identifier.
 * @param metadata_version Version of the metadata format.
 * @return RAGFILE_SUCCESS on success, or an error code on failure.
 */
RagfileError ragfile_create(RagFile** rf, const char* text, const uint32_t* token_ids, size_t token_count,
                            const float* embeddings, uint32_t embedding_size, const char* extended_metadata, 
                            const char* tokenizer_id, const char* embedding_id, 
                            uint16_t extended_metadata_version, uint16_t num_embeddings, uint16_t embedding_dim);

/**
 * Load a RagFile from disk.
 *
 * @param rf Pointer to a RagFile pointer where the loaded object will be stored.
 * @param filename Path to the file to be loaded.
 * @return RAGFILE_SUCCESS on success, or an error code on failure.
 */
RagfileError ragfile_load(RagFile** rf, FILE* file);
/**
 * Save a RagFile to disk.
 *
 * @param rf Pointer to the RagFile to be saved.
 * @param filename Path where the file will be saved.
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

/**
 * Compute the MinHash signature for a set of token IDs.
 *
 * @param token_ids Array of token IDs.
 * @param token_count Number of tokens in the array.
 * @param minhash_signature Pointer to an array where the MinHash signature will be stored.
 * @return RAGFILE_SUCCESS on success, or an error code on failure.
 */
RagfileError ragfile_compute_minhash(const uint32_t* token_ids, size_t token_count, uint32_t* minhash_signature);
RagfileError compute_binary_embedding(RagFile* rf, const float* embeddings, uint32_t num_embeddings, uint16_t embedding_dim);
 
#endif // RAGFILE_H
