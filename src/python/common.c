#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include <unistd.h>
#include "../include/config.h"
#include "../core/ragfile.h"
#include "../core/minhash.h"
#include "../algorithms/jaccard.h"
#include "../algorithms/hamming.h"
#include "../algorithms/cosine.h"
#include "../search/heap.h"
#include "../search/scan.h"
#include "common.h"

// Forward declarations for header and file type deallocations
static void PyRagFileHeader_dealloc(PyRagFileHeader* self);
static void PyRagFile_dealloc(PyRagFile* self);

// Initialize PyRagFile
static int PyRagFile_shared_init(PyRagFile* self, RagFile* rf, int is_loaded, PyTypeObject* header_type);
static int PyRagFile_init(PyRagFile* self, PyObject* args, PyObject* kwds);

// Deallocate PyRagFileHeader
static void PyRagFileHeader_dealloc(PyRagFileHeader* self) {
    if (self->header) {
        printf("PyRagFileHeader_dealloc: Deallocating header: %p\n", (void*)self->header);
        self->header = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Deallocate PyRagFile
static void PyRagFile_dealloc(PyRagFile* self) {
    if (self->rf) {
        printf("PyRagFile_dealloc: Deallocating rf: %p\n", (void*)self->rf);
        ragfile_free(self->rf);
        self->rf = NULL;
    }
    Py_XDECREF(self->header);
    Py_XDECREF(self->file_metadata);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Create a new PyRagFile
PyObject* PyRagFile_New(PyTypeObject* type, RagFile* rf, PyTypeObject* header_type) {
    printf("in PyRagFile_New\n");
    PyRagFile* obj = (PyRagFile*)type->tp_alloc(type, 0);
    if (!obj) {
        return PyErr_NoMemory();
    }
    printf("setting objects\n");
    obj->rf = rf;
    obj->header = NULL;
    obj->file_metadata = NULL;
    printf("calling custom init for deserialization\n");

    // Initialize the object using the shared init method for deserialization
    if (PyRagFile_shared_init(obj, rf, 1, header_type) < 0) {  // Passing 1 to indicate the object is already loaded
        Py_DECREF(obj);
        return NULL;
    }

    printf("returning obj\n");

    return (PyObject*)obj;
}

// Create a new PyRagFileHeader
PyObject* PyRagFileHeader_New(PyTypeObject* type, RagfileHeader* header) {
    PyRagFileHeader* obj = (PyRagFileHeader*)type->tp_alloc(type, 0);
    if (!obj)
        return PyErr_NoMemory();
    obj->header = header;
    return (PyObject*)obj;
}

// Validate embeddings and tokens, and prepare the embeddings array
static int prepare_embeddings(PyObject* embeddings_obj, float** flattened, size_t* total_floats, uint32_t* num_embeddings, uint32_t* embedding_dim) {
    if (!PyList_Check(embeddings_obj)) {
        PyErr_SetString(PyExc_TypeError, "embeddings must be a list of lists of floats");
        return 0;
    }

    *num_embeddings = PyList_Size(embeddings_obj);
    if (*num_embeddings == 0) {
        PyErr_SetString(PyExc_ValueError, "embeddings list cannot be empty");
        return 0;
    }

    // Check the first embedding to determine the dimension
    PyObject* first_emb = PyList_GetItem(embeddings_obj, 0);
    if (!PyList_Check(first_emb)) {
        PyErr_SetString(PyExc_TypeError, "Each embedding must be a list of floats");
        return 0;
    }

    *embedding_dim = PyList_Size(first_emb);
    *total_floats = *num_embeddings * *embedding_dim;
    *flattened = (float*)malloc(*total_floats * sizeof(float));
    if (*flattened == NULL) {
        PyErr_NoMemory();
        return 0;
    }

    float* ptr = *flattened;
    for (Py_ssize_t i = 0; i < *num_embeddings; i++) {
        PyObject* emb_list = PyList_GetItem(embeddings_obj, i);
        if (!PyList_Check(emb_list) || PyList_Size(emb_list) != *embedding_dim) {
            PyErr_SetString(PyExc_TypeError, "All embeddings must be lists of floats of the same length");
            free(*flattened);
            return 0;
        }
        for (Py_ssize_t j = 0; j < *embedding_dim; j++) {
            PyObject* float_obj = PyList_GetItem(emb_list, j);
            if (!PyFloat_Check(float_obj)) {
                PyErr_SetString(PyExc_TypeError, "Embedding values must be floats");
                free(*flattened);
                return 0;
            }
            *ptr++ = (float)PyFloat_AsDouble(float_obj);
        }
    }
    return 1;
}

// Initialize PyRagFile with shared logic for both creation and deserialization
static int PyRagFile_shared_init(PyRagFile* self, RagFile* rf, int is_loaded, PyTypeObject* header_type) {
    self->rf = rf;

    self->file_metadata = PyDict_New();
    if (!self->file_metadata) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create file metadata dictionary");
        return -1;
    }

    // Use the provided type for the header
    PyObject* header_obj = PyRagFileHeader_New(header_type, &(self->rf->header));
    if (!header_obj) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize RagFile header");
        return -1;
    }
    self->header = (PyRagFileHeader*)header_obj;

    // Debug statements
    printf("PyRagFile_shared_init: self->header: %p\n", (void*)self->header);
    printf("PyRagFile_shared_init: self->header->header: %p\n", (void*)self->header->header);
    printf("PyRagFile_shared_init: self->header->header->version: %d\n", self->header->header->version);
    printf("PyRagFile_shared_init: self->header->header->tokenizer_id_hash: %u\n", self->header->header->tokenizer_id_hash);
    printf("PyRagFile_shared_init: self->header->header->embedding_id_hash: %u\n", self->header->header->embedding_id_hash);

    if (!self->header->header) {
        printf("PyRagFile_shared_init: Failed to set self->header->header\n");
        return -1;
    }

    // Populate file_metadata only if the object is already loaded
    if (is_loaded) {
        PyDict_SetItemString(self->file_metadata, "tokenizer_id", PyUnicode_FromString(self->rf->file_metadata.tokenizer_id));
        PyDict_SetItemString(self->file_metadata, "embedding_id", PyUnicode_FromString(self->rf->file_metadata.embedding_id));
        PyDict_SetItemString(self->file_metadata, "metadata_version", PyLong_FromUnsignedLong(self->rf->file_metadata.metadata_version));
        PyDict_SetItemString(self->file_metadata, "num_embeddings", PyLong_FromUnsignedLong(self->rf->file_metadata.num_embeddings));
        PyDict_SetItemString(self->file_metadata, "embedding_dim", PyLong_FromUnsignedLong(self->rf->file_metadata.embedding_dim));
    }

    return 0;
}

// Initialize PyRagFile for creation from scratch
static int PyRagFile_init(PyRagFile* self, PyObject* args, PyObject* kwds) {
    const char* text = NULL;
    PyObject* token_ids_obj = NULL;
    PyObject* embeddings_obj = NULL;
    const char* extended_metadata = NULL;
    const char* tokenizer_id = NULL;
    const char* embedding_id = NULL;
    uint16_t metadata_version = 0;
    uint32_t num_embeddings = 0;
    uint32_t embedding_dim = 0;
    int is_loaded = 0;

    static char* kwlist[] = {"text", "token_ids", "embeddings", "extended_metadata", "tokenizer_id", "embedding_id", "metadata_version", "is_loaded", NULL};

    printf("PyRagFile_init: Parsing arguments\n");

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOOsssHi", kwlist,
                                     &text, &token_ids_obj, &embeddings_obj, &extended_metadata,
                                     &tokenizer_id, &embedding_id, &metadata_version, &is_loaded)) {
        return -1; // Error handling if arguments are not correctly parsed
    }

    printf("PyRagFile_init: Arguments parsed\n");

    if (!is_loaded && (!text || !token_ids_obj || !embeddings_obj || !tokenizer_id || !embedding_id)) {
        PyErr_SetString(PyExc_ValueError, "Missing required arguments");
        return -1;
    }

    uint32_t* token_ids = NULL;
    float* flattened_embeddings = NULL;
    size_t total_floats = 0;

    if (!is_loaded) {
        // Token IDs conversion
        Py_ssize_t num_tokens = PyList_Size(token_ids_obj);
        token_ids = (uint32_t*)malloc(num_tokens * sizeof(uint32_t));
        if (!token_ids) {
            PyErr_NoMemory();
            return -1;
        }
        for (Py_ssize_t i = 0; i < num_tokens; i++) {
            PyObject* item = PyList_GetItem(token_ids_obj, i);
            if (!PyLong_Check(item)) {
                free(token_ids);
                PyErr_SetString(PyExc_TypeError, "Token IDs must be integers");
                return -1;
            }
            token_ids[i] = (uint32_t)PyLong_AsUnsignedLong(item);
        }

        printf("PyRagFile_init: Converted token IDs\n");

        // Embeddings preparation
        if (!prepare_embeddings(embeddings_obj, &flattened_embeddings, &total_floats, &num_embeddings, &embedding_dim)) {
            free(token_ids);
            return -1; // Validate and prepare the embeddings array
        }

        printf("PyRagFile_init: Prepared embeddings\n");

        // RagFile creation
        RagfileError error = ragfile_create(&self->rf, text, token_ids, num_tokens,
                                            flattened_embeddings, total_floats, extended_metadata,
                                            tokenizer_id, embedding_id, metadata_version, num_embeddings, embedding_dim);
        free(token_ids);
        free(flattened_embeddings);

        if (error != RAGFILE_SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create RagFile");
            return -1;
        }

        printf("PyRagFile_init: Created RagFile\n");
    }

    // Call the shared initialization logic
    return PyRagFile_shared_init(self, self->rf, is_loaded, &PyRagFileHeaderType);
}

// Methods for similarity calculations
static PyObject* PyRagFile_jaccard(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    if (!PyArg_ParseTuple(args, "O!", Py_TYPE(self), &other)) {
        return NULL;
    }

    float similarity = jaccard_similarity(self->rf->header.minhash_signature, 
                                          other->rf->header.minhash_signature);
    return PyFloat_FromDouble(similarity);
}

static PyObject* PyRagFile_hamming(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    if (!PyArg_ParseTuple(args, "O!", Py_TYPE(self), &other)) {
        return NULL;
    }

    float similarity = hamming_similarity(self->rf->header.binary_embedding, 
                                          other->rf->header.binary_embedding,
                                          self->rf->file_metadata.embedding_dim);
    return PyFloat_FromDouble(similarity);
}

// Compute Cosine Similarity with options for max or avg
static PyObject* PyRagFile_cosine(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    const char* mode = "max";

    if (!PyArg_ParseTuple(args, "O!|s", &PyRagFileType, &other, &mode)) {
        return NULL;
    }

    // Assume embedding dimension and number of embeddings are part of file_metadata or a similar accessible structure
    int embedding_dim = self->rf->file_metadata.embedding_dim;
    int num_embeddings_self = self->rf->file_metadata.num_embeddings;
    int num_embeddings_other = other->rf->file_metadata.num_embeddings;

    float max_similarity = -1.0f;
    float total_similarity = 0.0f;
    int count = 0;

    for (int i = 0; i < num_embeddings_self; i++) {
        for (int j = 0; j < num_embeddings_other; j++) {
            float* emb1 = self->rf->embeddings + (i * embedding_dim);
            float* emb2 = other->rf->embeddings + (j * embedding_dim);
            float similarity = cosine_similarity(emb1, emb2, embedding_dim);
            max_similarity = fmax(max_similarity, similarity);
            total_similarity += similarity;
            count++;
        }
    }

    float result = strcmp(mode, "avg") == 0 ? total_similarity / count : max_similarity;
    return PyFloat_FromDouble(result);
}

// Scanning
static PyObject* PyRagFile_match(PyRagFile* self, PyObject* args, PyObject* kwds) {
    PyObject* file_iter;
    unsigned int top_k;

    static char *kwlist[] = {"file_iter", "top_k", NULL};

    // Parse Python keyword arguments
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OI", kwlist, &file_iter, &top_k)) {
        return NULL;
    }

    if (!PyIter_Check(file_iter)) {
        PyErr_SetString(PyExc_TypeError, "file_iter must be an iterator");
        return NULL;
    }

    if (top_k == 0) {
        PyErr_SetString(PyExc_ValueError, "top_k must be greater than 0");
        return NULL;
    }

    MinHeap* heap = create_min_heap(top_k);
    if (heap == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create a heap");
        return NULL;
    }

    PyObject* file_path;
    while ((file_path = PyIter_Next(file_iter)) != NULL) {
        const char* path = PyUnicode_AsUTF8(file_path);
        if (path == NULL) {
            PyErr_Format(PyExc_ValueError, "Invalid file path encountered");
            Py_DECREF(file_path);
            continue;
        }

        int process_status = process_file(path, self->rf, heap);
        Py_DECREF(file_path);
        if (process_status != 0) {
            PyErr_SetString(PyExc_RuntimeError, "File processing failed");
            free_min_heap(heap);
            return NULL;
        }
    }

    PyObject* result_list = PyList_New(0);
    if (result_list == NULL) {
        free_min_heap(heap);
        PyErr_SetString(PyExc_MemoryError, "Failed to create list");
        return NULL;
    }

    // Extract elements from the heap and append them to the Python list
    while (heap->size > 0) {
        FileScore min_score = heap->heap[0];  // Get the root, which has the minimum score
        PyObject* dict = Py_BuildValue("{s:s, s:f}", "file", min_score.path, "jaccard", min_score.score);
        if (PyList_Append(result_list, dict) == -1) {
            Py_XDECREF(dict);
            Py_DECREF(result_list);
            free_min_heap(heap);
            PyErr_SetString(PyExc_MemoryError, "Failed to append to list");
            return NULL;
        }
        Py_DECREF(dict);
        remove_root(heap);  // Remove the root to get the next item
    }

    // Now reverse the list to get it in descending order of score
    if (PyList_Reverse(result_list) == -1) {
        Py_DECREF(result_list);
        free_min_heap(heap);
        PyErr_SetString(PyExc_RuntimeError, "Failed to reverse the list");
        return NULL;
    }

    free_min_heap(heap);
    return result_list;
}

// Getter methods for RagFile

static PyObject* PyRagFile_get_text(PyRagFile* self, void* closure) {
    return PyUnicode_FromString(self->rf->text);
}

static PyObject* PyRagFile_get_embeddings(PyRagFile* self, void* closure) {
    int num_embeddings = self->rf->file_metadata.num_embeddings;
    int embedding_dim = self->rf->file_metadata.embedding_dim;

    // Creating the outer list that will contain all embeddings
    PyObject* embeddings_list = PyList_New(num_embeddings);
    if (!embeddings_list) return PyErr_NoMemory();

    // Calculate the base index for each embedding and create lists for them
    for (int i = 0; i < num_embeddings; i++) {
        PyObject* single_embedding = PyList_New(embedding_dim);
        if (!single_embedding) {
            Py_DECREF(embeddings_list);
            return PyErr_NoMemory();
        }
        
        // Accessing the correct elements in the flat array
        for (int j = 0; j < embedding_dim; j++) {
            int idx = i * embedding_dim + j;  // Correct index in the flat array
            PyObject* float_obj = PyFloat_FromDouble(self->rf->embeddings[idx]);
            PyList_SET_ITEM(single_embedding, j, float_obj); // Takes ownership of the float_obj reference
        }

        PyList_SET_ITEM(embeddings_list, i, single_embedding);  // Takes ownership of the single_embedding reference
    }

    return embeddings_list;
}

// Getter method for RagFile header
static PyObject* PyRagFile_get_header(PyRagFile* self, void* closure) {
    printf("Entering PyRagFile_get_header\n");
    if (!self) {
        printf("PyRagFile_get_header: self is NULL\n");
        Py_RETURN_NONE;
    }
    if (!self->header) {
        printf("PyRagFile_get_header: self->header is NULL\n");
        Py_RETURN_NONE;
    }
    if (!self->header->header) {
        printf("PyRagFile_get_header: self->header->header is NULL\n");
        Py_RETURN_NONE;
    }

    // Print details about the header and its contents
    printf("PyRagFile_get_header: self->header: %p\n", (void*)self->header);
    printf("PyRagFile_get_header: self->header->header: %p\n", (void*)self->header->header);
    printf("PyRagFile_get_header: self->header->header->version: %d\n", self->header->header->version);
    printf("PyRagFile_get_header: self->header->header->tokenizer_id_hash: %u\n", self->header->header->tokenizer_id_hash);
    printf("PyRagFile_get_header: self->header->header->embedding_id_hash: %u\n", self->header->header->embedding_id_hash);

    Py_INCREF(self->header);
    printf("Returning header: %p\n", (void*)self->header);
    return (PyObject*)self->header;
}

// Getter methods for RagFileHeader

static PyObject* PyRagFileHeader_get_version(PyRagFileHeader* self, void* closure) {
    if (self == NULL || self->header == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Header is NULL");
        return NULL;
    }
    return PyLong_FromUnsignedLong((unsigned long)self->header->version);
}

static PyObject* PyRagFileHeader_get_tokenizer_hash(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->tokenizer_id_hash);
}

static PyObject* PyRagFileHeader_get_embedding_hash(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->embedding_id_hash);
}

static PyObject* PyRagFileHeader_get_minhash_signature(PyRagFileHeader* self, void* closure) {
    PyObject* signature = PyList_New(MINHASH_SIZE);
    if (signature == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < MINHASH_SIZE; i++) {
        PyObject* item = PyLong_FromUnsignedLong(self->header->minhash_signature[i]);
        if (!item) {  // Always check for failure!
            Py_DECREF(signature);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(signature, i, item);  // Transfers ownership to list
    }
    return signature;
}

static PyObject* PyRagFileHeader_get_binary_embedding(PyRagFileHeader* self, void* closure) {
    PyObject* binary_embedding = PyList_New(BINARY_EMBEDDING_BYTE_DIM);
    if (binary_embedding == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < BINARY_EMBEDDING_BYTE_DIM; i++) {
        PyObject* item = PyLong_FromUnsignedLong(self->header->binary_embedding[i]);
        if (!item) {  // Always check for failure!
            Py_DECREF(binary_embedding);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(binary_embedding, i, item);  // Transfers ownership to list
    }
    return binary_embedding;
}

static PyObject* PyRagFile_get_extended_metadata(PyRagFile* self, void* closure) {
    if (self->rf->extended_metadata == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(self->rf->extended_metadata);
}

static PyObject* PyRagFile_get_file_metadata(PyRagFile* self, void* closure) {
    Py_INCREF(self->file_metadata);
    return (PyObject*)self->file_metadata;
}

// Method definitions
static PyMethodDef PyRagFile_methods[] = {
    {"jaccard", (PyCFunction)PyRagFile_jaccard, METH_VARARGS, "Compute Jaccard similarity with another RagFile"},
    {"hamming", (PyCFunction)PyRagFile_hamming, METH_VARARGS, "Compute Hamming similarity from the binary embedding"},
    {"cosine", (PyCFunction)PyRagFile_cosine, METH_VARARGS | METH_KEYWORDS, "Compute Cosine similarity with another RagFile"},
    {"match", (PyCFunction)PyRagFile_match, METH_VARARGS | METH_KEYWORDS, "Find matches in a directory using Jaccard similarity"},
    {NULL}  /* Sentinel */
};

static PyGetSetDef PyRagFile_getsetters[] = {
    {"text", (getter)PyRagFile_get_text, NULL, "Get the text content", NULL},
    {"embeddings", (getter)PyRagFile_get_embeddings, NULL, "Get the embeddings", NULL},
    {"extended_metadata", (getter)PyRagFile_get_extended_metadata, NULL, "Get the extended metadata", NULL},
    {"header", (getter)PyRagFile_get_header, NULL, "Get the header object", NULL},
    {"file_metadata", (getter)PyRagFile_get_file_metadata, NULL, "Get the file metadata", NULL},
    {NULL}  /* Sentinel */
};

static PyGetSetDef PyRagFileHeader_getsetters[] = {
    {"version", (getter)PyRagFileHeader_get_version, NULL, "Get the version", NULL},
    {"tokenizer_id_hash", (getter)PyRagFileHeader_get_tokenizer_hash, NULL, "Get the CRC 16 hash of the tokenzier id", NULL},
    {"embedding_id_hash", (getter)PyRagFileHeader_get_embedding_hash, NULL, "Get the CRC 16 hash of the embedding id", NULL},
    {"binary_embedding", (getter)PyRagFileHeader_get_binary_embedding, NULL, "Get the Binary Embedding", NULL},
    {"minhash_signature", (getter)PyRagFileHeader_get_minhash_signature, NULL, "Get the MinHash signature", NULL},
    {NULL}  /* Sentinel */
};

// RagFile type definition
PyTypeObject PyRagFileType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ragfile.RagFile",
    .tp_doc = "RagFile object",
    .tp_basicsize = sizeof(PyRagFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_alloc = PyType_GenericAlloc,  // Ensure tp_alloc is set
    .tp_init = (initproc)PyRagFile_init,
    .tp_dealloc = (destructor)PyRagFile_dealloc,
    .tp_methods = PyRagFile_methods,
    .tp_getset = PyRagFile_getsetters,
};

// RagFileHeader type definition
PyTypeObject PyRagFileHeaderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ragfile.RagFileHeader",
    .tp_doc = "RagFile header object",
    .tp_basicsize = sizeof(PyRagFileHeader),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_alloc = PyType_GenericAlloc,  // Ensure tp_alloc is set
    .tp_dealloc = (destructor)PyRagFileHeader_dealloc,
    .tp_getset = PyRagFileHeader_getsetters,
};

