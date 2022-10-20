/*
 * Memory allocator for TinyGL
 */
#include "zgl.h"
#include "ea_malloc.h"

/* modify these functions so that they suit your needs */

void gl_free(void *p)
{
    ea_free(p);
}

void *gl_malloc(int size)
{
    return ea_malloc(size);
}

void *gl_zalloc(int size)
{
    uint8_t* ptr = (uint8_t*)ea_malloc(size);
    memset(ptr, 0, size);
    return ptr;
}
