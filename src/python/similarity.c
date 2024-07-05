#include "similarity.h"
#include "../utils/file_io.h"
#include "../algorithms/minhash.h"
#include "../algorithms/jaccard.h"
#include "../algorithms/hamming.h"
#include "../algorithms/cosine.h"
#include "../search/heap.h"
#include "../search/scan.h"

// Methods for similarity calculations
PyObject* PyRagFile_jaccard(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    int vec_num = 1;
    if (!PyArg_ParseTuple(args, "O!|i", Py_TYPE(self), &other, &vec_num)) {
        return NULL;
    }

    VecInfo query_info = get_vec_info(self->rf, vec_num);
    VecInfo candidate_info = get_vec_info(other->rf, vec_num);

    if (query_info.type != MIN_HASH || candidate_info.type != MIN_HASH) {
        PyErr_SetString(PyExc_ValueError, "Jaccard similarity requires MIN_HASH type vectors");
        return NULL;
    }

    float similarity = jaccard_similarity(query_info.vec, candidate_info.vec, fmin(query_info.dim, candidate_info.dim));
    return PyFloat_FromDouble(similarity);
}

PyObject* PyRagFile_hamming(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    int vec_num = 1;
    if (!PyArg_ParseTuple(args, "O!|i", Py_TYPE(self), &other, &vec_num)) {
        return NULL;
    }

    VecInfo query_info = get_vec_info(self->rf, vec_num);
    VecInfo candidate_info = get_vec_info(other->rf, vec_num);

    if (query_info.type != BINARY_EMBEDDING || candidate_info.type != BINARY_EMBEDDING) {
        PyErr_SetString(PyExc_ValueError, "Hamming similarity requires BINARY_EMBEDDING type vectors");
        return NULL;
    }

    float similarity = hamming_similarity((const uint8_t*)query_info.vec, (const uint8_t*)candidate_info.vec, fmin(query_info.dim, candidate_info.dim));
    return PyFloat_FromDouble(similarity);
}

// Compute Cosine Similarity with options for max or avg
PyObject* PyRagFile_cosine(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    const char* mode = "max";

    if (!PyArg_ParseTuple(args, "O!|s", &PyRagFileType, &other, &mode)) {
        return NULL;
    }

    int embedding_dim = self->rf->header.embedding_dim;
    int num_embeddings_self = self->rf->header.num_embeddings;
    int num_embeddings_other = other->rf->header.num_embeddings;

    float max_similarity = -1.0f;
    float total_similarity = 0.0f;
    int count = 0;

    for (int i = 0; i < num_embeddings_self; i++) {
        for (int j = 0; j < num_embeddings_other; j++) {
            float* emb1 = self->rf->embeddings + (i * embedding_dim);
            float* emb2 = other->rf->embeddings + (j * embedding_dim);
            float similarity = cosine_similarity(emb1, emb2, embedding_dim);
            max_similarity = fmax(max_similarity, similarity);
            total_similarity += similarity;
            count++;
        }
    }

    float result = strcmp(mode, "avg") == 0 ? total_similarity / count : max_similarity;
    return PyFloat_FromDouble(result);
}

// Scanning
PyObject* PyRagFile_match(PyRagFile* self, PyObject* args, PyObject* kwds) {
    PyObject* file_iter;
    unsigned int top_k;
    const char* method = "jaccard";
    int vec_num = 1;

    static char *kwlist[] = {"file_iter", "top_k", "method", "use_alt_vector", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OI|si", kwlist, &file_iter, &top_k, &method, &vec_num)) {
        return NULL;
    }

    if (!PyIter_Check(file_iter)) {
        PyErr_SetString(PyExc_TypeError, "file_iter must be an iterator");
        return NULL;
    }

    if (top_k == 0) {
        PyErr_SetString(PyExc_ValueError, "top_k must be greater than 0");
        return NULL;
    }

    MinHeap* heap = create_min_heap(top_k);
    if (heap == NULL) {
        PyErr_SetString(PyExc_MemoryError, "Failed to create a heap");
        return NULL;
    }

    PyObject* file_path;
    while ((file_path = PyIter_Next(file_iter)) != NULL) {
        const char* path = PyUnicode_AsUTF8(file_path);
        if (path == NULL) {
            PyErr_Format(PyExc_ValueError, "Invalid file path encountered");
            Py_DECREF(file_path);
            continue;
        }

        int process_status;
        if (strcmp(method, "jaccard") == 0) {
            process_status = process_file_jaccard(path, self->rf, heap, vec_num);
        } else if (strcmp(method, "hamming") == 0) {
            process_status = process_file_hamming(path, self->rf, heap, vec_num);
        } else {
            PyErr_SetString(PyExc_ValueError, "Invalid method specified");
            Py_DECREF(file_path);
            free_min_heap(heap);
            return NULL;
        }

        Py_DECREF(file_path);
        if (process_status != 0) {
            PyErr_SetString(PyExc_RuntimeError, "File processing failed");
            free_min_heap(heap);
            return NULL;
        }
    }

    PyObject* result_list = PyList_New(0);
    if (result_list == NULL) {
        free_min_heap(heap);
        PyErr_SetString(PyExc_MemoryError, "Failed to create list");
        return NULL;
    }

    while (heap->size > 0) {
        FileScore min_score = heap->heap[0];
        PyObject* dict = Py_BuildValue("{s:s, s:f}", "file", min_score.path, method, min_score.score);
        if (PyList_Append(result_list, dict) == -1) {
            Py_XDECREF(dict);
            Py_DECREF(result_list);
            free_min_heap(heap);
            PyErr_SetString(PyExc_MemoryError, "Failed to append to list");
            return NULL;
        }
        Py_DECREF(dict);
        remove_root(heap);
    }

    if (PyList_Reverse(result_list) == -1) {
        Py_DECREF(result_list);
        free_min_heap(heap);
        PyErr_SetString(PyExc_RuntimeError, "Failed to reverse the list");
        return NULL;
    }

    free_min_heap(heap);
    return result_list;
}

