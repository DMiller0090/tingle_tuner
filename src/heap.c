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

// First fit, merging free neighbours on the way. A multiple-of-4 request gets a 4-aligned
// payload: a block whose payload isn't aligned is split into an empty block and the rest.
// Near-miss: same code shape, but block/total/avail get r4/r2/r3 where the original has
// r5/r3/r4 (and the 0xFFFE constant is loaded later); declaration order and scopes don't move it.
//     void *HeapAlloc(struct Heap *heap, u32 size)
//     {
//         bool32 align4 = FALSE;
//         u32 need;
//         u16 *block;
//         u16 *firstFree;
//         u32 header;
//
//         if ((size & 3) == 0)
//             align4 = TRUE;
//         size = (size + 1) & ~HEAP_BLOCK_USED;
//         need = size + 2;
//         if (need > heap->size)
//             return NULL;
//         block = heap->blocks;
//         firstFree = heap->firstFree;
//         if (firstFree != NULL)
//             block = firstFree;
//         if (*block == HEAP_END)
//             return NULL;
//         header = size | HEAP_BLOCK_USED;
//         do
//         {
//             u32 len = *block & ~HEAP_BLOCK_USED;
//             u32 total = len + 2;
//             u32 avail;
//             u16 *next;
//
//             if (align4)
//             {
//                 avail = total;
//                 if (((u32)block & 3) == 0)
//                     avail = len;
//             }
//             else
//             {
//                 avail = total;
//             }
//             next = (u16 *)((u8 *)block + total);
//             if (!(*block & HEAP_BLOCK_USED))
//             {
//                 if (need > avail)
//                 {
//                     if (!(*next & HEAP_BLOCK_USED))
//                     {
//                         do
//                         {
//                             u32 n = (*next & ~HEAP_BLOCK_USED) + 2;
//
//                             total += n;
//                             avail += n;
//                             next = (u16 *)((u8 *)block + total);
//                         } while (need > avail && !(*next & HEAP_BLOCK_USED));
//                         *block = (total - 2) & ~HEAP_BLOCK_USED;
//                     }
//                     if (need > avail)
//                         goto next;
//                 }
//                 if (align4 && ((u32)(block + 1) & 3) && need <= total - 2)
//                 {
//                     *block = 0;
//                     block[1] = (total - 4) & ~HEAP_BLOCK_USED;
//                     block++;
//                     total -= 2;
//                     avail = total;
//                 }
//                 if (need <= avail)
//                 {
//                     if (need < total)
//                     {
//                         u16 *rest = (u16 *)((u8 *)block + need);
//
//                         *rest = (total - need - 2) & ~HEAP_BLOCK_USED;
//                         if (firstFree > rest)
//                             heap->firstFree = rest;
//                     }
//                     *block = header;
//                     return block + 1;
//                 }
//             }
//         next:
//             block = next;
//         } while (*block != HEAP_END);
//         return NULL;
//     }
INCLUDE_ASM(HeapAlloc);

// Nearly matches (the re-read and the -2 mask come out right), but r0/r1 are swapped
// around the AND in the `&=`; operand order, casts, 0xFFFE and a u16 temporary don't
// change it (an s16 temporary gets the registers right but adds a sign extension):
//     void HeapFree(struct Heap *heap, void *ptr)
//     {
//         volatile s16 *block;
//
//         if (ptr != NULL)
//         {
//             block = (volatile s16 *)ptr - 1;
//             if (*block & HEAP_BLOCK_USED)
//             {
//                 *block &= ~HEAP_BLOCK_USED;
//                 if (heap != NULL && (heap->firstFree > (u16 *)block || heap->firstFree == NULL))
//                     heap->firstFree = (u16 *)block;
//             }
//         }
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
