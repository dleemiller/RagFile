#ifndef PYRAGFILEHEADER_H
#define PYRAGFILEHEADER_H

#include <Python.h>
#include "../core/ragfile.h"

// Forward declarations
typedef struct {
    PyObject_HEAD
    RagfileHeader* header;
} PyRagFileHeader;

extern PyTypeObject PyRagFileHeaderType;

void PyRagFileHeader_dealloc(PyRagFileHeader* self);
PyObject* PyRagFileHeader_New(PyTypeObject* type, RagfileHeader* header);

#endif // PYRAGFILEHEADER_H

