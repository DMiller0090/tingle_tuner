#ifndef GUARD_CRT0_H
#define GUARD_CRT0_H

#include "global.h"

typedef void (*IntrFunc)(void);

// crt0.s
extern const u32 intr_main[];

// Interrupt table used by intr_main: serial, VBlank, HBlank, VCount, DMA0-3, keypad, cart,
// plus unused slots (14 entries).
extern IntrFunc gIntrTable[14];
extern const IntrFunc gIntrTableInit[14];

// Linker symbols: the .data load image at the end of the program, copied to IWRAM.
extern const u8 __data_lma_start[];
extern const u8 __data_lma_end[];

#endif // GUARD_CRT0_H
