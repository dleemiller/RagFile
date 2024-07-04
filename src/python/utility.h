#ifndef UTILITY_H
#define UTILITY_H

#include <Python.h>

int prepare_embeddings(PyObject* embeddings_obj, float** flattened, size_t* total_floats, uint32_t* num_embeddings, uint32_t* embedding_dim);

#endif // UTILITY_H

