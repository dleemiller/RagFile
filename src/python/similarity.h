#ifndef SIMILARITY_H
#define SIMILARITY_H

#include <Python.h>
#include "pyragfile.h"

PyObject* PyRagFile_jaccard(PyRagFile* self, PyObject* args);
PyObject* PyRagFile_hamming(PyRagFile* self, PyObject* args);
PyObject* PyRagFile_cosine(PyRagFile* self, PyObject* args);
PyObject* PyRagFile_match(PyRagFile* self, PyObject* args, PyObject* kwds);

#endif // SIMILARITY_H

