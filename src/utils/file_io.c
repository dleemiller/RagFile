#include "file_io.h"
#include <stdlib.h>
#include <string.h>

FileIOError file_open(FILE** file, const char* filename, bool write_mode) {
    *file = fopen(filename, write_mode ? "wb" : "rb");
    return *file ? FILE_IO_SUCCESS : FILE_IO_ERROR_OPEN;
}

void file_close(FILE* file) {
    if (file) {
        fclose(file);
    }
}

FileIOError file_read(FILE* file, void* buffer, size_t size, size_t count) {
    return fread(buffer, size, count, file) == count ? FILE_IO_SUCCESS : FILE_IO_ERROR_READ;
}

FileIOError file_write(FILE* file, const void* buffer, size_t size, size_t count) {
    return fwrite(buffer, size, count, file) == count ? FILE_IO_SUCCESS : FILE_IO_ERROR_WRITE;
}

FileIOError file_seek(FILE* file, long offset, int origin) {
    return fseek(file, offset, origin) == 0 ? FILE_IO_SUCCESS : FILE_IO_ERROR_SEEK;
}

FileIOError read_ragfile_header(FILE* file, RagfileHeader* header) {
    return file_read(file, header, sizeof(RagfileHeader), 1);
}

FileIOError write_ragfile_header(FILE* file, const RagfileHeader* header) {
    return file_write(file, header, sizeof(RagfileHeader), 1);
}

FileIOError read_file_metadata(FILE* file, FileMetadata* metadata) {
    if (fread(metadata, sizeof(FileMetadata), 1, file) != 1) {
        return FILE_IO_ERROR_READ;
    }
    return FILE_IO_SUCCESS;
}

FileIOError write_file_metadata(FILE* file, const FileMetadata* metadata) {
    if (fwrite(metadata, sizeof(FileMetadata), 1, file) != 1) {
        return FILE_IO_ERROR_WRITE;
    }
    return FILE_IO_SUCCESS;
}

FileIOError read_text(FILE* file, char** text, size_t size) {
    *text = (char*)malloc(size + 1);
    if (*text == NULL) {
        return FILE_IO_ERROR_INVALID_ARGUMENT;
    }
    FileIOError result = file_read(file, *text, 1, size);
    if (result == FILE_IO_SUCCESS) {
        (*text)[size] = '\0';
    } else {
        free(*text);
        *text = NULL;
    }
    return result;
}

FileIOError write_text(FILE* file, const char* text, size_t size) {
    return file_write(file, text, 1, size);
}

FileIOError read_embedding(FILE* file, float* embedding, size_t size) {
    return file_read(file, embedding, sizeof(float), size);
}

FileIOError write_embedding(FILE* file, const float* embedding, size_t size) {
    return file_write(file, embedding, sizeof(float), size);
}

FileIOError read_metadata(FILE* file, char** metadata, size_t size) {
    return read_text(file, metadata, size);
}

FileIOError write_metadata(FILE* file, const char* metadata, size_t size) {
    return write_text(file, metadata, size);
}
