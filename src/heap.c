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
// Near-miss (112 masked diff lines): same code, but avail/block get r2/r4 where the original
// has r4/r5 (and the registers derived from them). The header `size | 1` is written at the
// store so loop.c hoists it after the 0xFFFE mask, as in the original. Keeping `total - need`
// in len (a global pseudo, so local-alloc can't tie it to the `- 2` result) moves rest to r2
// and total to r3 as in the original; avail still takes r2 (the original's avail must
// conflict with an r2 pseudo, maybe the `& 3` constant). Temporaries for `total - need`, the
// avail if/else forms and the align-split forms don't help.
//     void *HeapAlloc(struct Heap *heap, u32 size)
//     {
//         bool32 align4 = FALSE;
//         u32 need;
//         u16 *block;
//         u16 *firstFree;
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
//                         len = total - need;
//                         *rest = (len - 2) & ~HEAP_BLOCK_USED;
//                         if (firstFree > rest)
//                             heap->firstFree = rest;
//                     }
//                     *block = size | HEAP_BLOCK_USED;
//                     return block + 1;
//                 }
//             }
//         next:
//             block = next;
//         } while (*block != HEAP_END);
//         return NULL;
//     }
INCLUDE_ASM(HeapAlloc);

// The header is cleared through a u32 temporary: its zero-extending load keeps the loaded
// value as the AND's destination (a u16 `&=` ties the -2 constant's register instead).
void HeapFree(struct Heap *heap, void *ptr)
{
    u16 *block;

    if (ptr != NULL)
    {
        block = (u16 *)ptr - 1;
        if (*block & HEAP_BLOCK_USED)
        {
            u32 header = *block;

            header &= ~HEAP_BLOCK_USED;
            *block = header;
            if (heap != NULL && (heap->firstFree > block || heap->firstFree == NULL))
                heap->firstFree = block;
        }
    }
}

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
