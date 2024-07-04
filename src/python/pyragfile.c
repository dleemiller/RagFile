#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pyragfile.h"
#include "pyragfileheader.h"
#include "similarity.h"
#include "utility.h"

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

// Initialize PyRagFile with shared logic for both creation and deserialization
int PyRagFile_shared_init(PyRagFile* self, RagFile* rf, int is_loaded, PyTypeObject* header_type) {
    self->rf = rf;

    self->file_metadata = PyDict_New();
    if (!self->file_metadata) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create file metadata dictionary");
        return -1;
    }

    PyObject* header_obj = PyRagFileHeader_New(header_type, &(self->rf->header));
    if (!header_obj) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create PyRagFileHeader object");
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

// PyRagFile type definition
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

