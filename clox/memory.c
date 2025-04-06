#include "memory.h"

#include <stdlib.h>

void *reallocate(void *previous, size_t oldSize, size_t newSize)
{
    if (newSize == 0)
    {
        free(previous);
        return NULL;
    }

    void *result = realloc(pointer, newSize);
}

void free(void *_pointer)
{
    // TODO: Implement
}