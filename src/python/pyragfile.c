#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pyragfile.h"
#include "pyragfileheader.h"
#include "similarity.h"
#include "utility.h"

// Deallocate PyRagFile
static void PyRagFile_dealloc(PyRagFile* self) {
    if (self->rf) {
        ragfile_free(self->rf);
        self->rf = NULL;
    }
    Py_XDECREF(self->header);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Create a new PyRagFile
PyObject* PyRagFile_New(PyTypeObject* type, RagFile* rf, PyTypeObject* header_type) {
    PyRagFile* obj = (PyRagFile*)type->tp_alloc(type, 0);
    if (!obj) {
        return PyErr_NoMemory();
    }
    obj->rf = rf;
    obj->header = NULL;

    // Initialize the object using the shared init method for deserialization
    if (PyRagFile_shared_init(obj, rf, 1, header_type) < 0) {
        Py_DECREF(obj);
        return NULL;
    }

    return (PyObject*)obj;
}

// Initialize PyRagFile with shared logic for both creation and deserialization
int PyRagFile_shared_init(PyRagFile* self, RagFile* rf, int is_loaded, PyTypeObject* header_type) {
    self->rf = rf;

    PyObject* header_obj = PyRagFileHeader_New(header_type, &(self->rf->header));
    if (!header_obj) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create PyRagFileHeader object");
        return -1;
    }
    self->header = (PyRagFileHeader*)header_obj;

    return 0;
}

// Initialize PyRagFile for creation from scratch
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
    int vector1_type, vector2_type;
    PyObject* vector1_obj = NULL;
    PyObject* vector2_obj = NULL;
    int is_loaded = 0;

    static char* kwlist[] = {"text", "token_ids", "embeddings", "extended_metadata", "tokenizer_id", "embedding_id", "metadata_version", "vector1_type", "vector2_type", "vector1", "vector2", "is_loaded", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOOsssHiiOOi", kwlist,
                                     &text, &token_ids_obj, &embeddings_obj, &extended_metadata,
                                     &tokenizer_id, &embedding_id, &metadata_version,
                                     &vector1_type, &vector2_type,
                                     &vector1_obj, &vector2_obj,
                                     &is_loaded)) {
        return -1;
    }

    if (!is_loaded && (!text || !token_ids_obj || !embeddings_obj || !tokenizer_id || !embedding_id || !vector1_obj || !vector2_obj)) {
        PyErr_SetString(PyExc_ValueError, "Missing required arguments");
        return -1;
    }

    uint32_t* token_ids = NULL;
    uint32_t* vector1 = NULL;
    uint32_t* vector2 = NULL;
    float* flattened_embeddings = NULL;
    size_t total_floats = 0;
    Py_ssize_t num_tokens = 0;
    uint16_t vector1_dim = 0;
    uint16_t vector2_dim = 0;

    if (!is_loaded) {
        // Token IDs conversion
        num_tokens = PyList_Size(token_ids_obj);
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

        // Vector conversion
        vector1_dim = PyList_Size(vector1_obj);
        vector2_dim = PyList_Size(vector2_obj);
        vector1 = (uint32_t*)calloc(DIMENSION, sizeof(uint32_t));
        vector2 = (uint32_t*)calloc(DIMENSION, sizeof(uint32_t));
        if (!vector1 || !vector2) {
            free(token_ids);
            PyErr_NoMemory();
            return -1;
        }
        for (size_t i = 0; i < vector1_dim; i++) {
            PyObject* item = PyList_GetItem(vector1_obj, i);
            if (!PyLong_Check(item)) {
                free(token_ids);
                free(vector1);
                free(vector2);
                PyErr_SetString(PyExc_TypeError, "Vectors must be lists of integers");
                return -1;
            }
            vector1[i] = (uint32_t)PyLong_AsUnsignedLong(item);
        }
        for (size_t i = 0; i < vector2_dim; i++) {
            PyObject* item = PyList_GetItem(vector2_obj, i);
            if (!PyLong_Check(item)) {
                free(token_ids);
                free(vector1);
                free(vector2);
                PyErr_SetString(PyExc_TypeError, "Vectors must be lists of integers");
                return -1;
            }
            vector2[i] = (uint32_t)PyLong_AsUnsignedLong(item);
        }

        // Embeddings preparation
        if (!prepare_embeddings(embeddings_obj, &flattened_embeddings, &total_floats, &num_embeddings, &embedding_dim)) {
            free(token_ids);
            free(vector1);
            free(vector2);
            return -1;
        }

        // RagFile creation
        RagfileError error = ragfile_create(&self->rf, text, 
                                            vector1, vector1_dim, vector2, vector2_dim,
                                            vector1_type, vector2_type,
                                            flattened_embeddings, total_floats, extended_metadata,
                                            tokenizer_id, embedding_id, metadata_version, num_embeddings, embedding_dim);
        free(token_ids);
        free(vector1);
        free(vector2);
        free(flattened_embeddings);

        if (error != RAGFILE_SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create RagFile");
            return -1;
        }
    }

    // Call the shared initialization logic
    return PyRagFile_shared_init(self, self->rf, is_loaded, &PyRagFileHeaderType);
}

// Getter methods for RagFile

static PyObject* PyRagFile_get_text(PyRagFile* self, void* closure) {
    return PyUnicode_FromString(self->rf->text);
}

static PyObject* PyRagFile_get_embeddings(PyRagFile* self, void* closure) {
    int num_embeddings = self->rf->header.num_embeddings;
    int embedding_dim = self->rf->header.embedding_dim;

    PyObject* embeddings_list = PyList_New(num_embeddings);
    if (!embeddings_list) return PyErr_NoMemory();

    for (int i = 0; i < num_embeddings; i++) {
        PyObject* single_embedding = PyList_New(embedding_dim);
        if (!single_embedding) {
            Py_DECREF(embeddings_list);
            return PyErr_NoMemory();
        }
        
        for (int j = 0; j < embedding_dim; j++) {
            int idx = i * embedding_dim + j;
            PyObject* float_obj = PyFloat_FromDouble(self->rf->embeddings[idx]);
            PyList_SET_ITEM(single_embedding, j, float_obj);
        }

        PyList_SET_ITEM(embeddings_list, i, single_embedding);
    }

    return embeddings_list;
}

// Getter method for RagFile header
static PyObject* PyRagFile_get_header(PyRagFile* self, void* closure) {
    if (!self->header) {
        Py_RETURN_NONE;
    }

    Py_INCREF(self->header);
    return (PyObject*)self->header;
}

static PyObject* PyRagFile_get_extended_metadata(PyRagFile* self, void* closure) {
    if (self->rf->extended_metadata == NULL) {
        Py_RETURN_NONE;
    }
    return PyUnicode_FromString(self->rf->extended_metadata);
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
    {NULL}  /* Sentinel */
};

// PyRagFile type definition
PyTypeObject PyRagFileType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "ragfile.RagFile",
    .tp_doc = "RagFile object",
    .tp_basicsize = sizeof(PyRagFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = PyType_GenericNew,
    .tp_alloc = PyType_GenericAlloc,
    .tp_init = (initproc)PyRagFile_init,
    .tp_dealloc = (destructor)PyRagFile_dealloc,
    .tp_methods = PyRagFile_methods,
    .tp_getset = PyRagFile_getsetters,
};

