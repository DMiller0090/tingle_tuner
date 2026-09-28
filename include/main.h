#ifndef GUARD_MAIN_H
#define GUARD_MAIN_H

#include "global.h"
#include "heap.h"

// Main work area in IWRAM (0x03003080 in every version).
struct Main
{
    /*0x000*/ u32 intrMain[0x100];  // IWRAM copy of intr_main (INTR_VECTOR points here)
    /*0x400*/ u32 gameCode;         // from the packer's header (0x020000AC)
    /*0x404*/ u32 unk404;
    /*0x408*/ s32 random;           // UpdateRandom
    /*0x40C*/ u16 unk40C[4];        // cleared by InitSystem
    /*0x414*/ vu16 intrCheck:16;    // VBlankIntr sets bit 0; MainLoop waits for it
    /*0x416*/ u16 heldKeys;
    /*0x418*/ u16 newKeys;
    // Shadow display registers, in I/O order; VBlankIntr copies them to the hardware.
    // (agbcc pads nested structs to 4 bytes, so they can't be a struct of their own.)
    /*0x41A*/ u16 dispcnt;
    /*0x41C*/ u16 bgcnt[4];
    /*0x424*/ u16 mosaic;
    /*0x426*/ s16 bgOfs[4][2];      // [bg][0 = HOFS, 1 = VOFS]
    /*0x436*/ u16 win0h;
    /*0x438*/ u16 win1h;
    /*0x43A*/ u16 win0v;
    /*0x43C*/ u16 win1v;
    /*0x43E*/ u16 winin;
    /*0x440*/ u16 winout;
    /*0x442*/ u16 bldcnt;
    /*0x444*/ u16 bldalpha;
    /*0x446*/ u16 bldy;
    /*0x448*/ u8 filler448[4];
    /*0x44C*/ u8 dispCopied;        // VBlankIntr copied the shadows and OAM; MainLoop clears it
    /*0x44D*/ u8 frameCount;        // counted by VBlankIntr
    /*0x44E*/ u8 soundDmaStop;      // VBlankIntr calls SoundDmaStop
    /*0x44F*/ u8 filler44F;
    /*0x450*/ u8 unk450;
    /*0x451*/ u8 unk451;
    /*0x452*/ u8 mode;              // index into gMainModeFuncs; 2 when the link drops
    /*0x453*/ u8 modeState;
    /*0x454*/ u8 unk454;
    /*0x455*/ u8 hblankScroll;      // VBlankIntr runs the per-line BG scroll DMA
    /*0x456*/ u8 inDungeon;         // the GC's dungeon number is not 0
    /*0x457*/ u8 unk457;
    /*0x458*/ u8 unk458;
    /*0x459*/ u8 linked:1;          // GbaComMain
    /*0x45C*/ union { struct Heap heap; u8 raw[0x1800]; } heap;
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
void MainMode0(void);
void MainMode1(void);
void MainMode2(void);
void UpdateRandom(void);

#endif // GUARD_MAIN_H
