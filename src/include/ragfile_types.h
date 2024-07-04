#ifndef RAGFILE_TYPES_H
#define RAGFILE_TYPES_H

#include <Python.h>
#include "../core/ragfile.h"

typedef struct {
    PyObject_HEAD
    RagfileHeader* header;
} PyRagFileHeader;

typedef struct {
    PyObject_HEAD
    RagFile* rf;         // Replace with actual definition or include necessary header
    PyRagFileHeader* header;    // Python object for header
    PyObject* file_metadata; // Metadata as a Python dictionary
} PyRagFile;

extern PyTypeObject PyRagFileType;
extern PyTypeObject PyRagFileHeaderType;

PyObject* PyRagFile_New(RagFile* rf);
PyObject* PyRagFileHeader_New(RagfileHeader* header);


#endif // RAGFILE_TYPES_H

