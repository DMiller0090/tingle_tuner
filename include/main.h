#ifndef GUARD_MAIN_H
#define GUARD_MAIN_H

#include "global.h"

// Main work area in IWRAM (0x03003080 in every version).
struct Main
{
    /*0x000*/ u32 intrMain[0x100];  // IWRAM copy of intr_main (INTR_VECTOR points here)
    /*0x400*/ u32 gameCode;         // from the packer's header (0x020000AC)
    /*0x404*/ u8 filler404[4];
    /*0x408*/ s32 random;           // UpdateRandom
    /*0x40C*/ u8 filler40C[0x416 - 0x40C];
    /*0x416*/ u16 heldKeys;
    /*0x418*/ u16 newKeys;
    /*0x41A*/ u8 filler41A[0x44C - 0x41A];
    /*0x44C*/ u8 unk44C;
    /*0x44D*/ u8 filler44D[0x450 - 0x44D];
    /*0x450*/ u8 unk450;
    /*0x451*/ u8 unk451;
    /*0x452*/ u8 unk452;            // set to 2 when the link drops (GbaComMain)
    /*0x453*/ u8 unk453;
    /*0x454*/ u8 filler454[0x45C - 0x454];
    /*0x45C*/ u8 heap[0x1800];      // HeapInit(gMain.heap, sizeof(gMain.heap))
};

extern struct Main gMain;

void InitSystem(void);
void MainLoop(void);

#endif // GUARD_MAIN_H
