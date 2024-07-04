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
#include "../include/ragfile_types.h"



// Forward declarations for header and file type deallocations
static void PyRagFileHeader_dealloc(PyRagFileHeader* self);
static void PyRagFile_dealloc(PyRagFile* self);

// Initialize PyRagFile
static int PyRagFile_init(PyRagFile* self, PyObject* args, PyObject* kwds);

// Deallocate PyRagFileHeader
static void PyRagFileHeader_dealloc(PyRagFileHeader* self) {
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Deallocate PyRagFile
static void PyRagFile_dealloc(PyRagFile* self) {
    ragfile_free(self->rf);
    Py_XDECREF(self->header);
    Py_XDECREF(self->file_metadata);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Initialize PyRagFile

PyObject* PyRagFile_New(RagFile* rf) {
    PyRagFile* obj = (PyRagFile*)PyType_GenericNew(&PyRagFileType, NULL, NULL);
    if (!obj)
        return PyErr_NoMemory();
    obj->rf = rf;
    return (PyObject*)obj;
}

PyObject* PyRagFileHeader_New(RagfileHeader* header) {
    PyRagFileHeader* obj = (PyRagFileHeader*)PyType_GenericNew(&PyRagFileHeaderType, NULL, NULL);
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

// Initialize PyRagFile
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

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|sOOsssHi", kwlist,
                                     &text, &token_ids_obj, &embeddings_obj, &extended_metadata,
                                     &tokenizer_id, &embedding_id, &metadata_version, &is_loaded)) {
        return -1; // Error handling if arguments are not correctly parsed
    }

    if (!is_loaded && (!text || !token_ids_obj || !embeddings_obj || !tokenizer_id || !embedding_id)) {
        PyErr_SetString(PyExc_ValueError, "Missing required arguments");
        return -1;
    }

    uint32_t* token_ids = NULL;
    float* flattened_embeddings = NULL;
    size_t total_floats = 0;

    self->file_metadata = PyDict_New();
    if (!self->file_metadata) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create file metadata dictionary");
        Py_DECREF(self->header);
        return -1;
    }

    if (!is_loaded) {
        // Token IDs conversion
        Py_ssize_t num_tokens = PyList_Size(token_ids_obj);
        token_ids = (uint32_t*) malloc(num_tokens * sizeof(uint32_t));
        if (!token_ids) {
            PyErr_NoMemory();
            return -1;
        }
        for (Py_ssize_t i = 0; i < num_tokens; i++) {
            PyObject* item = PyList_GetItem(token_ids_obj, i);
            if (!PyLong_Check(item)) {
                free(token_ids);
		token_ids = NULL;
                PyErr_SetString(PyExc_TypeError, "Token IDs must be integers");
                return -1;
            }
            token_ids[i] = (uint32_t)PyLong_AsUnsignedLong(item);
        }

        // Embeddings preparation
        if (!prepare_embeddings(embeddings_obj, &flattened_embeddings, &total_floats, &num_embeddings, &embedding_dim)) {
            free(token_ids);
	    token_ids = NULL;
            return -1; // Validate and prepare the embeddings array
        }

        // RagFile creation
        RagfileError error = ragfile_create(&self->rf, text, token_ids, num_tokens,
                                            flattened_embeddings, total_floats, extended_metadata,
                                            tokenizer_id, embedding_id, metadata_version, num_embeddings, embedding_dim);
	free(token_ids);
	token_ids = NULL;
	free(flattened_embeddings);
	flattened_embeddings = NULL;

        if (error != RAGFILE_SUCCESS) {
            PyErr_SetString(PyExc_RuntimeError, "Failed to create RagFile");
            return -1;
        }

    	PyDict_SetItemString(self->file_metadata, "tokenizer_id", PyUnicode_FromString(tokenizer_id));
    	PyDict_SetItemString(self->file_metadata, "embedding_id", PyUnicode_FromString(embedding_id));
    	PyDict_SetItemString(self->file_metadata, "metadata_version", PyLong_FromUnsignedLong(metadata_version));
    	PyDict_SetItemString(self->file_metadata, "num_embeddings", PyLong_FromUnsignedLong(num_embeddings));
    	PyDict_SetItemString(self->file_metadata, "embedding_dim", PyLong_FromUnsignedLong(embedding_dim));

    }

    self->header = PyObject_New(PyRagFileHeader, &PyRagFileHeaderType);
    if (!self->header) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to initialize RagFile header");
        return -1;
    }
    self->header->header = &(self->rf->header);


    return 0;
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

static PyObject* PyRagFile_get_header(PyRagFile* self, void* closure) {
    Py_INCREF(self->header);
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
    .tp_dealloc = (destructor)PyRagFileHeader_dealloc,
    .tp_getset = PyRagFileHeader_getsetters,
};


// Module definition
static PyModuleDef ragfilemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "ragfile",
    .m_doc = "Python bindings for RagFile library",
    .m_size = -1,
    //.m_methods = ragfile_methods,
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

