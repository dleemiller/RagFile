#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ragfile.h"
#include "../include/config.h"
#include "../include/float16.h"
#include "../utils/file_io.h"
#include "../utils/strdup.h"

RagfileError ragfile_create(RagFile** rf, const char* text, 
                            const uint32_t* scan_vector, uint16_t scan_vector_dim, 
                            const float16_t* dense_vector, uint16_t dense_vector_dim,
                            const float* embeddings, uint32_t embedding_size, const char* extended_metadata,
                            const char* tokenizer_id, const char* embedding_id, 
                            uint16_t extended_metadata_version, uint16_t num_embeddings, uint16_t embedding_dim) {
    if (!rf || !text || !scan_vector || !dense_vector || !embeddings || !tokenizer_id || !embedding_id) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    *rf = (RagFile*)calloc(1, sizeof(RagFile));
    if (*rf == NULL) {
        return RAGFILE_ERROR_MEMORY;
    }

    // Initialize header
    (*rf)->header.magic = RAGFILE_MAGIC;
    (*rf)->header.version = RAGFILE_VERSION;
    (*rf)->header.flags = 0;
    (*rf)->header.scan_vector_dim = scan_vector_dim;
    (*rf)->header.dense_vector_dim = dense_vector_dim;

    // Initialize vectors with zeros and copy provided dimensions
    memset((*rf)->header.scan_vector, 0, SCAN_VEC_DIM * sizeof(uint32_t));
    memset((*rf)->header.dense_vector, 0, DENSE_VEC_DIM * sizeof(float16_t));
    memcpy((*rf)->header.scan_vector, scan_vector, scan_vector_dim * sizeof(uint32_t));
    memcpy((*rf)->header.dense_vector, dense_vector, dense_vector_dim * sizeof(uint32_t));

    memset((*rf)->header.tokenizer_id, 0, MODEL_ID_SIZE);
    strncpy((*rf)->header.tokenizer_id, tokenizer_id, MODEL_ID_SIZE - 1);
    (*rf)->header.tokenizer_id[MODEL_ID_SIZE - 1] = '\0';
    memset((*rf)->header.embedding_id, 0, MODEL_ID_SIZE);
    strncpy((*rf)->header.embedding_id, embedding_id, MODEL_ID_SIZE - 1);
    (*rf)->header.embedding_id[MODEL_ID_SIZE - 1] = '\0';

    (*rf)->header.text_size = strlen(text);
    (*rf)->header.text_hash = 0;  // Hashing removed
    (*rf)->header.embedding_size = embedding_size;
    (*rf)->header.metadata_size = extended_metadata ? strlen(extended_metadata) : 0;
    (*rf)->header.metadata_version = extended_metadata_version;
    (*rf)->header.num_embeddings = num_embeddings;
    (*rf)->header.embedding_dim = embedding_dim;

    // Copy text
    (*rf)->text = strdup(text);
    if ((*rf)->text == NULL) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }

    // Allocating embeddings using calloc to initialize them to zero
    (*rf)->embeddings = (float*)calloc(embedding_size, sizeof(float));
    if ((*rf)->embeddings == NULL) {
        ragfile_free(*rf);  // Ensures all previously allocated memory is freed properly
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }
    memcpy((*rf)->embeddings, embeddings, embedding_size * sizeof(float));

    // Copy extended metadata
    if (extended_metadata) {
        (*rf)->extended_metadata = strdup(extended_metadata);
        if ((*rf)->extended_metadata == NULL) {
            ragfile_free(*rf);
            *rf = NULL;
            return RAGFILE_ERROR_MEMORY;
        }
    } else {
        (*rf)->extended_metadata = NULL;
    }

    return RAGFILE_SUCCESS;
}

void ragfile_free(RagFile* rf) {
    if (rf) {
        free(rf->text);
        rf->text = NULL;  // Prevent dangling pointer

        free(rf->embeddings);
        rf->embeddings = NULL;  // Prevent dangling pointer

        free(rf->extended_metadata);
        rf->extended_metadata = NULL;  // Prevent dangling pointer

        free(rf);
        rf = NULL;  // Prevent dangling pointer
    }
}

RagfileError ragfile_load(RagFile** rf, FILE* file) {
    if (!rf || !file) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    *rf = (RagFile*)calloc(1, sizeof(RagFile));
    if (*rf == NULL) {
        return RAGFILE_ERROR_MEMORY;
    }

    if (read_ragfile_header(file, &(*rf)->header) != FILE_IO_SUCCESS) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    if ((*rf)->header.magic != RAGFILE_MAGIC || (*rf)->header.version != RAGFILE_VERSION) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_FORMAT;
    }

    if (read_text(file, &(*rf)->text, (*rf)->header.text_size) != FILE_IO_SUCCESS) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    (*rf)->embeddings = (float*)calloc((*rf)->header.embedding_size, sizeof(float));
    if ((*rf)->embeddings == NULL || read_embedding(file, (*rf)->embeddings, (*rf)->header.embedding_size) != FILE_IO_SUCCESS) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }

    if ((*rf)->header.metadata_size > 0) {
        if (read_metadata(file, &(*rf)->extended_metadata, (*rf)->header.metadata_size) != FILE_IO_SUCCESS) {
            ragfile_free(*rf);
            *rf = NULL;
            return RAGFILE_ERROR_IO;
        }
    } else {
        (*rf)->extended_metadata = NULL;
    }

    return RAGFILE_SUCCESS;
}

RagfileError ragfile_save(const RagFile* rf, FILE* file) {
    if (!rf || !file || !rf->text || !rf->embeddings || (rf->extended_metadata && rf->header.metadata_size == 0)) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    if (write_ragfile_header(file, &rf->header) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_text(file, rf->text, rf->header.text_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_embedding(file, rf->embeddings, rf->header.embedding_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (rf->extended_metadata && rf->header.metadata_size > 0) {
        if (write_metadata(file, rf->extended_metadata, rf->header.metadata_size) != FILE_IO_SUCCESS) {
            return RAGFILE_ERROR_IO;
        }
    }

    return RAGFILE_SUCCESS;
}

