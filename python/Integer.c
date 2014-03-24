/*
 * Coefficient.c
 *
 *  Created on: Nov 8, 2013
 *      Author: dejan
 */

#include "Integer.h"

#include <structmember.h>

static void
CoefficientRing_dealloc(CoefficientRing* self);

static PyObject*
CoefficientRing_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static int
CoefficientRing_init(CoefficientRing* self, PyObject* args);

static int
CoefficientRing_cmp(PyObject* self, PyObject* args);

static PyObject*
CoefficientRing_modulus(PyObject* self);

static PyObject*
CoefficientRing_str(PyObject* self);

PyMethodDef CoefficientRing_methods[] = {
    {"modulus", (PyCFunction)CoefficientRing_modulus, METH_NOARGS, "Returns the degree of the polynomial"},
    {NULL}  /* Sentinel */
};

PyTypeObject CoefficientRingType = {
    PyObject_HEAD_INIT(NULL)
    0,                          /*ob_size*/
    "polypy.CoefficientRing",   /*tp_name*/
    sizeof(CoefficientRing), /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)CoefficientRing_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    CoefficientRing_cmp,      /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    CoefficientRing_str,            /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_CHECKTYPES, /*tp_flags*/
    "Coefficient ring objects", /* tp_doc */
    0,                             /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    CoefficientRing_methods,       /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)CoefficientRing_init,/* tp_init */
    0,                         /* tp_alloc */
    CoefficientRing_new,           /* tp_new */
};

static void
CoefficientRing_dealloc(CoefficientRing* self)
{
  if (self->K) int_ring_ops.detach(self->K);
  self->ob_type->tp_free((PyObject*)self);
}

PyObject*
PyCoefficientRing_create(int_ring K) {
  CoefficientRing *self;
  self = (CoefficientRing*)CoefficientRingType.tp_alloc(&CoefficientRingType, 0);
  if (self != NULL) {
    self->K = K;
  }
  return (PyObject *)self;
}

static PyObject*
CoefficientRing_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  return PyCoefficientRing_create(0);
}

static int
CoefficientRing_init(CoefficientRing* self, PyObject* args)
{
    if (PyTuple_Check(args)) {
      if (PyTuple_Size(args) == 0) {
        // Defaults to Z
        self->K = Z;
      } else if (PyTuple_Size(args) == 1) {
        // Get the list of coefficients
        PyObject* modulus = PyTuple_GetItem(args, 0);
        if (!PyLong_Check(modulus)) {
          if (!PyInt_Check(modulus)) {
            return -1;
          } else {
            long M_int = PyInt_AsLong(modulus);
            integer_t M;
            integer_ops.construct_from_int(Z, &M, M_int);
            int is_prime = integer_ops.is_prime(&M);
            self->K = int_ring_ops.create(&M, is_prime);
            integer_ops.destruct(&M);
          }
        } else {
          int overflow = 0;
          long M_int = PyLong_AsLongAndOverflow(modulus, &overflow);
          if (overflow) {
            PyObject* M_str = PyObject_Str(modulus);
            char* M_cstr = PyString_AsString(M_str);
            integer_t M;
            integer_ops.construct_from_string(Z, &M, M_cstr, 10);
            int is_prime = integer_ops.is_prime(&M);
            self->K = int_ring_ops.create(&M, is_prime);
            Py_DECREF(M_str);
          } else if (M_int > 0) {
            integer_t M;
            integer_ops.construct_from_int(Z, &M, M_int);
            int is_prime = integer_ops.is_prime(&M);
            self->K = int_ring_ops.create(&M, is_prime);
          } else {
            return -1;
          }
        }
      } else {
        return -1;
      }
    } else {
      return -1;
    }

    return 0;
}

static int
CoefficientRing_cmp(PyObject* self, PyObject* other) {
  // Check arguments
  if (!PyCoefficientRing_CHECK(self) || !PyCoefficientRing_CHECK(other)) {
    // should return -1 and set an exception condition when an error occurred
    return -1;
  }
  // Get arguments
  CoefficientRing* K1 = (CoefficientRing*) self;
  CoefficientRing* K2 = (CoefficientRing*) other;
  // Are they equal
  if (K1->K == K2->K) {
    return 0;
  }
  // Is one of them Z
  if (K1->K == Z) {
    return 1;
  }
  if (K2->K == Z) {
    return -1;
  }
  // Compare
  return integer_ops.cmp(Z, &K1->K->M, &K2->K->M);
}

static PyObject*
CoefficientRing_modulus(PyObject* self) {
  CoefficientRing* K = (CoefficientRing*) self;
  if (K && K->K) {
    char* K_str = integer_ops.to_string(&K->K->M);
    char* p = 0;
    PyObject* M = PyLong_FromString(K_str, &p, 10);
    free(K_str);
    return M;
  } else {
    Py_RETURN_NONE;
  }
}

static PyObject* CoefficientRing_str(PyObject* self) {
  CoefficientRing* K = (CoefficientRing*) self;
  if (K) {
    if (K->K) {
      char* K_str = int_ring_ops.to_string(K->K);
      PyObject* str = PyString_FromString(K_str);
      free(K_str);
      return str;
    } else {
      return PyString_FromString("Z");
    }
  } else {
    Py_RETURN_NONE;
  }
}
