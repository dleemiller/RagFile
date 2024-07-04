#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pyragfileheader.h"

// Deallocate PyRagFileHeader
void PyRagFileHeader_dealloc(PyRagFileHeader* self) {
    if (self->header) {
        printf("PyRagFileHeader_dealloc: Deallocating header: %p\n", (void*)self->header);
        self->header = NULL;
    }
    Py_TYPE(self)->tp_free((PyObject*)self);
}

// Create a new PyRagFileHeader
PyObject* PyRagFileHeader_New(PyTypeObject* type, RagfileHeader* header) {
    PyRagFileHeader* obj = (PyRagFileHeader*)type->tp_alloc(type, 0);
    if (!obj) {
        return PyErr_NoMemory();
    }

    obj->header = header;
    return (PyObject*)obj;
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

// Getters for RagFileHeader
static PyGetSetDef PyRagFileHeader_getsetters[] = {
    {"version", (getter)PyRagFileHeader_get_version, NULL, "Get the version", NULL},
    {"tokenizer_id_hash", (getter)PyRagFileHeader_get_tokenizer_hash, NULL, "Get the CRC 16 hash of the tokenzier id", NULL},
    {"embedding_id_hash", (getter)PyRagFileHeader_get_embedding_hash, NULL, "Get the CRC 16 hash of the embedding id", NULL},
    {"binary_embedding", (getter)PyRagFileHeader_get_binary_embedding, NULL, "Get the Binary Embedding", NULL},
    {"minhash_signature", (getter)PyRagFileHeader_get_minhash_signature, NULL, "Get the MinHash signature", NULL},
    {NULL}  /* Sentinel */
};

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

