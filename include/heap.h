#ifndef GUARD_HEAP_H
#define GUARD_HEAP_H

#include "global.h"

void HeapInit(void *heap, u32 size);
void *HeapAlloc(void *heap, u32 size);
void HeapFree(void *heap, void *ptr);

#endif // GUARD_HEAP_H
