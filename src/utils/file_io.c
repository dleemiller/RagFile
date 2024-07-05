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

FileIOError read_text(FILE* file, char** text, size_t size) {
    if (size == 0) {
        *text = NULL;
        return FILE_IO_SUCCESS;
    }

    *text = (char*)calloc(size + 1, 1);  // Allocate with zero-initialization
    if (*text == NULL) {
        return FILE_IO_ERROR_MEMORY;
    }

    size_t read_count = fread(*text, 1, size, file);
    if (read_count != size) {
        free(*text);
        *text = NULL;
        return FILE_IO_ERROR_READ;  // or FILE_IO_ERROR_INCOMPLETE_READ if partial reads are acceptable
    }

    (*text)[size] = '\0';  // Ensure null-termination
    return FILE_IO_SUCCESS;
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

