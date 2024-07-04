#include "utility.h"

// Validate embeddings and tokens, and prepare the embeddings array
int prepare_embeddings(PyObject* embeddings_obj, float** flattened, size_t* total_floats, uint32_t* num_embeddings, uint32_t* embedding_dim) {
    if (!PyList_Check(embeddings_obj)) {
        PyErr_SetString(PyExc_TypeError, "embeddings must be a list of lists of floats");
        return 0;
    }

    *num_embeddings = PyList_Size(embeddings_obj);
    if (*num_embeddings == 0) {
        PyErr_SetString(PyExc_ValueError, "embeddings list cannot be empty");
        return 0;
    }

    // Check the first embedding to determine the dimension
    PyObject* first_emb = PyList_GetItem(embeddings_obj, 0);
    if (!PyList_Check(first_emb)) {
        PyErr_SetString(PyExc_TypeError, "Each embedding must be a list of floats");
        return 0;
    }

    *embedding_dim = PyList_Size(first_emb);
    *total_floats = *num_embeddings * *embedding_dim;
    *flattened = (float*)malloc(*total_floats * sizeof(float));
    if (*flattened == NULL) {
        PyErr_NoMemory();
        return 0;
    }

    float* ptr = *flattened;
    for (Py_ssize_t i = 0; i < *num_embeddings; i++) {
        PyObject* emb_list = PyList_GetItem(embeddings_obj, i);
        if (!PyList_Check(emb_list) || PyList_Size(emb_list) != *embedding_dim) {
            PyErr_SetString(PyExc_TypeError, "All embeddings must be lists of floats of the same length");
            free(*flattened);
            return 0;
        }
        for (Py_ssize_t j = 0; j < *embedding_dim; j++) {
            PyObject* float_obj = PyList_GetItem(emb_list, j);
            if (!PyFloat_Check(float_obj)) {
                PyErr_SetString(PyExc_TypeError, "Embedding values must be floats");
                free(*flattened);
                return 0;
            }
            *ptr++ = (float)PyFloat_AsDouble(float_obj);
        }
    }
    return 1;
}

