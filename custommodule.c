#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include "structmember.h"
#include <numpy/arrayobject.h>
#include <stdio.h>

PyObject *add(PyObject *self, PyObject *args){
	int x;
	int y;
	PyArg_ParseTuple(args, "ii", &x, &y);
	return PyLong_FromLong(x+y);
}

PyObject *sub(PyObject *self, PyObject *args){
	int x;
	int y;
	PyArg_ParseTuple(args, "ii", &x, &y);
	return PyLong_FromLong(x-y);
}

//officially these functions should be static to hide them within the shared object
static PyObject *sum(PyObject *self, PyObject *args){
	PyArrayObject *arr;
	PyArg_ParseTuple(args, "O", &arr);
	if (PyErr_Occurred()){
		return NULL;
	}
	if (!PyArray_Check(arr)) {
		PyErr_SetString(PyExc_TypeError, "Argument must be a numpy array");
		return NULL;
	}

	int64_t size = PyArray_SIZE(arr); //len(arr)
	double *data;// = PyArray_DATA(arr);
	npy_intp dims[] = { [0] = size };
	PyArray_AsCArray((PyObject **)&arr, &data, dims, 1, PyArray_DescrFromType(NPY_DOUBLE));
	if (PyErr_Occurred()) {
		return NULL;
	}
	
	double total = 0;
	for (int i=0; i<size; ++i){
		total += data[i];
	}
	return PyFloat_FromDouble(total);
}

static PyObject *np_double_elts(PyObject *self, PyObject *args){
	PyArrayObject *arr;
	PyArg_ParseTuple(args, "O", &arr);
	if (PyErr_Occurred()){
		return NULL;
	}
	if (!PyArray_Check(arr)) {
		PyErr_SetString(PyExc_TypeError, "Argument must be a numeric numpy array");
		return NULL;
	}

	int64_t size = PyArray_SIZE(arr); //len(arr)
	double *data;// = PyArray_DATA(arr);
	npy_intp dims[] = { [0] = size };
	PyArray_AsCArray((PyObject **)&arr, &data, dims, 1, PyArray_DescrFromType(NPY_DOUBLE));
	if (PyErr_Occurred()) {
		return NULL;
	}
	
	PyObject *result = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
	double *result_data = PyArray_DATA((PyArrayObject *)result);

	for (int i=0; i<size; ++i){
		result_data[i] = 2 * data[i];
	}
	return result;
}


typedef struct {
    PyObject_HEAD
    PyObject *first; // first name 
    PyObject *last;  // last name 
    int number;
} CustomObject;

static void
Custom_dealloc(CustomObject *self)
{
    Py_XDECREF(self->first);
    Py_XDECREF(self->last);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *
Custom_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    CustomObject *self;
    self = (CustomObject *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->first = PyUnicode_FromString("");
        if (self->first == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        self->last = PyUnicode_FromString("");
        if (self->last == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        self->number = 0;
    }
    return (PyObject *) self;
}

static int
Custom_init(CustomObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"first", "last", "number", NULL};
    PyObject *first = NULL, *last = NULL, *tmp;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOi", kwlist,
                                     &first, &last,
                                     &self->number))
        return -1;

    if (first) {
        tmp = self->first;
        Py_INCREF(first);
        self->first = first;
        Py_XDECREF(tmp);
    }
    if (last) {
        tmp = self->last;
        Py_INCREF(last);
        self->last = last;
        Py_XDECREF(tmp);
    }
    return 0;
}


static PyMemberDef Custom_members[] = {
    {"first", T_OBJECT_EX, offsetof(CustomObject, first), 0,
     "first name"},
    {"last", T_OBJECT_EX, offsetof(CustomObject, last), 0,
     "last name"},
    {"number", T_INT, offsetof(CustomObject, number), 0,
     "custom number"},
    {NULL}  // Sentinel
};

static PyObject *
Custom_name(CustomObject *self, PyObject *Py_UNUSED(ignored))
{
    if (self->first == NULL) {
        PyErr_SetString(PyExc_AttributeError, "first");
        return NULL;
    }
    if (self->last == NULL) {
        PyErr_SetString(PyExc_AttributeError, "last");
        return NULL;
    }
    return PyUnicode_FromFormat("%S %S", self->first, self->last);
}

static PyMethodDef Custom_methods[] = {
	{"name", (PyCFunction) Custom_name, METH_NOARGS, "Return the name, combining the first and last name"},
    {NULL}  /* Sentinel */
};
static PyMethodDef Custom_functions[] = {
  	{ "add", add, METH_VARARGS, "Adds two integers" },
	{ "sub", add, METH_VARARGS, "Subtracts two integers" },
	{ "sum", sum, METH_VARARGS, "calculates sum of nparray" },
	{ "double", np_double_elts, METH_VARARGS, "doubles all elements of nparray" },
    {NULL}  /* Sentinel */
};

static PyTypeObject CustomType = {
    .ob_base = PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "custommodule.Custom",
    .tp_doc = PyDoc_STR("Custom objects"),
    .tp_basicsize = sizeof(CustomObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = Custom_new,
    .tp_init = (initproc) Custom_init,
    .tp_dealloc = (destructor) Custom_dealloc,
    .tp_members = Custom_members,
    .tp_methods = Custom_methods,
};

static PyModuleDef custommodule = {
    .m_base =PyModuleDef_HEAD_INIT,
    .m_name = "custommodule",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
	//methods?
	.m_methods = Custom_functions
};

PyMODINIT_FUNC
PyInit_custommodule(void)
{
	printf("Hello from Custom C Module!\n");
    PyObject *m;
    if (PyType_Ready(&CustomType) < 0)
    	return NULL;

    m = PyModule_Create(&custommodule);
    if (m == NULL)
        return NULL;

    Py_INCREF(&CustomType);
    if (PyModule_AddObject(m, "Custom", (PyObject *) &CustomType) < 0) {
        Py_DECREF(&CustomType);
        Py_DECREF(m);
        return NULL;
    }
	import_array()
	printf("Returning from PyInit\n");
    return m;
}

