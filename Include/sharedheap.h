#ifndef CPYTHON_INCLUDE_SHAREDHEAP_H_
#define CPYTHON_INCLUDE_SHAREDHEAP_H_

#include "Python.h"

#define ALIEN_TO(size, align) (((size) + ((align)-1)) & ~((align)-1))
#define UNSHIFT(p, shift, ty) ((ty *)((void *)(p) + (shift)))

/* Indicates that I know what I'm doing when casting one pointer to an
 * unrelated type. */
#define REINTERPRET_CAST(t, p) ((t *)(void *)(p))

struct HeapArchivedObject {
    PyTypeObject *type;
    PyObject *ready_obj;

    void *obj;
};

struct HeapArchiveHeader {
    void *mapped_addr;
    void *none_addr;
    void *true_addr;
    void *false_addr;
    void *ellipsis_addr;
    size_t used;

    struct HeapArchivedObject *obj;
};

struct HeapArchivedObject *
serialize(PyObject *op, void *(*alloc)(size_t));
PyObject *
deserialize(struct HeapArchivedObject *archived_object, long shift);

void
_PyMem_SharedMoveIn(PyObject *o);
PyObject *
_PyMem_SharedGetObj(void);

int
_PyMem_IsShared(void *ptr);

#endif  // CPYTHON_INCLUDE_SHAREDHEAP_H_
