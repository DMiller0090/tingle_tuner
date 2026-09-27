#include "global.h"
#include "heap.h"

// First-fit heap. gMain.heap is the only instance (JoyCom keeps its IRQ copy there).

// The block headers are written through an s16 pointer (the size mask stays 0xFFFE) and the
// end marker through a u16 one (0xFFFF from the pool rather than -1).
void HeapInit(struct Heap *heap, u32 size)
{
    s16 *blocks = (s16 *)heap->blocks;
    u16 *end = (u16 *)((u8 *)blocks + (size - 10));

    *blocks = (size - 12) & ~HEAP_BLOCK_USED;
    *end = HEAP_END;
    heap->firstFree = (u16 *)blocks;
    heap->size = size - 8;
    heap->unk6 = 0;
    heap->unk7 = 0;
}

INCLUDE_ASM(HeapAlloc);

// Nearly matches with `volatile s16 *block = (volatile s16 *)ptr - 1;` (re-read and the -2
// mask come out right), but r0/r1 are swapped around the AND:
//     if (ptr != NULL && (*block & HEAP_BLOCK_USED))
//     {
//         *block &= ~HEAP_BLOCK_USED;
//         if (heap != NULL && (heap->firstFree > block || heap->firstFree == NULL))
//             heap->firstFree = block;
//     }
INCLUDE_ASM(HeapFree);

// Merge each run of adjacent free blocks into one and recompute firstFree.
void HeapCompact(struct Heap *heap)
{
    u16 *firstFree = NULL;
    u16 *block = heap->blocks;

    while (*block != HEAP_END)
    {
        u16 header = *block;
        u32 len = (header & ~HEAP_BLOCK_USED) + 2;
        u16 *next = (u16 *)((u8 *)block + len);

        if (!(header & HEAP_BLOCK_USED))
        {
            if (!(*next & HEAP_BLOCK_USED))
            {
                do
                {
                    len += (*next & ~HEAP_BLOCK_USED) + 2;
                    next = (u16 *)((u8 *)block + len);
                } while (!(*next & HEAP_BLOCK_USED));
                *block = (len - 2) & ~HEAP_BLOCK_USED;
            }
            if (firstFree == NULL)
                firstFree = block;
        }
        block = next;
    }
    heap->firstFree = firstFree;
}
