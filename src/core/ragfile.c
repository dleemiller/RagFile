#include "ragfile.h"
#include "minhash.h"
#include "../utils/file_io.h"
#include <stdlib.h>
#include <string.h>

RagfileError ragfile_compute_minhash(const uint32_t* token_ids, size_t token_count, uint64_t* minhash_signature) {
    if (!token_ids || !minhash_signature || token_count == 0) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    MinHash* mh;
    if (minhash_create(&mh, MINHASH_SIZE, 0) != MINHASH_SUCCESS) {
        return RAGFILE_ERROR_MEMORY;
    }

    MinHashError mh_error = minhash_compute_from_tokens(mh, token_ids, token_count, 3);  // Using 3-token shingles
    if (mh_error != MINHASH_SUCCESS) {
        minhash_free(mh);
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    memcpy(minhash_signature, mh->signature, MINHASH_SIZE * sizeof(uint64_t));

    minhash_free(mh);
    return RAGFILE_SUCCESS;
}

RagfileError ragfile_create(RagFile** rf, const char* text, const uint32_t* token_ids, size_t token_count,
                            const float* embedding, uint32_t embedding_size, const char* metadata, 
                            uint16_t tokenizer_id_hash, uint16_t embedding_id_hash, 
                            uint16_t metadata_version) {
    if (!rf || !text || !token_ids || !embedding) {
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
    (*rf)->header.text_size = strlen(text);
    (*rf)->header.embedding_size = embedding_size;
    (*rf)->header.metadata_size = metadata ? strlen(metadata) : 0;
    (*rf)->header.tokenizer_id_hash = tokenizer_id_hash;
    (*rf)->header.embedding_id_hash = embedding_id_hash;
    (*rf)->header.metadata_version = metadata_version;

    // Compute minhash signature
    RagfileError mh_error = ragfile_compute_minhash(token_ids, token_count, (*rf)->header.minhash_signature);
    if (mh_error != RAGFILE_SUCCESS) {
        free(*rf);
        *rf = NULL;
        return mh_error;
    }

    // Copy text
    (*rf)->text = strdup(text);
    if ((*rf)->text == NULL) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }

    // Copy embedding
    (*rf)->embedding = (float*)malloc(embedding_size * sizeof(float));
    if ((*rf)->embedding == NULL) {
        free((*rf)->text);
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }
    memcpy((*rf)->embedding, embedding, embedding_size * sizeof(float));

    // Copy metadata
    if (metadata) {
        (*rf)->metadata = strdup(metadata);
        if ((*rf)->metadata == NULL) {
            free((*rf)->embedding);
            free((*rf)->text);
            free(*rf);
            *rf = NULL;
            return RAGFILE_ERROR_MEMORY;
        }
    } else {
        (*rf)->metadata = NULL;
    }

    return RAGFILE_SUCCESS;
}

void ragfile_free(RagFile* rf) {
    if (rf) {
        free(rf->text);
        free(rf->embedding);
        free(rf->metadata);
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

    // Load text
    if (read_text(file, &(*rf)->text, (*rf)->header.text_size) != FILE_IO_SUCCESS) {
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Load embedding
    (*rf)->embedding = (float*)malloc((*rf)->header.embedding_size * sizeof(float));
    if ((*rf)->embedding == NULL) {
        free((*rf)->text);
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_MEMORY;
    }
    if (read_embedding(file, (*rf)->embedding, (*rf)->header.embedding_size) != FILE_IO_SUCCESS) {
        free((*rf)->embedding);
        free((*rf)->text);
        free(*rf);
        *rf = NULL;
        return RAGFILE_ERROR_IO;
    }

    // Load metadata
    if ((*rf)->header.metadata_size > 0) {
        if (read_metadata(file, &(*rf)->metadata, (*rf)->header.metadata_size) != FILE_IO_SUCCESS) {
            free((*rf)->embedding);
            free((*rf)->text);
            free(*rf);
            *rf = NULL;
            return RAGFILE_ERROR_IO;
        }
    } else {
        (*rf)->metadata = NULL;
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

    if (write_text(file, rf->text, rf->header.text_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (write_embedding(file, rf->embedding, rf->header.embedding_size) != FILE_IO_SUCCESS) {
        return RAGFILE_ERROR_IO;
    }

    if (rf->metadata && rf->header.metadata_size > 0) {
        if (write_metadata(file, rf->metadata, rf->header.metadata_size) != FILE_IO_SUCCESS) {
            return RAGFILE_ERROR_IO;
        }
    }

    return RAGFILE_SUCCESS;
}

uint16_t ragfile_compute_id_hash(const char* id_string) {
    if (!id_string) {
        return 0;
    }

    // Simple CRC16 implementation
    uint16_t crc = 0xFFFF;
    size_t len = strlen(id_string);
    
    for (size_t i = 0; i < len; i++) {
        uint8_t ch = id_string[i];
        for (int j = 0; j < 8; j++) {
            uint16_t b = (crc ^ ch) & 1;
            crc >>= 1;
            if (b) crc ^= 0xA001;
            ch >>= 1;
        }
    }
    
    return crc;
}
