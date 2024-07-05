#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pyragfileheader.h"

// Deallocate PyRagFileHeader
void PyRagFileHeader_dealloc(PyRagFileHeader* self) {
    if (self->header) {
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
    return PyLong_FromUnsignedLong((unsigned long)self->header->version);
}

static PyObject* PyRagFileHeader_get_flags(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->flags);
}

static PyObject* PyRagFileHeader_get_vector1_type(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->vector1_type);
}

static PyObject* PyRagFileHeader_get_vector2_type(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->vector2_type);
}

static PyObject* PyRagFileHeader_get_vector1_dim(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->vector1_dim);
}

static PyObject* PyRagFileHeader_get_vector2_dim(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->vector2_dim);
}

static PyObject* PyRagFileHeader_get_vector1(PyRagFileHeader* self, void* closure) {
    PyObject* vector = PyList_New(DIMENSION);
    if (vector == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < DIMENSION; i++) {
        PyObject* item = PyLong_FromUnsignedLong(self->header->vector1[i]);
        if (!item) {
            Py_DECREF(vector);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(vector, i, item);
    }
    return vector;
}

static PyObject* PyRagFileHeader_get_vector2(PyRagFileHeader* self, void* closure) {
    PyObject* vector = PyList_New(DIMENSION);
    if (vector == NULL) {
        return PyErr_NoMemory();
    }
    for (int i = 0; i < DIMENSION; i++) {
        PyObject* item = PyLong_FromUnsignedLong(self->header->vector2[i]);
        if (!item) {
            Py_DECREF(vector);
            return PyErr_NoMemory();
        }
        PyList_SET_ITEM(vector, i, item);
    }
    return vector;
}

static PyObject* PyRagFileHeader_get_text_size(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->text_size);
}

static PyObject* PyRagFileHeader_get_metadata_version(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->metadata_version);
}

static PyObject* PyRagFileHeader_get_metadata_size(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->metadata_size);
}

static PyObject* PyRagFileHeader_get_num_embeddings(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->num_embeddings);
}

static PyObject* PyRagFileHeader_get_embedding_dim(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->embedding_dim);
}

static PyObject* PyRagFileHeader_get_embedding_size(PyRagFileHeader* self, void* closure) {
    return PyLong_FromUnsignedLong((unsigned long)self->header->embedding_size);
}

static PyObject* PyRagFileHeader_get_tokenizer_id(PyRagFileHeader* self, void* closure) {
    return PyUnicode_FromString(self->header->tokenizer_id);
}

static PyObject* PyRagFileHeader_get_embedding_id(PyRagFileHeader* self, void* closure) {
    return PyUnicode_FromString(self->header->embedding_id);
}

// Getters for RagFileHeader
static PyGetSetDef PyRagFileHeader_getsetters[] = {
    {"version", (getter)PyRagFileHeader_get_version, NULL, "Get the version", NULL},
    {"flags", (getter)PyRagFileHeader_get_flags, NULL, "Get the flags", NULL},
    {"vector1_type", (getter)PyRagFileHeader_get_vector1_type, NULL, "Get vector1 type", NULL},
    {"vector2_type", (getter)PyRagFileHeader_get_vector2_type, NULL, "Get vector2 type", NULL},
    {"vector1_dim", (getter)PyRagFileHeader_get_vector1_dim, NULL, "Get vector1 dimension", NULL},
    {"vector2_dim", (getter)PyRagFileHeader_get_vector2_dim, NULL, "Get vector2 dimension", NULL},
    {"vector1", (getter)PyRagFileHeader_get_vector1, NULL, "Get vector1", NULL},
    {"vector2", (getter)PyRagFileHeader_get_vector2, NULL, "Get vector2", NULL},
    {"text_size", (getter)PyRagFileHeader_get_text_size, NULL, "Get text size", NULL},
    {"metadata_version", (getter)PyRagFileHeader_get_metadata_version, NULL, "Get metadata version", NULL},
    {"metadata_size", (getter)PyRagFileHeader_get_metadata_size, NULL, "Get metadata size", NULL},
    {"num_embeddings", (getter)PyRagFileHeader_get_num_embeddings, NULL, "Get number of embeddings", NULL},
    {"embedding_dim", (getter)PyRagFileHeader_get_embedding_dim, NULL, "Get embedding dimension", NULL},
    {"embedding_size", (getter)PyRagFileHeader_get_embedding_size, NULL, "Get embedding size", NULL},
    {"tokenizer_id", (getter)PyRagFileHeader_get_tokenizer_id, NULL, "Get the tokenizer ID", NULL},
    {"embedding_id", (getter)PyRagFileHeader_get_embedding_id, NULL, "Get the embedding ID", NULL},
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

