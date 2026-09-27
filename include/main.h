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
    /*0x40C*/ u16 unk40C[4];        // cleared by InitSystem
    /*0x414*/ u16 unk414;
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

// One OAM entry as two words (attr0/1, attr2/affine).
struct OamPair
{
    u32 lo;
    u32 hi;
};

extern const struct OamPair gOamHiddenEntry;  // y = 160: off screen
extern struct OamPair gOamBuffer[128];

void AgbMain(void);
void VBlankIntr(void);
void IntrDummy(void);
void ReadKeys(void);
void InitSystem(void);
void InitOamBuffer(void);
void MainLoop(void);
void SeedRandom(void);
void UpdateRandom(void);

#endif // GUARD_MAIN_H
