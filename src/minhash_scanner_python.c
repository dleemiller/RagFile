#define PY_SSIZE_T_CLEAN
#include <Python.h>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <numpy/npy_common.h>

#include "minhash_scanner.h"

// Check NumPy version at compile time
#if NPY_API_VERSION >= 0x00000010 // NumPy >= 2.0.0
    #define NUMPY_2_0_PLUS
#endif

static PyObject* get_file_header_type(PyObject* self, PyObject* args) {
    static PyTypeObject* file_header_type = NULL;
    if (file_header_type == NULL) {
        static PyStructSequence_Field fields[] = {
            {"magic", "Magic number"},
            {"version", "Version number"},
            {"minhash_size", "Size of minhash"},
            {"embedding_size", "Size of embedding"},
            {"string_length", "Length of string"},
            {"string_offset", "Offset to string data"},
            {"embedding_offset", "Offset to embedding data"},
            {NULL}
        };
        static PyStructSequence_Desc desc = {
            "FileHeader",
            NULL,
            fields,
            7
        };
        file_header_type = PyStructSequence_NewType(&desc);
    }
    Py_INCREF(file_header_type);
    return (PyObject*)file_header_type;
}

static PyObject* py_read_file_header(PyObject* self, PyObject* args) {
    const char* filename;
    if (!PyArg_ParseTuple(args, "s", &filename))
        return NULL;

    FileHeader* header = read_file_header(filename);
    if (header == NULL) {
        // The error has already been set by read_file_header
        return NULL;
    }

    PyObject* header_type = get_file_header_type(NULL, NULL);
    PyObject* header_obj = PyStructSequence_New((PyTypeObject*)header_type);

    if (header_obj == NULL) {
        free(header);
        return NULL;
    }

    PyStructSequence_SetItem(header_obj, 0, PyLong_FromUnsignedLong(header->magic));
    PyStructSequence_SetItem(header_obj, 1, PyLong_FromUnsignedLong(header->version));
    PyStructSequence_SetItem(header_obj, 2, PyLong_FromUnsignedLong(header->minhash_size));
    PyStructSequence_SetItem(header_obj, 3, PyLong_FromUnsignedLong(header->embedding_size));
    PyStructSequence_SetItem(header_obj, 4, PyLong_FromUnsignedLong(header->string_length));
    PyStructSequence_SetItem(header_obj, 5, PyLong_FromUnsignedLongLong(header->string_offset));
    PyStructSequence_SetItem(header_obj, 6, PyLong_FromUnsignedLongLong(header->embedding_offset));

    free(header);
    return header_obj;
}

static PyObject* py_create_minhash(PyObject* self, PyObject* args) {
    const char* input_string;
    int num_hashes;
    if (!PyArg_ParseTuple(args, "si", &input_string, &num_hashes))
        return NULL;

    uint32_t* minhash = create_minhash(input_string, num_hashes);
    if (minhash == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create minhash");
        return NULL;
    }

    #ifdef NUMPY_2_0_PLUS
        PyObject* np_array = PyArray_SimpleNewFromData(1, (npy_intp[]){num_hashes}, NPY_UINT32, minhash);
    #else
        npy_intp dims[1] = {num_hashes};
        PyObject* np_array = PyArray_SimpleNewFromData(1, dims, NPY_UINT32, minhash);
    #endif

    if (np_array == NULL) {
        free(minhash);
        return NULL;
    }

    PyArray_ENABLEFLAGS((PyArrayObject*)np_array, NPY_ARRAY_OWNDATA);
    return np_array;
}

static PyObject* py_scan_files(PyObject* self, PyObject* args) {
    const char* directory;
    PyObject* py_query_minhash;
    float threshold;
    if (!PyArg_ParseTuple(args, "sOf", &directory, &py_query_minhash, &threshold))
        return NULL;

    PyArrayObject* np_query_minhash = (PyArrayObject*)PyArray_FROM_OTF(py_query_minhash, NPY_UINT32, NPY_ARRAY_IN_ARRAY);
    if (np_query_minhash == NULL) {
        return NULL;
    }

    int minhash_size = (int)PyArray_SIZE(np_query_minhash);
    uint32_t* query_minhash = (uint32_t*)PyArray_DATA(np_query_minhash);

    int num_matches;
    MatchResult* matches = scan_files(directory, query_minhash, minhash_size, threshold, &num_matches);

    PyObject* result_list = PyList_New(num_matches);
    for (int i = 0; i < num_matches; i++) {
        PyObject* match_tuple = Py_BuildValue("sf", matches[i].filename, matches[i].similarity);
        PyList_SetItem(result_list, i, match_tuple);
    }

    free_matches(matches, num_matches);
    Py_DECREF(np_query_minhash);

    return result_list;
}

static PyMethodDef RagFileMethods[] = {
    {"get_file_header_type", get_file_header_type, METH_NOARGS, "Get FileHeader type"},
    {"read_file_header", py_read_file_header, METH_VARARGS, "Read file header"},
    {"create_minhash", py_create_minhash, METH_VARARGS, "Create minhash"},
    {"scan_files", py_scan_files, METH_VARARGS, "Scan files for matches"},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef ragfilemodule = {
    PyModuleDef_HEAD_INIT,
    "ragfile",
    "RagFile extension module",
    -1,
    RagFileMethods
};

PyMODINIT_FUNC PyInit_ragfile(void) {
    import_array();
    if (PyErr_Occurred()) return NULL;
    return PyModule_Create(&ragfilemodule);
}
