#define IMG_SIZE (1024 * 1024 * 1024)
#define REQUESTING_ADDR ((void* )0x280000000L)

#include "Python.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define USING_MMAP 1

static char *shm;
struct header *h;
static int n_alloc;

struct header {
    void *mapped_addr;
    void *none_addr;
    void *true_addr;
    void *false_addr;
    void *ellipsis_addr;
    size_t used;
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
_PyMem_CreateSharedMmap(wchar_t *const archive)
{
    int fd = open(Py_EncodeLocale(archive, NULL), O_RDWR | O_CREAT | O_TRUNC, 0666);
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
    h->used = 4096;
    return shm;
}

void patch_obj_header(void);

#ifdef MAP_POPULATE
#define M_POPULATE MAP_POPULATE
#else
#define M_POPULATE 0
#endif

void *
_PyMem_LoadSharedMmap(wchar_t *const archive)
{
    long t0 = nanoTime();
    char *local_archive = Py_EncodeLocale(archive, NULL);
    if (local_archive == NULL) {
        // todo: error handling
    }
    int fd = open(local_archive, O_RDWR);
    struct stat buf;
    fstat(fd, &buf);
    struct header hbuf;
    if (read(fd, &hbuf, sizeof(hbuf)) != sizeof(hbuf)) {
        perror("read header");
        abort();
    }
    printf("[sharedheap] requesting %p...", hbuf.mapped_addr);
    size_t aligned_size = (hbuf.used + 4095) & ~4095;
    shm = mmap(hbuf.mapped_addr, aligned_size,
               PROT_READ | PROT_WRITE,
               MAP_PRIVATE | MAP_FIXED | (USING_MMAP ? M_POPULATE : MAP_ANONYMOUS),
               fd, 0);
    long t1 = nanoTime();
    if (shm == MAP_FAILED || shm != hbuf.mapped_addr) {
        perror("mmap");
        abort();
    }
    h = (struct header *) shm;
    if (USING_MMAP) {
        for (int i = 0; i < h->used; i += 4096) {
            ((char volatile *) shm)[i] += 0;
        }
    } else {
        lseek(fd, 0, SEEK_SET);
        int nread = 0;
        while (nread < hbuf.used) {
            nread += read(fd, shm + nread, hbuf.used - nread);
        }
    }
    close(fd);
    long t2 = nanoTime();
    printf("SUCCESS elapsed=%ld ns (%ld + %ld)\n", t2 - t0, t1 - t0, t2 - t1);
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
        type->tp_after_patch(op, shift);
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
    PyObject *op = *opp;
    if (op == h->none_addr || op == h->true_addr || op == h->false_addr || op == h->ellipsis_addr) {
        *opp = (PyObject *) ((char *) op + (long) shift);
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
    printf("[sharedheap] ASLR data segment shift = %c0x%lx\n", shift < 0 ? '-' : ' ', labs(shift));
    long t0 = nanoTime();
    if (shift) {
        if (h->obj) {
            patch_type1(&h->obj, (void *) shift);
        }
        h->none_addr = Py_None;
        h->true_addr = Py_True;
        h->false_addr = Py_False;
        h->ellipsis_addr = Py_Ellipsis;
    }
    printf("[sharedheap] ASLR fix FINISH, elapsed=%ld ns\n", nanoTime() - t0);
}

static void *
_PyMem_SharedMalloc(size_t size)
{
    n_alloc++;
    if (!size) size = 1;
    size_t size_aligned = (size + 7) & ~7;
    void *res = shm + h->used;
    h->used += size_aligned;
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
    assert(!h->obj);
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

