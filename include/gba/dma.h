#ifndef GUARD_GBA_DMA_H
#define GUARD_GBA_DMA_H

#include "global.h"

// AGB SDK DMA macros: they go through a pointer to the channel's registers.

#define REG_ADDR_DMA0 0x040000B0

#define DmaSet(dmaNo, src, dest, control)                  \
{                                                          \
    vu32 *dmaRegs = (vu32 *)(REG_ADDR_DMA0 + (dmaNo) * 12); \
    dmaRegs[0] = (vu32)(src);                              \
    dmaRegs[1] = (vu32)(dest);                             \
    dmaRegs[2] = (vu32)(control);                          \
    dmaRegs[2];                                            \
}

#define DmaStop(dmaNo)                                                         \
{                                                                              \
    vu16 *dmaRegs = (vu16 *)(REG_ADDR_DMA0 + (dmaNo) * 12);                    \
    dmaRegs[5] &= ~((DMA_START_MASK | DMA_DREQ_ON | DMA_REPEAT) >> 16);        \
    dmaRegs[5] &= ~(DMA_ENABLE >> 16);                                         \
    dmaRegs[5];                                                                \
}

#endif // GUARD_GBA_DMA_H
