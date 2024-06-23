#include "ragfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


RagfileError ragfile_create(RagFile** rf, const uint32_t* token_ids, size_t token_count,
                            const float* embedding, uint32_t embedding_size,
                            const char* metadata, uint16_t tokenizer_id_hash,
                            uint16_t embedding_id_hash, size_t minhash_size) {
    *rf = (RagFile*)malloc(sizeof(RagFile));
    if (*rf == NULL) {
        return RAGFILE_ERROR_MEMORY;
    }

    // Initialize header
    (*rf)->header.magic = RAGFILE_MAGIC;
    (*rf)->header.version = RAGFILE_VERSION;
    (*rf)->header.flags = 0;  // No flags set for now
    (*rf)->header.minhash_size = minhash_size;
    (*rf)->header.embedding_size = embedding_size;
    (*rf)->header.metadata_size = metadata ? strlen(metadata) : 0;
    (*rf)->header.tokenizer_id_hash = tokenizer_id_hash;
    (*rf)->header.embedding_id_hash = embedding_id_hash;

    // Create MinHash
    if (minhash_create(&(*rf)->minhash, minhash_size, 0) != MINHASH_SUCCESS) {
        free(*rf);
        return RAGFILE_ERROR_MEMORY;
    }
    if (minhash_compute_from_tokens((*rf)->minhash, token_ids, token_count, 3) != MINHASH_SUCCESS) {
        minhash_free((*rf)->minhash);
        free(*rf);
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    // Copy embedding
    (*rf)->embedding = (float*)malloc(embedding_size * sizeof(float));
    if ((*rf)->embedding == NULL) {
        minhash_free((*rf)->minhash);
        free(*rf);
        return RAGFILE_ERROR_MEMORY;
    }
    memcpy((*rf)->embedding, embedding, embedding_size * sizeof(float));

    // Copy metadata
    if (metadata) {
        (*rf)->metadata = strdup(metadata);
        if ((*rf)->metadata == NULL) {
            free((*rf)->embedding);
            minhash_free((*rf)->minhash);
            free(*rf);
            return RAGFILE_ERROR_MEMORY;
        }
    } else {
        (*rf)->metadata = NULL;
    }

    return RAGFILE_SUCCESS;
}

void ragfile_free(RagFile* rf) {
    if (rf) {
        minhash_free(rf->minhash);
        free(rf->embedding);
        free(rf->metadata);
        free(rf);
    }
}


RagfileError ragfile_load(RagFile** rf, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        return RAGFILE_ERROR_IO;
    }

    *rf = (RagFile*)malloc(sizeof(RagFile));
    if (*rf == NULL) {
        fclose(file);
        return RAGFILE_ERROR_MEMORY;
    }

    // Read header
    if (fread(&(*rf)->header, sizeof(RagfileHeader), 1, file) != 1) {
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Verify magic number and version
    if ((*rf)->header.magic != RAGFILE_MAGIC || (*rf)->header.version != RAGFILE_VERSION) {
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_FORMAT;
    }

    // Load MinHash
    if (minhash_create(&(*rf)->minhash, (*rf)->header.minhash_size, 0) != MINHASH_SUCCESS) {
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_MEMORY;
    }
    if (fread((*rf)->minhash->signature, sizeof(uint64_t), (*rf)->header.minhash_size, file) != (*rf)->header.minhash_size) {
        minhash_free((*rf)->minhash);
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Load embedding
    (*rf)->embedding = (float*)malloc((*rf)->header.embedding_size * sizeof(float));
    if ((*rf)->embedding == NULL) {
        minhash_free((*rf)->minhash);
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_MEMORY;
    }
    if (fread((*rf)->embedding, sizeof(float), (*rf)->header.embedding_size, file) != (*rf)->header.embedding_size) {
        free((*rf)->embedding);
        minhash_free((*rf)->minhash);
        free(*rf);
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Load metadata
    if ((*rf)->header.metadata_size > 0) {
        (*rf)->metadata = (char*)malloc((*rf)->header.metadata_size + 1);
        if ((*rf)->metadata == NULL) {
            free((*rf)->embedding);
            minhash_free((*rf)->minhash);
            free(*rf);
            fclose(file);
            return RAGFILE_ERROR_MEMORY;
        }
        if (fread((*rf)->metadata, 1, (*rf)->header.metadata_size, file) != (*rf)->header.metadata_size) {
            free((*rf)->metadata);
            free((*rf)->embedding);
            minhash_free((*rf)->minhash);
            free(*rf);
            fclose(file);
            return RAGFILE_ERROR_IO;
        }
        (*rf)->metadata[(*rf)->header.metadata_size] = '\0';
    } else {
        (*rf)->metadata = NULL;
    }

    fclose(file);
    return RAGFILE_SUCCESS;
}

RagfileError ragfile_save(const RagFile* rf, const char* filename) {
    if (!rf) {
        return RAGFILE_ERROR_INVALID_ARGUMENT;
    }

    FILE* file = fopen(filename, "wb");
    if (!file) {
        return RAGFILE_ERROR_IO;
    }

    // Write header
    if (fwrite(&rf->header, sizeof(RagfileHeader), 1, file) != 1) {
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Write MinHash signature
    if (fwrite(rf->minhash->signature, sizeof(uint64_t), rf->header.minhash_size, file) != rf->header.minhash_size) {
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Write embedding
    if (fwrite(rf->embedding, sizeof(float), rf->header.embedding_size, file) != rf->header.embedding_size) {
        fclose(file);
        return RAGFILE_ERROR_IO;
    }

    // Write metadata
    if (rf->metadata && rf->header.metadata_size > 0) {
        if (fwrite(rf->metadata, 1, rf->header.metadata_size, file) != rf->header.metadata_size) {
            fclose(file);
            return RAGFILE_ERROR_IO;
        }
    }

    fclose(file);
    return RAGFILE_SUCCESS;
}

uint16_t ragfile_compute_id_hash(const char* id_string) {
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
