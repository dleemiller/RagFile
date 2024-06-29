#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "../core/ragfile.h"

typedef enum {
    FILE_IO_SUCCESS = 0,
    FILE_IO_ERROR_OPEN,
    FILE_IO_ERROR_READ,
    FILE_IO_ERROR_WRITE,
    FILE_IO_ERROR_SEEK,
    FILE_IO_ERROR_INVALID_ARGUMENT
} FileIOError;

FileIOError file_open(FILE** file, const char* filename, bool write_mode);
void file_close(FILE* file);
FileIOError file_read(FILE* file, void* buffer, size_t size, size_t count);
FileIOError file_write(FILE* file, const void* buffer, size_t size, size_t count);
FileIOError file_seek(FILE* file, long offset, int origin);

FileIOError read_ragfile_header(FILE* file, RagfileHeader* header);
FileIOError write_ragfile_header(FILE* file, const RagfileHeader* header);

FileIOError read_file_metadata(FILE* file, FileMetadata* metadata);
FileIOError write_file_metadata(FILE* file, const FileMetadata* metadata);

FileIOError read_text(FILE* file, char** text, size_t size);
FileIOError write_text(FILE* file, const char* text, size_t size);

FileIOError read_embedding(FILE* file, float* embedding, size_t size);
FileIOError write_embedding(FILE* file, const float* embedding, size_t size);

FileIOError read_metadata(FILE* file, char** metadata, size_t size);
FileIOError write_metadata(FILE* file, const char* metadata, size_t size);

#endif // FILE_IO_H
