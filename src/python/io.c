#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "../include/config.h"
#include "../include/ragfile_types.h"
#include "../core/ragfile.h"


// Methods for module
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

PyMODINIT_FUNC PyInit_ragfile_io(void) {
    PyObject* m;
    Py_INCREF(&PyRagFileType);  // Ensure type is initialized and increased ref count
    m = PyModule_Create(&ragfile_io);
    if (m == NULL) {
        return NULL;
    }
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

    return PyRagFile_New(rf);
}

// Dump RagFile to file object
static PyObject* py_ragfile_dump(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;
    PyObject* file_obj;
    if (!PyArg_ParseTuple(args, "OO", &py_rf, &file_obj)) {
        return NULL;
    }

    if (!PyObject_TypeCheck(py_rf, &PyRagFileType)) {
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

    if (!PyArg_ParseTuple(args, "s#", &data, &length)) {
        return NULL;
    }

    if (length == 0) {
        PyErr_SetString(PyExc_ValueError, "Empty data cannot be loaded as a RagFile");
        return NULL;
    }

    FILE* file = fmemopen((void*)data, length, "rb");
    if (!file) {
        PyErr_SetString(PyExc_IOError, "Failed to open memory buffer as file");
        return NULL;
    }

    RagFile* rf = NULL;
    RagfileError error = ragfile_load(&rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS || !rf) {
        if (rf) {
            ragfile_free(rf);
        }
        PyErr_Format(PyExc_IOError, "Failed to load RagFile from string, error code: %d", error);
        return NULL;
    }

    PyObject* result = PyRagFile_New(rf);
    if (!result) {
        ragfile_free(rf); // Clean up if Python object creation fails
    }
    return result;
}

// Dump RagFile to string
static PyObject* py_ragfile_dumps(PyObject* self, PyObject* args) {
    PyRagFile* py_rf;

    if (!PyArg_ParseTuple(args, "O!", &PyRagFileType, &py_rf)) {
        return NULL;
    }

    if (!py_rf->rf) {
        PyErr_SetString(PyExc_RuntimeError, "Invalid RagFile object");
        return NULL;
    }

    char* buffer = NULL;
    size_t size = 0;
    FILE* file = open_memstream(&buffer, &size);
    if (!file) {
        PyErr_SetString(PyExc_IOError, "Failed to create memory buffer");
        return NULL;
    }

    RagfileError error = ragfile_save(py_rf->rf, file);
    fclose(file);

    if (error != RAGFILE_SUCCESS) {
        if (buffer) free(buffer);
        PyErr_Format(PyExc_IOError, "Failed to save RagFile to string, error code: %d", error);
        return NULL;
    }

    PyObject* result = PyBytes_FromStringAndSize(buffer, size);
    free(buffer);
    if (!result) {
        return PyErr_NoMemory();
    }
    return result;
}


