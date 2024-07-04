#ifndef PYRAGFILE_H
#define PYRAGFILE_H

#include <Python.h>
#include "../core/ragfile.h"
#include "pyragfileheader.h"

// Forward declarations
typedef struct {
    PyObject_HEAD
    RagFile* rf;         // Replace with actual definition or include necessary header
    PyRagFileHeader* header;    // Python object for header
    PyObject* file_metadata; // Metadata as a Python dictionary
} PyRagFile;

extern PyTypeObject PyRagFileType;

// Function declarations
PyObject* PyRagFile_New(PyTypeObject* type, RagFile* rf, PyTypeObject* header_type);
int PyRagFile_shared_init(PyRagFile* self, RagFile* rf, int is_loaded, PyTypeObject* header_type);

#endif // PYRAGFILE_H

