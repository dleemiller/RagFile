#include "similarity.h"
#include "../core/minhash.h"
#include "../algorithms/jaccard.h"
#include "../algorithms/hamming.h"
#include "../algorithms/cosine.h"
#include "../search/heap.h"
#include "../search/scan.h"

// Methods for similarity calculations
PyObject* PyRagFile_jaccard(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    if (!PyArg_ParseTuple(args, "O!", Py_TYPE(self), &other)) {
        return NULL;
    }

    float similarity = jaccard_similarity(self->rf->header.minhash_signature, 
                                          other->rf->header.minhash_signature);
    return PyFloat_FromDouble(similarity);
}

PyObject* PyRagFile_hamming(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    if (!PyArg_ParseTuple(args, "O!", Py_TYPE(self), &other)) {
        return NULL;
    }

    float similarity = hamming_similarity(self->rf->header.binary_embedding, 
                                          other->rf->header.binary_embedding,
                                          self->rf->file_metadata.embedding_dim);
    return PyFloat_FromDouble(similarity);
}

// Compute Cosine Similarity with options for max or avg
PyObject* PyRagFile_cosine(PyRagFile* self, PyObject* args) {
    PyRagFile* other;
    const char* mode = "max";

    if (!PyArg_ParseTuple(args, "O!|s", &PyRagFileType, &other, &mode)) {
        return NULL;
    }

    // Assume embedding dimension and number of embeddings are part of file_metadata or a similar accessible structure
    int embedding_dim = self->rf->file_metadata.embedding_dim;
    int num_embeddings_self = self->rf->file_metadata.num_embeddings;
    int num_embeddings_other = other->rf->file_metadata.num_embeddings;

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

    static char *kwlist[] = {"file_iter", "top_k", NULL};

    // Parse Python keyword arguments
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OI", kwlist, &file_iter, &top_k)) {
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

        int process_status = process_file(path, self->rf, heap);
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

    // Extract elements from the heap and append them to the Python list
    while (heap->size > 0) {
        FileScore min_score = heap->heap[0];  // Get the root, which has the minimum score
        PyObject* dict = Py_BuildValue("{s:s, s:f}", "file", min_score.path, "jaccard", min_score.score);
        if (PyList_Append(result_list, dict) == -1) {
            Py_XDECREF(dict);
            Py_DECREF(result_list);
            free_min_heap(heap);
            PyErr_SetString(PyExc_MemoryError, "Failed to append to list");
            return NULL;
        }
        Py_DECREF(dict);
        remove_root(heap);  // Remove the root to get the next item
    }

    // Now reverse the list to get it in descending order of score
    if (PyList_Reverse(result_list) == -1) {
        Py_DECREF(result_list);
        free_min_heap(heap);
        PyErr_SetString(PyExc_RuntimeError, "Failed to reverse the list");
        return NULL;
    }

    free_min_heap(heap);
    return result_list;
}

