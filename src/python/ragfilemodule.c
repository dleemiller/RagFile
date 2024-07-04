#include <Python.h>
#include "pyragfile.h"
#include "pyragfileheader.h"

// Module definition
static PyModuleDef ragfilemodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "ragfile",
    .m_doc = "Python bindings for RagFile library",
    .m_size = -1,
    //.m_methods = ragfile_methods,
};

// Module initialization
PyMODINIT_FUNC PyInit_ragfile(void) {
    PyObject* m;

    if (PyType_Ready(&PyRagFileType) < 0)
        return NULL;

    if (PyType_Ready(&PyRagFileHeaderType) < 0)
        return NULL;

    m = PyModule_Create(&ragfilemodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&PyRagFileType);
    if (PyModule_AddObject(m, "RagFile", (PyObject*)&PyRagFileType) < 0) {
        Py_DECREF(&PyRagFileType);
        Py_DECREF(m);
        return NULL;
    }

    Py_INCREF(&PyRagFileHeaderType);
    if (PyModule_AddObject(m, "RagFileHeader", (PyObject*)&PyRagFileHeaderType) < 0) {
        Py_DECREF(&PyRagFileHeaderType);
        Py_DECREF(&PyRagFileType);
        Py_DECREF(m);
        return NULL;
    }

    // Create capsules containing the addresses of PyRagFileType and PyRagFileHeaderType
    PyObject* type_capsule = PyCapsule_New((void *)&PyRagFileType, "ragfile.PyRagFileType", NULL);
    if (!type_capsule) {
        Py_DECREF(m);
        return NULL;
    }

    PyObject* header_capsule = PyCapsule_New((void *)&PyRagFileHeaderType, "ragfile.PyRagFileHeaderType", NULL);
    if (!header_capsule) {
        Py_DECREF(type_capsule);
        Py_DECREF(m);
        return NULL;
    }

    PyModule_AddObject(m, "PyRagFileType_capsule", type_capsule);
    PyModule_AddObject(m, "PyRagFileHeaderType_capsule", header_capsule);

    return m;
}

