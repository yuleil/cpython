#define IMG_FILE "heap.img"
#define IMG_SIZE (1024 * 1024 * 1024)
#define REQUESTING_ADDR ((void* )0x280000000L)

#include "Python.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


static char *shm;
struct header *h;
static size_t offset = 4096;
static int n_alloc;

struct header {
    void *mapped_addr;
    void *none_addr;
    void *true_addr;
    void *false_addr;
    void *ellipsis_addr;
    PyObject *obj;
};

static inline long
nanoTime()
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000000000 + t.tv_nsec;
}

void *
_PyMem_CreateSharedMmap(void)
{
    int fd = open(IMG_FILE, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        exit(-1);
    }
    ftruncate(fd, IMG_SIZE);

    shm = mmap(REQUESTING_ADDR, IMG_SIZE, PROT_READ | PROT_WRITE,
               MAP_SHARED, fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        abort();
    }
    close(fd);
    h = (struct header *) shm;
    h->mapped_addr = shm;
    h->none_addr = Py_None;
    h->true_addr = Py_True;
    h->false_addr = Py_False;
    h->ellipsis_addr = Py_Ellipsis;
    return shm;
}

void patch_obj_header(void);


void *
_PyMem_LoadSharedMmap(void)
{
    int fd = open(IMG_FILE, O_RDWR);
    struct stat buf;
    fstat(fd, &buf);
    struct header tmp_header;
    if (read(fd, &tmp_header, sizeof(tmp_header)) != sizeof(tmp_header)) {
        perror("read header");
        abort();
    }
    printf("[sharedheap] requesting %p...", tmp_header.mapped_addr);
    long t0 = nanoTime();
    shm = mmap(tmp_header.mapped_addr, buf.st_size, PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_FIXED, fd, 0);
    if (shm == MAP_FAILED || shm != tmp_header.mapped_addr) {
        perror("mmap");
        abort();
    }
    close(fd);
    printf("SUCCESS elapsed=%ld ns\n", nanoTime() - t0);
    h = (struct header *) shm;
    patch_obj_header();
    return shm;
}

// typedef int (*visitproc)(PyObject *, void *);
// typedef int (*traverseproc)(PyObject *, visitproc, void *);
int patch_type1(PyObject **opp, void *shift);

int
patch_type(PyObject *op, void *shift)
{
    PyTypeObject *type = (PyTypeObject *) ((char *) Py_TYPE(op) + (long) shift);
    Py_SET_TYPE(op, type);
    if (type->tp_after_patch) {
        type->tp_after_patch(op);
    }
    if (type->tp_traverse1) {
        type->tp_traverse1(op, patch_type1, shift);
    } else if (type->tp_traverse) {
        type->tp_traverse(op, patch_type, shift);
    }
    return 0;
}

int
patch_type1(PyObject **opp, void *shift)
{
    if (*opp == h->none_addr) {
        *opp = Py_None;
    } else if (*opp == h->true_addr) {
        *opp = Py_True;
    } else if (*opp == h->false_addr) {
        *opp = Py_False;
    } else if (*opp == h->ellipsis_addr) {
        *opp = Py_Ellipsis;
    } else {
        patch_type(*opp, shift);
    }
    return 0;
}

void
patch_obj_header(void)
{
    assert(shm);
    long shift = (char *) Py_None - (char *) h->none_addr;
    printf("[sharedheap] ASLR data segment shift = %c0x%lx\n", shift < 0 ? '-' : ' ', (shift < 0) ? -shift : shift);
    long t0 = nanoTime();
    if (shift && h->obj) {
        patch_type1(&h->obj, (void *) shift);
    }
    printf("[sharedheap] ASLR fix FINISH, elapsed=%ld ns\n", nanoTime() - t0);
}

static void *
_PyMem_SharedMalloc(size_t size)
{
    n_alloc++;
    if (!size) size = 1;
    size_t size_aligned = (size + 7) & ~7;
    void *res = shm + offset;
    offset += size_aligned;
    printf("[sharedheap] size=%ld, size_aligned=%ld, p = %p\n", size, size_aligned, res);
    return res;
}

int
_PyMem_IsShared(void *ptr)
{
    return shm && (char *) ptr >= shm && (char *) ptr < (shm + IMG_SIZE);
}

void
_PyMem_SharedMoveIn(PyObject *o)
{
    PyTypeObject *type = Py_TYPE(o);
    assert(type->tp_copy);
    PyObject *copy = type->tp_copy(o, _PyMem_SharedMalloc);
    assert(_PyMem_IsShared(copy));
    printf("[sharedheap] deep copy a `%s object`@%p to %p\n", Py_TYPE(copy)->tp_name, o, copy);
    h->obj = copy;
}

PyObject *
_PyMem_SharedGetObj(void)
{
    if (!h->obj) return Py_None;
    Py_INCREF(h->obj);
    return h->obj;
}

