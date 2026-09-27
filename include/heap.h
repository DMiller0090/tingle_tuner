#ifndef GUARD_HEAP_H
#define GUARD_HEAP_H

#include "global.h"

// A heap is a header plus a run of blocks. Each block starts with a u16: its payload size
// (even) with bit 0 set when in use. A 0xFFFF word ends the run.
#define HEAP_BLOCK_USED 1
#define HEAP_END        0xFFFF

struct Heap
{
    /*0x0*/ u16 *firstFree;  // lowest free block, or NULL
    /*0x4*/ u16 size;        // bytes from blocks[] to the end marker
    /*0x6*/ u8 unk6;
    /*0x7*/ u8 unk7;
    /*0x8*/ u16 blocks[1];
};

void HeapInit(struct Heap *heap, u32 size);
void *HeapAlloc(struct Heap *heap, u32 size);
void HeapFree(struct Heap *heap, void *ptr);
void HeapCompact(struct Heap *heap);

#endif // GUARD_HEAP_H
