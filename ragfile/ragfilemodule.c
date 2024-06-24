#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <unistd.h>
#include "../src/core/ragfile.h"
#include "../src/core/minhash.h"
#include "../src/algorithms/jaccard.h"
#include "../src/algorithms/cosine.h"

// Define types
static PyTypeObject PyRagFileType;
static PyTypeObject PyRagFileHeaderType;

// RagFile header object
typedef struct {
    PyObject_HEAD
    RagfileHeader* header;
} PyRagFileHeader;

// RagFile object
typedef struct {
    PyObject_HEAD
    RagFile* rf;
    PyRagFileHeader* header;
} PyRagFile;

// Forward declarations for header and file type deallocations
static void PyRagFileHeader_dealloc(PyRagFileHeader* self);
static void PyRagFile_dealloc(PyRagFile* self);

// Initialize PyRagFile
static int PyRagFile_init(PyRagFile* self, PyObject* args, PyObject* kwds);

// Function declarations for module-level operations
static PyObject* PyRagFile_from_RagFile(RagFile* rf);
static PyObject* py_ragfile_load(PyObject* self, PyObject* args);
static PyObject* py_ragfile_dump(PyObject* self, PyObject* args);
static PyObject* py_ragfile_loads(PyObject* self, PyObject* args);
static PyObject* py_ragfile_dumps(PyObject* self, PyObject* args);

// Deallocate PyRagFileHeader
static void PyRagFileHeader_dealloc(PyRagFileHeader* self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Deallocate PyRagFile
static void PyRagFile_dealloc(PyRagFile* self) {
    ragfile_free(self->rf);
    Py_XDECREF(self->header);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Initialize PyRagFile
static int PyRagFile_init(PyRagFile* self, PyObject* args, PyObject* kwds) {
    printf("Entering PyRagFile_init\n");

    const char* text = NULL;
    PyObject* token_ids_obj = NULL;
    PyObject* embedding_obj = NULL;
    const char* metadata = NULL;
    PyObject* tokenizer_id_obj = NULL;
    PyObject* embedding_id_obj = NULL;
    uint16_t metadata_version = 0;
    int is_loaded = 0;

    static char* kwlist[] = {"text", "token_ids", "embedding", "metadata", "tokenizer_id", "embedding_id", "metadata_version", "is_loaded", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOOsOOHi", kwlist, &text, &token_ids_obj, &embedding_obj, &metadata, &tokenizer_id_obj, &embedding_id_obj, &metadata_version, &is_loaded)) {
        printf("Failed to parse arguments\n");
        return -1;
    }

    // Check required arguments if not loaded
    if (!is_loaded && (text == NULL || token_ids_obj == NULL || embedding_obj == NULL || tokenizer_id_obj == NULL || embedding_id_obj == NULL)) {
        PyErr_SetString(PyExc_ValueError, "Missing required arguments: text, token_ids, embedding, tokenizer_id, and embedding_id are required");
        return -1;
    }

    // Validate the types of token_ids and embedding if not loaded
    if (!is_loaded && token_ids_obj && !PyList_Check(token_ids_obj)) {
        PyErr_SetString(PyExc_TypeError, "token_ids must be a list");
        return -1;
    }

    if (!is_loaded && embedding_obj && !PyList_Check(embedding_obj)) {
        PyErr_SetString(PyExc_TypeError, "embedding must be a list");
        return -1;
    }

    // Validate that token_ids are integers if not loaded
    if (!is_loaded && token_ids_obj) {
        Py_ssize_t token_count = PyList_Size(token_ids_obj);
        for (Py_ssize_t i = 0; i < token_count; i++) {
            if (!PyLong_Check(PyList_GetItem(token_ids_obj, i))) {
                PyErr_SetString(PyExc_TypeError, "token_ids must be a list of integers");
                return -1;
            }
        }
    }

    // Validate that embedding are floats if not loaded
    if (!is_loaded && embedding_obj) {
        Py_ssize_t embedding_size = PyList_Size(embedding_obj);
        for (Py_ssize_t i = 0; i < embedding_size; i++) {
            if (!PyFloat_Check(PyList_GetItem(embedding_obj, i))) {
                PyErr_SetString(PyExc_TypeError, "embedding must be a list of floats");
                return -1;
            }
        }
    }

    // Validate the length of token_ids if not loaded
    if (!is_loaded && PyList_Size(token_ids_obj) < 3) {
        PyErr_SetString(PyExc_ValueError, "token_ids must contain at least 3 elements for the MinHash algorithm to work");
        return -1;
    }

    // Initialize with zeros to avoid uninitialized memory issues
    uint16_t tokenizer_id_hash = 0;
    uint16_t embedding_id_hash = 0;

    if (tokenizer_id_obj) {
        if (PyUnicode_Check(tokenizer_id_obj)) {
            const char* tokenizer_id_str = PyUnicode_AsUTF8(tokenizer_id_obj);
            tokenizer_id_hash = ragfile_compute_id_hash(tokenizer_id_str);
        } else if (PyLong_Check(tokenizer_id_obj)) {
            tokenizer_id_hash = (uint16_t)PyLong_AsUnsignedLong(tokenizer_id_obj);
        } else {
            PyErr_SetString(PyExc_TypeError, "tokenizer_id must be a string or an integer");
            printf("Invalid tokenizer_id type\n");
            return -1;
        }
    }

    if (embedding_id_obj) {
        if (PyUnicode_Check(embedding_id_obj)) {
            const char* embedding_id_str = PyUnicode_AsUTF8(embedding_id_obj);
            embedding_id_hash = ragfile_compute_id_hash(embedding_id_str);
        } else if (PyLong_Check(embedding_id_obj)) {
            embedding_id_hash = (uint16_t)PyLong_AsUnsignedLong(embedding_id_obj);
        } else {
            PyErr_SetString(PyExc_TypeError, "embedding_id must be a string or an integer");
            printf("Invalid embedding_id type\n");
            return -1;
        }
    }

    printf("tokenizer_id_hash: %u, embedding_id_hash: %u\n", tokenizer_id_hash, embedding_id_hash);

    if (!is_loaded) {
        Py_ssize_t token_count = PyList_Size(token_ids_obj);
        uint32_t* token_ids = malloc(token_count * sizeof(uint32_t));
        if (token_ids == NULL) {
            PyErr_NoMemory();
            printf("Failed to allocate memory for token_ids\n");
            return -1;
        }
        for (Py_ssize_t i = 0; i < token_count; i++) {
            token_ids[i] = PyLong_AsUnsignedLong(PyList_GetItem(token_ids_obj, i));
        }

        Py_ssize_t embedding_size = PyList_Size(embedding_obj);
        float* embedding = malloc(embedding_size * sizeof(float));
        if (embedding == NULL) {
            free(token_ids);
            PyErr_NoMemory();
            printf("Failed to allocate memory for embedding\n");
            return -1;
        }
        for (Py_ssize_t i = 0; i < embedding_size; i++) {
            embedding[i] = (float)PyFloat_AsDouble(PyList_GetItem(embedding_obj, i));
        }

        RagfileError error = ragfile_create(&self->rf, text, token_ids, token_count, 
                                            embedding, embedding_size, metadata, 
                                            tokenizer_id_hash, embedding_id_hash, metadata_version);

        free(token_ids);
        free(embedding);

        if (error != RAGFILE_SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create RagFile");
            printf("Failed to create RagFile\n");
            return -1;
        }
    }

    // Check and initialize header properly
    if (self->header == NULL) {
        self->header = (PyRagFileHeader*)PyObject_New(PyRagFileHeader, &PyRagFileHeaderType);
        if (self->header == NULL) {
            printf("Failed to create PyRagFileHeader\n");
            return -1;
        }
    }
    self->header->header = &(self->rf->header);

    printf("PyRagFile initialized with tokenizer_id_hash: %u and embedding_id_hash: %u\n",
           tokenizer_id_hash, embedding_id_hash);

    return 0;
}

static PyObject* PyRagFile_from_RagFile(RagFile* rf) {
    printf("Creating PyRagFile from RagFile\n");

    if (rf == NULL) {
        printf("Error: RagFile is NULL\n");
        return NULL;
    }

    PyObject* py_rf_args = Py_BuildValue("()");  // Empty args
    PyObject* py_rf_kw = Py_BuildValue("{s:i}", "is_loaded", 1);  // Keyword arguments with is_loaded flag

    PyRagFile* py_rf = (PyRagFile*)PyObject_Call((PyObject*)&PyRagFileType, py_rf_args, py_rf_kw);

    Py_DECREF(py_rf_args);
    Py_DECREF(py_rf_kw);

    if (py_rf == NULL) {
        printf("Failed to create PyRagFile object\n");
        return NULL;
    }

    py_rf->rf = rf;

    printf("Creating PyRagFileHeader\n");
    py_rf->header = (PyRagFileHeader*)PyObject_New(PyRagFileHeader, &PyRagFileHeaderType);
    if (py_rf->header == NULL) {
        Py_DECREF(py_rf);
        printf("Failed to create PyRagFileHeader\n");
        return NULL;
    }
    py_rf->header->header = &(py_rf->rf->header);

    printf("Successfully created PyRagFile object\n");
    return (PyObject*)py_rf;
}

// Load RagFile from file object
static PyObject* py_ragfile_load(PyObject* self, PyObject* args) {
    PyObject* file_obj;
    if (!PyArg_ParseTuple(args, "O", &file_obj)) {
        return NULL;
    }

    int fd = PyObject_AsFileDescriptor(file_obj);
    if (fd == -1) {
        PyErr_SetString(PyExc_TypeError, "Expected a file-like object");
        return NULL;
    }

    FILE* file = fdopen(dup(fd), "rb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open file");
        return NULL;
    }

    RagFile* rf;
    RagfileError error = ragfile_load(&rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        PyErr_SetString(PyExc_IOError, "Failed to load RagFile");
        return NULL;
    }

    return PyRagFile_from_RagFile(rf);
}

// Dump RagFile to file object
static PyObject* py_ragfile_dump(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;
    PyObject* file_obj;
    if (!PyArg_ParseTuple(args, "OO", &py_rf, &file_obj)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(py_rf, &PyRagFileType)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a RagFile object");
        return NULL;
    }

    int fd = PyObject_AsFileDescriptor(file_obj);
    if (fd == -1) {
        PyErr_SetString(PyExc_TypeError, "Expected a file-like object");
        return NULL;
    }

    FILE* file = fdopen(dup(fd), "wb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open file");
        return NULL;
    }

    RagfileError error = ragfile_save(py_rf->rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        PyErr_SetString(PyExc_IOError, "Failed to save RagFile");
        return NULL;
    }

    Py_RETURN_NONE;
}

// Load RagFile from string
static PyObject* py_ragfile_loads(PyObject* self, PyObject* args) {
    const char* data;
    Py_ssize_t length;

    if (!PyArg_ParseTuple(args, "s#", &data, &length)) {
        return NULL;
    }

    FILE* file = fmemopen((void*)data, length, "rb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open memory buffer as file");
        return NULL;
    }

    RagFile* rf;
    RagfileError error = ragfile_load(&rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        PyErr_SetString(PyExc_IOError, "Failed to load RagFile from string");
        return NULL;
    }

    return PyRagFile_from_RagFile(rf);
}

// Dump RagFile to string
static PyObject* py_ragfile_dumps(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;
    if (!PyArg_ParseTuple(args, "O", &py_rf)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(py_rf, &PyRagFileType)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a RagFile object");
        return NULL;
    }

    char* buffer;
    size_t size;
    FILE* file = open_memstream(&buffer, &size);
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to create memory buffer");
        return NULL;
    }

    RagfileError error = ragfile_save(py_rf->rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        free(buffer);
        PyErr_SetString(PyExc_IOError, "Failed to save RagFile to string");
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize(buffer, size);
    free(buffer);
    return result;
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

static PyObject* PyRagFile_cosine(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    if (!PyArg_ParseTuple(args, "O!", Py_TYPE(self), &other)) {
        return NULL;
    }

    if (self->rf->header.embedding_size != other->rf->header.embedding_size) {
        PyErr_SetString(PyExc_ValueError, "Embeddings must have the same size");
        return NULL;
    }

    float similarity = cosine_similarity(self->rf->embedding, other->rf->embedding, 
                                         self->rf->header.embedding_size);
    return PyFloat_FromDouble(similarity);
}

// Getter methods for RagFile

static PyObject* PyRagFile_get_text(PyRagFile* self, void* closure) {
    return PyUnicode_FromString(self->rf->text);
}

static PyObject* PyRagFile_get_embedding(PyRagFile* self, void* closure) {
    PyObject* embedding = PyList_New(self->rf->header.embedding_size);
    if (embedding == NULL) {
        return NULL;
    }
    for (uint32_t i = 0; i < self->rf->header.embedding_size; i++) {
        PyList_SET_ITEM(embedding, i, PyFloat_FromDouble(self->rf->embedding[i]));
    }
    return embedding;
}

static PyObject* PyRagFile_get_metadata(PyRagFile* self, void* closure) {
    if (self->rf->metadata == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(self->rf->metadata);
}

static PyObject* PyRagFile_get_header(PyRagFile* self, void* closure) {
    Py_INCREF(self->header);
    return (PyObject*)self->header;
}

// Getter methods for RagFileHeader

static PyObject* PyRagFileHeader_get_version(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong(self->header->version);
}

static PyObject* PyRagFileHeader_get_minhash_signature(PyRagFileHeader* self, void* closure) {
    PyObject* signature = PyList_New(MINHASH_SIZE);
    if (signature == NULL) {
        return NULL;
    }
    for (int i = 0; i < MINHASH_SIZE; i++) {
        PyList_SET_ITEM(signature, i, PyLong_FromUnsignedLongLong(self->header->minhash_signature[i]));
    }
    return signature;
}

// Method definitions
static PyMethodDef PyRagFile_methods[] = {
    {"jaccard", (PyCFunction)PyRagFile_jaccard, METH_VARARGS, "Compute Jaccard similarity with another RagFile"},
    {"cosine", (PyCFunction)PyRagFile_cosine, METH_VARARGS, "Compute Cosine similarity with another RagFile"},
    {NULL}  /* Sentinel */
};

static PyGetSetDef PyRagFile_getsetters[] = {
    {"text", (getter)PyRagFile_get_text, NULL, "Get the text content", NULL},
    {"embedding", (getter)PyRagFile_get_embedding, NULL, "Get the embedding", NULL},
    {"metadata", (getter)PyRagFile_get_metadata, NULL, "Get the metadata", NULL},
    {"header", (getter)PyRagFile_get_header, NULL, "Get the header", NULL},
    {NULL}  /* Sentinel */
};

static PyGetSetDef PyRagFileHeader_getsetters[] = {
    {"version", (getter)PyRagFileHeader_get_version, NULL, "Get the version", NULL},
    {"minhash_signature", (getter)PyRagFileHeader_get_minhash_signature, NULL, "Get the MinHash signature", NULL},
    {NULL}  /* Sentinel */
};

// RagFile type definition
static PyTypeObject PyRagFileType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ragfile.RagFile",
    .tp_doc = "RagFile object",
    .tp_basicsize = sizeof(PyRagFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_init = (initproc)PyRagFile_init,
    .tp_dealloc = (destructor)PyRagFile_dealloc,
    .tp_methods = PyRagFile_methods,
    .tp_getset = PyRagFile_getsetters,
};

// RagFileHeader type definition
static PyTypeObject PyRagFileHeaderType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ragfile.RagFileHeader",
    .tp_doc = "RagFile header object",
    .tp_basicsize = sizeof(PyRagFileHeader),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_dealloc = (destructor)PyRagFileHeader_dealloc,
    .tp_getset = PyRagFileHeader_getsetters,
};

// Module-level method definitions
static PyMethodDef ragfile_methods[] = {
    {"load", py_ragfile_load, METH_VARARGS, "Load a RagFile from a file"},
    {"dump", py_ragfile_dump, METH_VARARGS, "Save a RagFile to a file"},
    {"loads", py_ragfile_loads, METH_VARARGS, "Load a RagFile from a string"},
    {"dumps", py_ragfile_dumps, METH_VARARGS, "Save a RagFile to a string"},
    {NULL, NULL, 0, NULL}
};

// Module definition
static PyModuleDef ragfilemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "ragfile",
    .m_doc = "Python bindings for RagFile library",
    .m_size = -1,
    .m_methods = ragfile_methods,
};

// Module initialization
PyMODINIT_FUNC PyInit_ragfile(void) {
    PyObject* m;

    if (PyType_Ready(&PyRagFileType) < 0)
        return NULL;

    if (PyType_Ready(&PyRagFileHeaderType) < 0)
        return NULL;

    m = PyModule_Create(&ragfilemodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyRagFileType);
    if (PyModule_AddObject(m, "RagFile", (PyObject*)&PyRagFileType) < 0) {
        Py_DECREF(&PyRagFileType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&PyRagFileHeaderType);
    if (PyModule_AddObject(m, "RagFileHeader", (PyObject*)&PyRagFileHeaderType) < 0) {
        Py_DECREF(&PyRagFileHeaderType);
        Py_DECREF(&PyRagFileType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}
