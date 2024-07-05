#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../core/ragfile.h"
#include "minhash.h"

// Define default values
#define DEFAULT_NGRAM 3
#define DEFAULT_PERMUTE 128
#define DEFAULT_SEED 0

static PyObject* pyminhash_char(PyObject* self, PyObject* args, PyObject* kwargs) {
    const char* text;
    int ngram = DEFAULT_NGRAM;
    int permute = DEFAULT_PERMUTE;
    uint32_t seed = DEFAULT_SEED;

    static char* kwlist[] = {"text", "ngram", "permute", "seed", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|iii", kwlist, &text, &ngram, &permute, &seed)) {
        return NULL;
    }

    MinHash* mh;
    if (minhash_create(&mh, permute, seed) != MINHASH_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create MinHash object");
        return NULL;
    }

    if (minhash_compute_char(mh, text, ngram) != MINHASH_SUCCESS) {
        minhash_free(mh);
        PyErr_SetString(PyExc_RuntimeError, "Failed to compute MinHash from characters");
        return NULL;
    }

    PyObject* signature = PyList_New(mh->num_hashes);
    for (size_t i = 0; i < mh->num_hashes; i++) {
        PyList_SET_ITEM(signature, i, PyLong_FromUnsignedLong(mh->signature[i]));
    }

    minhash_free(mh);
    return signature;
}

static PyObject* pyminhash_word(PyObject* self, PyObject* args, PyObject* kwargs) {
    const char* text;
    int ngram = DEFAULT_NGRAM;
    int permute = DEFAULT_PERMUTE;
    uint32_t seed = DEFAULT_SEED;

    static char* kwlist[] = {"text", "ngram", "permute", "seed", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|iii", kwlist, &text, &ngram, &permute, &seed)) {
        return NULL;
    }

    MinHash* mh;
    if (minhash_create(&mh, permute, seed) != MINHASH_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create MinHash object");
        return NULL;
    }

    if (minhash_compute_word(mh, text, ngram) != MINHASH_SUCCESS) {
        minhash_free(mh);
        PyErr_SetString(PyExc_RuntimeError, "Failed to compute MinHash from words");
        return NULL;
    }

    PyObject* signature = PyList_New(mh->num_hashes);
    for (size_t i = 0; i < mh->num_hashes; i++) {
        PyList_SET_ITEM(signature, i, PyLong_FromUnsignedLong(mh->signature[i]));
    }

    minhash_free(mh);
    return signature;
}

static PyObject* pyminhash_tokens(PyObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* token_list;
    int ngram = DEFAULT_NGRAM;
    int permute = DEFAULT_PERMUTE;
    uint32_t seed = DEFAULT_SEED;

    static char* kwlist[] = {"token_list", "ngram", "permute", "seed", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|iii", kwlist, &token_list, &ngram, &permute, &seed)) {
        return NULL;
    }

    if (!PyList_Check(token_list)) {
        PyErr_SetString(PyExc_TypeError, "token_list must be a list");
        return NULL;
    }

    size_t token_count = PyList_Size(token_list);
    uint32_t* token_ids = (uint32_t*)malloc(token_count * sizeof(uint32_t));
    if (token_ids == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    for (size_t i = 0; i < token_count; i++) {
        PyObject* item = PyList_GetItem(token_list, i);
        if (!PyLong_Check(item)) {
            free(token_ids);
            PyErr_SetString(PyExc_TypeError, "All items in token_list must be integers");
            return NULL;
        }
        token_ids[i] = (uint32_t)PyLong_AsUnsignedLong(item);
    }

    MinHash* mh;
    if (minhash_create(&mh, permute, seed) != MINHASH_SUCCESS) {
        free(token_ids);
        PyErr_SetString(PyExc_RuntimeError, "Failed to create MinHash object");
        return NULL;
    }

    if (minhash_compute_tokens(mh, token_ids, token_count, ngram) != MINHASH_SUCCESS) {
        free(token_ids);
        minhash_free(mh);
        PyErr_SetString(PyExc_RuntimeError, "Failed to compute MinHash from tokens");
        return NULL;
    }

    free(token_ids);

    PyObject* signature = PyList_New(mh->num_hashes);
    for (size_t i = 0; i < mh->num_hashes; i++) {
        PyList_SET_ITEM(signature, i, PyLong_FromUnsignedLong(mh->signature[i]));
    }

    minhash_free(mh);
    return signature;
}

static PyObject* pyminhash_merge(PyObject* self, PyObject* args) {
    PyObject* mh1_list;
    PyObject* mh2_list;

    if (!PyArg_ParseTuple(args, "OO", &mh1_list, &mh2_list)) {
        return NULL;
    }

    if (!PyList_Check(mh1_list) || !PyList_Check(mh2_list)) {
        PyErr_SetString(PyExc_TypeError, "Both arguments must be lists");
        return NULL;
    }

    size_t num_hashes = PyList_Size(mh1_list);
    if (num_hashes != PyList_Size(mh2_list)) {
        PyErr_SetString(PyExc_ValueError, "Both lists must have the same length");
        return NULL;
    }

    MinHash* mh1;
    MinHash* mh2;
    if (minhash_create(&mh1, num_hashes, 0) != MINHASH_SUCCESS ||
        minhash_create(&mh2, num_hashes, 0) != MINHASH_SUCCESS) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create MinHash objects");
        return NULL;
    }

    for (size_t i = 0; i < num_hashes; i++) {
        mh1->signature[i] = (uint32_t)PyLong_AsUnsignedLong(PyList_GetItem(mh1_list, i));
        mh2->signature[i] = (uint32_t)PyLong_AsUnsignedLong(PyList_GetItem(mh2_list, i));
    }

    MinHashError err = minhash_merge(mh1, mh2);
    minhash_free(mh2);

    if (err != MINHASH_SUCCESS) {
        minhash_free(mh1);
        PyErr_SetString(PyExc_RuntimeError, "Failed to merge MinHash objects");
        return NULL;
    }

    PyObject* merged_signature = PyList_New(mh1->num_hashes);
    for (size_t i = 0; i < mh1->num_hashes; i++) {
        PyList_SET_ITEM(merged_signature, i, PyLong_FromUnsignedLong(mh1->signature[i]));
    }

    minhash_free(mh1);
    return merged_signature;
}

static PyMethodDef MinHashMethods[] = {
    {"char", (PyCFunction)pyminhash_char, METH_VARARGS | METH_KEYWORDS, "Compute MinHash from character n-grams"},
    {"word", (PyCFunction)pyminhash_word, METH_VARARGS | METH_KEYWORDS, "Compute MinHash from word n-grams"},
    {"tokens", (PyCFunction)pyminhash_tokens, METH_VARARGS | METH_KEYWORDS, "Compute MinHash from token IDs"},
    {"merge", pyminhash_merge, METH_VARARGS, "Merge two MinHash signatures"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef minhashmodule = {
    PyModuleDef_HEAD_INIT,
    "minhash",
    NULL,
    -1,
    MinHashMethods
};

PyMODINIT_FUNC PyInit_minhash(void) {
    PyObject* m;
    m = PyModule_Create(&minhashmodule);
    if (m == NULL) {
        return NULL;
    }

    // Define VectorType Enum
    PyObject* vectype = Py_BuildValue("{s:i, s:i, s:i}", "BINARY_EMBEDDING", BINARY_EMBEDDING, "MIN_HASH", MIN_HASH, "TOKEN_ID", TOKEN_ID);
    if (vectype == NULL) {
        return NULL;
    }

    if (PyModule_AddObject(m, "VectorType", vectype) < 0) {
        Py_DECREF(vectype);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

