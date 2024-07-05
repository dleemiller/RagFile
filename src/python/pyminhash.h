#ifndef PYMINHASH_H
#define PYMINHASH_H

#include <Python.h>

// Function declarations for the MinHash module
static PyObject* pyminhash_char(PyObject* self, PyObject* args);
static PyObject* pyminhash_word(PyObject* self, PyObject* args);
static PyObject* pyminhash_tokens(PyObject* self, PyObject* args);
static PyObject* pyminhash_merge(PyObject* self, PyObject* args);

static PyMethodDef MinHashMethods[];
static struct PyModuleDef minhashmodule;

PyMODINIT_FUNC PyInit_minhash(void);

#endif // PYMINHASH_H

