// src/minhash_scanner.c
#define _POSIX_C_SOURCE 200809L
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "minhash_scanner.h"

// MurmurHash3 implementation
uint32_t murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    if (len > 3) {
        const uint32_t* key_x4 = (const uint32_t*)key;
        size_t i = len >> 2;
        do {
            uint32_t k = *key_x4++;
            k *= 0xcc9e2d51;
            k = (k << 15) | (k >> 17);
            k *= 0x1b873593;
            h ^= k;
            h = (h << 13) | (h >> 19);
            h = h * 5 + 0xe6546b64;
        } while (--i);
        key = (const uint8_t*)key_x4;
    }
    if (len & 3) {
        size_t i = len & 3;
        uint32_t k = 0;
        key = &key[i - 1];
        do {
            k <<= 8;
            k |= *key--;
        } while (--i);
        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> 17);
        k *= 0x1b873593;
        h ^= k;
    }
    h ^= len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

FileHeader* read_file_header(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open file");
        printf("Failed to open file: %s\n", filename);
        return NULL;
    }

    FileHeader* header = (FileHeader*)malloc(sizeof(FileHeader));
    if (header == NULL) {
        fclose(file);
        PyErr_SetString(PyExc_MemoryError, "Failed to allocate memory for header");
        printf("Failed to allocate memory for header\n");
        return NULL;
    }

    size_t read_size = fread(header, 1, sizeof(FileHeader), file);
    fclose(file);

    printf("Read size: %zu, Expected size: %zu\n", read_size, sizeof(FileHeader));
    printf("Magic number (hex): %08X, Expected: %08X\n", header->magic, MAGIC_NUMBER);
    printf("Version: %u\n", header->version);
    printf("Minhash size: %u\n", header->minhash_size);

    if (read_size != sizeof(FileHeader)) {
        free(header);
        PyErr_SetString(PyExc_IOError, "Failed to read complete header");
        return NULL;
    }

    if (header->magic != MAGIC_NUMBER) {
        free(header);
        PyErr_SetString(PyExc_ValueError, "Invalid magic number");
        return NULL;
    }

    return header;
}

uint32_t* create_minhash(const char* input, int num_hashes) {
    if (num_hashes > MAX_MINHASH_SIZE) {
        num_hashes = MAX_MINHASH_SIZE;
    }

    uint32_t* minhash = (uint32_t*)malloc(num_hashes * sizeof(uint32_t));
    if (minhash == NULL) return NULL;

    for (int i = 0; i < num_hashes; i++) {
        minhash[i] = UINT32_MAX;
    }

    size_t input_len = strlen(input);
    for (size_t i = 0; i < input_len; i++) {
        for (int j = 0; j < num_hashes; j++) {
            uint32_t hash = murmur3_32((const uint8_t*)&input[i], 1, j);
            if (hash < minhash[j]) {
                minhash[j] = hash;
            }
        }
    }

    return minhash;
}

float jaccard_similarity(uint32_t* minhash1, uint32_t* minhash2, int size) {
    int intersection = 0;
    for (int i = 0; i < size; i++) {
        if (minhash1[i] == minhash2[i]) {
            intersection++;
        }
    }
    return (float)intersection / size;
}

MatchResult* scan_files(const char* directory, uint32_t* query_minhash, int minhash_size, float threshold, int* num_matches) {
    DIR* dir;
    struct dirent* entry;
    MatchResult* matches = NULL;
    int matches_capacity = 10;
    int matches_count = 0;

    matches = (MatchResult*)malloc(matches_capacity * sizeof(MatchResult));
    if (matches == NULL) {
        *num_matches = 0;
        return NULL;
    }

    dir = opendir(directory);
    if (dir == NULL) {
        free(matches);
        *num_matches = 0;
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".mhf") == NULL) continue;

        char filepath[1024];
        snprintf(filepath, sizeof(filepath), "%s/%s", directory, entry->d_name);

        FileHeader* header = read_file_header(filepath);
        if (header == NULL || header->minhash_size != (uint32_t)minhash_size) {
            free(header);
            continue;
        }

        FILE* file = fopen(filepath, "rb");
        if (file == NULL) {
            free(header);
            continue;
        }

        fseek(file, sizeof(FileHeader), SEEK_SET);
        uint32_t* file_minhash = (uint32_t*)malloc(minhash_size * sizeof(uint32_t));
        if (file_minhash == NULL) {
            fclose(file);
            free(header);
            continue;
        }

        if (fread(file_minhash, sizeof(uint32_t), minhash_size, file) != minhash_size) {
            // Handle error (e.g., file read failed)
            fclose(file);
            free(file_minhash);
            free(header);
            continue;
        }
        fclose(file);

        float similarity = jaccard_similarity(query_minhash, file_minhash, minhash_size);

        if (similarity >= threshold) {
            if (matches_count == matches_capacity) {
                matches_capacity *= 2;
                MatchResult* new_matches = (MatchResult*)realloc(matches, matches_capacity * sizeof(MatchResult));
                if (new_matches == NULL) {
                    free(file_minhash);
                    free(header);
                    break;
                }
                matches = new_matches;
            }
            matches[matches_count].filename = strdup(filepath);
            matches[matches_count].similarity = similarity;
            matches_count++;
        }

        free(file_minhash);
        free(header);
    }

    closedir(dir);
    *num_matches = matches_count;
    return matches;
}

void free_matches(MatchResult* matches, int num_matches) {
    for (int i = 0; i < num_matches; i++) {
        free(matches[i].filename);
    }
    free(matches);
}
