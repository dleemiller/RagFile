#ifndef CONFIG_H
#define CONFIG_H

#define RAGFILE_MAGIC 0x52414746 // "RAGF" in ASCII
#define RAGFILE_VERSION 1

#define MODEL_ID_SIZE 64
#define METADATA_MAX_SIZE 1024
#define SCAN_VEC_DIM 256
#define DENSE_VEC_DIM 1024

#ifndef BINARY_EMBEDDING_DIM
#define BINARY_EMBEDDING_DIM 128 // Default dimension
#endif

#define BINARY_EMBEDDING_BYTE_DIM (BINARY_EMBEDDING_DIM / 8)  // Number of bytes.

#endif // CONFIG_H
