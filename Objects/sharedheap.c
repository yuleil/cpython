#define IMG_SIZE (1024 * 1024 * 1024)
//#define REQUESTING_ADDR ((void *)0x280000000L)
#define REQUESTING_ADDR ((void *)0x300000000L)

#include "sharedheap.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define USING_MMAP 1

static char *shm;
struct HeapArchiveHeader *h;
static int n_alloc;
static long shift;
static bool dumped;
static int fd;

const unsigned int DUMP_FLAG = 1 << 0;
const unsigned int LOAD_FLAG = 1 << 1;
const unsigned int AUTO_FLAG = 1 << 2;
const unsigned int DEBUG_MASK = 1 << 3;
const unsigned int DEBUG_DUMP_FLAG = DEBUG_MASK | DUMP_FLAG;
const unsigned int DEBUG_LOAD_FLAG = DEBUG_MASK | LOAD_FLAG;

struct HeapArchivedObject;

struct HeapArchiveHeader;

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
    fd =
        open(Py_EncodeLocale(archive, NULL), O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        exit(-1);
    }
    ftruncate(fd, IMG_SIZE);

    shm = mmap(REQUESTING_ADDR, IMG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
               fd, 0);
    if (shm == MAP_FAILED) {
        perror("mmap");
        abort();
    }
    h = (struct HeapArchiveHeader *)shm;
    h->mapped_addr = shm;
    h->none_addr = Py_None;
    h->true_addr = Py_True;
    h->false_addr = Py_False;
    h->ellipsis_addr = Py_Ellipsis;
    h->used = 4096;
    return shm;
}

void
patch_obj_header(void);

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
    fd = open(local_archive, O_RDWR);
    struct stat buf;
    fstat(fd, &buf);
    struct HeapArchiveHeader hbuf;
    if (read(fd, &hbuf, sizeof(hbuf)) != sizeof(hbuf)) {
        printf("%zd\n", read(fd, &hbuf, sizeof(hbuf)));
        perror("read header");
        abort();
    }
    if (Py_CDSVerboseFlag > 0) {
        printf("[sharedheap] requesting %p...", hbuf.mapped_addr);
    }
    size_t aligned_size = ALIEN_TO(hbuf.used, 4096);
    shm = mmap(
        hbuf.mapped_addr, aligned_size, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_FIXED | (USING_MMAP ? M_POPULATE : MAP_ANONYMOUS),
        fd, 0);
    long t1 = nanoTime();
    if (shm == MAP_FAILED) {
        perror("mmap failed");
        abort();
    }
    else if (shm != hbuf.mapped_addr) {
        perror("mmap relocated");
        abort();
    }
    h = REINTERPRET_CAST(struct HeapArchiveHeader, shm);
    if (USING_MMAP) {
        for (size_t i = 0; i < h->used; i += 4096) {
            ((char volatile *)shm)[i] += 0;
        }
    }
    else {
        lseek(fd, 0, SEEK_SET);
        size_t nread = 0;
        while (nread < hbuf.used) {
            nread += read(fd, shm + nread, hbuf.used - nread);
        }
    }
    close(fd);
    long t2 = nanoTime();
    if (Py_CDSVerboseFlag > 0) {
        printf("[sharedheap] SUCCESS elapsed=%ld ns (%ld + %ld)\n", t2 - t0,
               t1 - t0, t2 - t1);
    }
    patch_obj_header();
    return shm;
}

void
patch_obj_header(void)
{
    assert(shm);
    if (h->none_addr) {
        shift = (void *)Py_None - (void *)h->none_addr;
        h->none_addr = Py_None;

#define SCHWIFTY(p, r)                                    \
    if (UNSHIFT((p), (shift), void *) != (void *)(r)) {   \
        Py_FatalError("[sharedheap] ASLR check failed."); \
    }                                                     \
    else {                                                \
        (p) = (r);                                        \
    }
        SCHWIFTY(h->true_addr, Py_True);
        SCHWIFTY(h->false_addr, Py_False);
        SCHWIFTY(h->ellipsis_addr, Py_Ellipsis);
#undef SCHWIFTY
    }
    if (shift) {
        h->none_addr = Py_None;
        h->true_addr = Py_True;
        h->false_addr = Py_False;
        h->ellipsis_addr = Py_Ellipsis;
    }
}

static void *
_PyMem_SharedMalloc(size_t size)
{
    n_alloc++;
    if (!size)
        size = 1;
    size_t size_aligned = ALIEN_TO(size, 8);
    void *res = shm + h->used;
    if ((h->used += size_aligned) > IMG_SIZE) {
        h->used -= size_aligned;
        return NULL;
    }
    return res;
}

int
_PyMem_IsShared(void *ptr)
{
    return shm && (char *)ptr >= shm && (char *)ptr < (shm + IMG_SIZE);
}

struct HeapArchivedObject *
serialize(PyObject *op, void *(*alloc)(size_t))
{
    struct HeapArchivedObject *res =
        _PyMem_SharedMalloc(sizeof(struct HeapArchivedObject));
    res->type = Py_TYPE(op);
    if (!res->type->tp_archive_serialize) {
        char buf[128];
        sprintf(buf, "type not supported: %s\n", res->type->tp_name);
        Py_FatalError(buf);
    }
    res->obj = res->type->tp_archive_serialize(op, alloc);
    return res;
}

void
_PyMem_SharedMoveIn(PyObject *o)
{
    const PyConfig *conf = _Py_GetConfig();
    if (dumped || (conf->cds_mode & 3) != 1)
        return;

    h->used = ALIEN_TO(sizeof(struct HeapArchiveHeader), 8);
    h->obj = serialize(o, _PyMem_SharedMalloc);

    dumped = true;
    ftruncate(fd, h->used);
    close(fd);
    fd = 0;
}

PyObject *
deserialize(struct HeapArchivedObject *archived_object, long shift)
{
    if (!archived_object || !archived_object->obj) {
        return Py_None;
    }
    if (!archived_object->ready_obj) {
        deserializearchivefunc f =
            UNSHIFT(archived_object->type, shift, PyTypeObject)
                ->tp_archive_deserialize;
        assert(f);
        // There's extra INCREF in tp_archive_deserialize since we don't need
        // to free archived objects.
        archived_object->ready_obj = f(archived_object->obj, shift);
    }
    Py_INCREF(archived_object->ready_obj);
    return archived_object->ready_obj;
}

PyObject *
_PyMem_SharedGetObj()
{
    return deserialize(h->obj, shift);
}
