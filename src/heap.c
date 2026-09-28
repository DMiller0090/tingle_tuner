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
// Near-miss: same code, but block/total/avail/rest get r4/r2/r3/r1 where the original has
// r5/r3/r4/r2. The header `size | 1` is written at the store so loop.c hoists it after the
// 0xFFFE mask, as in the original. The rest of the diff looks like it comes from the rest store:
// the original keeps `total - need` in r1 apart from its `- 2` (r0), so rest (allocated first)
// can't take r1 and total then can't take r2; agbcc ties the two temporaries into r0.
// Temporaries/user vars for `total - need`, other operand orders and the align-split forms
// don't change it.
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
//                         *rest = (total - need - 2) & ~HEAP_BLOCK_USED;
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
