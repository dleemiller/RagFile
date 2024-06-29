#include <stdlib.h>
#include <string.h>
#include "ragfile.h"
#include "minhash.h"
#include "../include/config.h"
#include "../utils/file_io.h"
#include "../utils/strdup.h"


RagfileError ragfile_compute_minhash(const uint32_t* token_ids, size_t token_count, uint32_t* minhash_signature) {
    if (!token_ids || !minhash_signature || token_count == 0) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    // Half of MINHASH_SIZE for each of bi-grams and tri-grams
    const size_t half_minhash_size = MINHASH_SIZE / 2;

    MinHash* mh_bigrams;
    MinHash* mh_trigrams;

    // Create MinHash for bigrams
    if (minhash_create(&mh_bigrams, half_minhash_size, 0) != MINHASH_SUCCESS) {
        return RAGFILE_ERROR_MEMORY;
    }

    // Create MinHash for trigrams
    if (minhash_create(&mh_trigrams, half_minhash_size, 0) != MINHASH_SUCCESS) {
        minhash_free(mh_bigrams);
        return RAGFILE_ERROR_MEMORY;
    }

    // Compute MinHash for bigrams
    if (minhash_compute_from_tokens(mh_bigrams, token_ids, token_count, 2) != MINHASH_SUCCESS) {
        minhash_free(mh_bigrams);
        minhash_free(mh_trigrams);
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    // Compute MinHash for trigrams
    if (minhash_compute_from_tokens(mh_trigrams, token_ids, token_count, 3) != MINHASH_SUCCESS) {
        minhash_free(mh_bigrams);
        minhash_free(mh_trigrams);
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    // Combine results
    memcpy(minhash_signature, mh_bigrams->signature, half_minhash_size * sizeof(uint32_t));
    memcpy(minhash_signature + half_minhash_size, mh_trigrams->signature, half_minhash_size * sizeof(uint32_t));

    // Free resources
    minhash_free(mh_bigrams);
    minhash_free(mh_trigrams);

    return RAGFILE_SUCCESS;
}

RagfileError ragfile_create(RagFile** rf, const char* text, const uint32_t* token_ids, size_t token_count,
                            const float* embeddings, uint32_t embedding_size, const char* extended_metadata,
                            const char* tokenizer_id, const char* embedding_id, 
                            uint16_t extended_metadata_version, uint16_t num_embeddings, uint16_t embedding_dim) {
    if (!rf || !text || !token_ids || !embeddings || !tokenizer_id || !embedding_id) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    *rf = (RagFile*)malloc(sizeof(RagFile));
    if (*rf == NULL) {
        return RAGFILE_ERROR_MEMORY;
    }

    // Initialize header
    (*rf)->header.magic = RAGFILE_MAGIC;
    (*rf)->header.version = RAGFILE_VERSION;
    (*rf)->header.flags = 0;  // No flags set for now
    (*rf)->header.tokenizer_id_hash = crc16(tokenizer_id);
    (*rf)->header.embedding_id_hash = crc16(embedding_id);

    // Initialize file metadata
    strcpy((*rf)->file_metadata.tokenizer_id, tokenizer_id);
    strcpy((*rf)->file_metadata.embedding_id, embedding_id);
    (*rf)->file_metadata.text_size = strlen(text);
    (*rf)->file_metadata.text_hash = crc16(text);
    (*rf)->file_metadata.embedding_size = embedding_size;
    (*rf)->file_metadata.metadata_size = extended_metadata ? strlen(extended_metadata) : 0;
    (*rf)->file_metadata.metadata_version = extended_metadata_version;
    (*rf)->file_metadata.num_embeddings = num_embeddings;
    (*rf)->file_metadata.embedding_dim = embedding_dim;

    // Compute minhash signature
    RagfileError mh_error = ragfile_compute_minhash(token_ids, token_count, (*rf)->header.minhash_signature);
    if (mh_error != RAGFILE_SUCCESS) {
        ragfile_free(*rf);
        *rf = NULL;
        return mh_error;
    }

    // Copy text
    (*rf)->text = strdup(text);
    if ((*rf)->text == NULL) {
        ragfile_free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }

    // Copy embedding
    (*rf)->embeddings = (float*)malloc(embedding_size * sizeof(float));
    if ((*rf)->embeddings == NULL) {
        ragfile_free(*rf);
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
        free(rf->embeddings);
        free(rf->extended_metadata);
        free(rf);
    }
}


RagfileError ragfile_load(RagFile** rf, FILE* file) {
    if (!rf || !file) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    *rf = (RagFile*)malloc(sizeof(RagFile));
    if (*rf == NULL) {
        return RAGFILE_ERROR_MEMORY;
    }

    // Read header
    if (read_ragfile_header(file, &(*rf)->header) != FILE_IO_SUCCESS) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Verify magic number and version
    if ((*rf)->header.magic != RAGFILE_MAGIC || (*rf)->header.version != RAGFILE_VERSION) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_FORMAT;
    }

    // Read header
    if (read_file_metadata(file, &(*rf)->file_metadata) != FILE_IO_SUCCESS) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Load text
    if (read_text(file, &(*rf)->text, (*rf)->file_metadata.text_size) != FILE_IO_SUCCESS) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Load embedding
    (*rf)->embeddings = (float*)malloc((*rf)->file_metadata.embedding_size * sizeof(float));
    if ((*rf)->embeddings == NULL) {
        free((*rf)->text);
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }
    if (read_embedding(file, (*rf)->embeddings, (*rf)->file_metadata.embedding_size) != FILE_IO_SUCCESS) {
        free((*rf)->embeddings);
        free((*rf)->text);
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Load metadata
    if ((*rf)->file_metadata.metadata_size > 0) {
        if (read_metadata(file, &(*rf)->extended_metadata, (*rf)->file_metadata.metadata_size) != FILE_IO_SUCCESS) {
            free((*rf)->embeddings);
            free((*rf)->text);
            free(*rf);
            *rf = NULL;
            return RAGFILE_ERROR_IO;
        }
    } else {
        (*rf)->extended_metadata = NULL;
    }

    return RAGFILE_SUCCESS;
}

RagfileError ragfile_save(const RagFile* rf, FILE* file) {
    if (!rf || !file) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    if (write_ragfile_header(file, &rf->header) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_file_metadata(file, &rf->file_metadata) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_text(file, rf->text, rf->file_metadata.text_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_embedding(file, rf->embeddings, rf->file_metadata.embedding_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (rf->extended_metadata && rf->file_metadata.metadata_size > 0) {
        if (write_metadata(file, rf->extended_metadata, rf->file_metadata.metadata_size) != FILE_IO_SUCCESS) {
            return RAGFILE_ERROR_IO;
        }
    }

    return RAGFILE_SUCCESS;
}

uint16_t crc16(const char* input_string) {
    if (!input_string) {
        return 0;
    }

    // Simple CRC16 implementation
    uint16_t crc = 0xFFFF;
    size_t len = strlen(input_string);
    
    for (size_t i = 0; i < len; i++) {
        uint8_t ch = input_string[i];
        for (int j = 0; j < 8; j++) {
            uint16_t b = (crc ^ ch) & 1;
            crc >>= 1;
            if (b) crc ^= 0xA001;
            ch >>= 1;
        }
    }
    
    return crc;
}
