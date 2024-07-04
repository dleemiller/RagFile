#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../include/config.h"
#include "common.h"
#include "../core/ragfile.h"

static PyTypeObject *imported_PyRagFileType = NULL;
static PyTypeObject *imported_PyRagFileHeaderType = NULL;

// Forward declaration of methods
static PyObject* py_ragfile_load(PyObject* self, PyObject* args);
static PyObject* py_ragfile_dump(PyObject* self, PyObject* args);
static PyObject* py_ragfile_loads(PyObject* self, PyObject* args);
static PyObject* py_ragfile_dumps(PyObject* self, PyObject* args);

static PyMethodDef ragfile_methods[] = {
    {"load", py_ragfile_load, METH_VARARGS, "Load a RagFile from a file"},
    {"dump", py_ragfile_dump, METH_VARARGS, "Save a RagFile to a file"},
    {"loads", py_ragfile_loads, METH_VARARGS, "Load a RagFile from a string"},
    {"dumps", py_ragfile_dumps, METH_VARARGS, "Save a RagFile to a string"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef ragfile_io = {
    PyModuleDef_HEAD_INIT,
    "ragfile_io",
    "Python bindings for RagFile I/O functionalities",
    -1,
    ragfile_methods,
};

PyMODINIT_FUNC PyInit_io(void) {
    PyObject *m, *ragfile_module, *capsule;

    m = PyModule_Create(&ragfile_io);
    if (!m) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to create the module");
        return NULL;
    }

    // Import ragfile.ragfile module
    ragfile_module = PyImport_ImportModule("ragfile.ragfile");
    if (!ragfile_module) {
        PyErr_SetString(PyExc_ImportError, "Failed to import 'ragfile.ragfile'");
        Py_DECREF(m);
        return NULL;
    } else {
        printf("Successfully imported 'ragfile.ragfile'\n");
    }

    // Retrieve the type capsule for PyRagFileType
    capsule = PyObject_GetAttrString(ragfile_module, "PyRagFileType_capsule");
    if (!capsule) {
        PyErr_SetString(PyExc_AttributeError, "Failed to retrieve 'PyRagFileType_capsule'");
        Py_DECREF(ragfile_module);
        Py_DECREF(m);
        return NULL;
    } else {
        printf("Successfully retrieved the capsule\n");
    }

    // Extract the type from the capsule
    imported_PyRagFileType = (PyTypeObject*)PyCapsule_GetPointer(capsule, "ragfile.PyRagFileType");
    if (!imported_PyRagFileType) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to extract type from capsule");
        Py_DECREF(capsule);
        Py_DECREF(ragfile_module);
        Py_DECREF(m);
        return NULL;
    } else {
        printf("Successfully extracted PyRagFileType from the capsule\n");
    }

    printf("Address of imported_PyRagFileType: %p\n", (void*)imported_PyRagFileType);

    Py_DECREF(capsule);

    // Retrieve the type capsule for PyRagFileHeaderType
    capsule = PyObject_GetAttrString(ragfile_module, "PyRagFileHeaderType_capsule");
    if (!capsule) {
        PyErr_SetString(PyExc_AttributeError, "Failed to retrieve 'PyRagFileHeaderType_capsule'");
        Py_DECREF(ragfile_module);
        Py_DECREF(m);
        return NULL;
    } else {
        printf("Successfully retrieved the header capsule\n");
    }

    // Extract the type from the capsule
    imported_PyRagFileHeaderType = (PyTypeObject*)PyCapsule_GetPointer(capsule, "ragfile.PyRagFileHeaderType");
    if (!imported_PyRagFileHeaderType) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to extract header type from capsule");
        Py_DECREF(capsule);
        Py_DECREF(ragfile_module);
        Py_DECREF(m);
        return NULL;
    } else {
        printf("Successfully extracted PyRagFileHeaderType from the capsule\n");
    }

    printf("Address of imported_PyRagFileHeaderType: %p\n", (void*)imported_PyRagFileHeaderType);

    Py_DECREF(capsule);
    Py_DECREF(ragfile_module);
    return m;
}

// Load RagFile from file object
static PyObject* py_ragfile_load(PyObject* self, PyObject* args) {
    PyObject* file_obj;
    if (!PyArg_ParseTuple(args, "O", &file_obj)) {
        return NULL;
    }

    int fd = PyObject_AsFileDescriptor(file_obj);
    if (fd == -1) {
        PyErr_SetString(PyExc_TypeError, "Expected a file-like object");
        return NULL;
    }

    FILE* file = fdopen(dup(fd), "rb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open file");
        return NULL;
    }

    RagFile* rf;
    RagfileError error = ragfile_load(&rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        PyErr_SetString(PyExc_IOError, "Failed to load RagFile");
        return NULL;
    }

    return PyRagFile_New(imported_PyRagFileType, rf, imported_PyRagFileHeaderType);}

// Dump RagFile to file object
static PyObject* py_ragfile_dump(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;
    PyObject* file_obj;
    if (!PyArg_ParseTuple(args, "OO", &py_rf, &file_obj)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(py_rf, imported_PyRagFileType)) {
        PyErr_SetString(PyExc_TypeError, "First argument must be a RagFile object");
        return NULL;
    }

    int fd = PyObject_AsFileDescriptor(file_obj);
    if (fd == -1) {
        PyErr_SetString(PyExc_TypeError, "Expected a file-like object");
        return NULL;
    }

    FILE* file = fdopen(dup(fd), "wb");
    if (file == NULL) {
        PyErr_SetString(PyExc_IOError, "Failed to open file");
        return NULL;
    }

    if (!py_rf->rf) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid RagFile");
        return NULL;
    }
    RagfileError error = ragfile_save(py_rf->rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        PyErr_SetString(PyExc_IOError, "Failed to save RagFile");
        return NULL;
    }

    Py_RETURN_NONE;
}

// Load RagFile from string
static PyObject* py_ragfile_loads(PyObject* self, PyObject* args) {
    const char* data;
    Py_ssize_t length;
    printf("aaa\n");

    if (!PyArg_ParseTuple(args, "s#", &data, &length)) {
        return NULL;
    }

    printf("baa\n");
    if (length == 0) {
        PyErr_SetString(PyExc_ValueError, "Empty data cannot be loaded as a RagFile");
        return NULL;
    }

    printf("caa\n");
    FILE* file = fmemopen((void*)data, length, "rb");
    if (!file) {
        PyErr_SetString(PyExc_IOError, "Failed to open memory buffer as file");
        return NULL;
    }

    printf("daa\n");
    RagFile* rf = NULL;
    RagfileError error = ragfile_load(&rf, file);
    fclose(file);

    printf("eaa\n");
    if (error != RAGFILE_SUCCESS || !rf) {
        if (rf) {
            ragfile_free(rf);
        }
        PyErr_Format(PyExc_IOError, "Failed to load RagFile from string, error code: %d", error);
        return NULL;
    }

    printf("Before PyRagFile_New\n");
    printf("imported_PyRagFileType: %p\n", (void*)imported_PyRagFileType);
    printf("imported_PyRagFileHeaderType: %p\n", (void*)imported_PyRagFileHeaderType);
    printf("rf: %p\n", (void*)rf);

    PyObject* result = PyRagFile_New(imported_PyRagFileType, rf, imported_PyRagFileHeaderType);
    printf("After PyRagFile_New\n");

    if (!result) {
        ragfile_free(rf); // Clean up if Python object creation fails
    }

    printf("gaa\n");
    return result;
}


// Dump RagFile to string
static PyObject* py_ragfile_dumps(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;
    printf("000\n");
    if (!PyArg_ParseTuple(args, "O!", imported_PyRagFileType, &py_rf)) {
        return NULL;
    }
    printf("111\n");

    if (!py_rf->rf) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid RagFile object");
        return NULL;
    }
    printf("211\n");

    char* buffer = NULL;
    size_t size = 0;
    FILE* file = open_memstream(&buffer, &size);
    if (!file) {
        PyErr_SetString(PyExc_IOError, "Failed to create memory buffer");
        return NULL;
    }
    printf("311\n");

    RagfileError error = ragfile_save(py_rf->rf, file);
    fclose(file);
    printf("411\n");

    if (error != RAGFILE_SUCCESS) {
        if (buffer) free(buffer);
        PyErr_Format(PyExc_IOError, "Failed to save RagFile to string, error code: %d", error);
        return NULL;
    }
    printf("511\n");

    PyObject* result = PyBytes_FromStringAndSize(buffer, size);
    free(buffer);
    if (!result) {
        return PyErr_NoMemory();
    }
    return result;
}

