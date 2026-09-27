#ifndef GUARD_GAME_H
#define GUARD_GAME_H

#include "global.h"
#include "gba_com.h"

// Game state in IWRAM (0x030050E0 in every version), 0x240 bytes up to gGbaComDataStatus.
struct GameState
{
    /*0x000*/ struct GbaComFlags flags;  // last slot 9 packet from the GC
    /*0x00C*/ u8 filler00C[0x3A - 0x00C];
    // Switch record sent to the GC in slot 8 (daAgb_Switch, read by daAgb_c::SwitchOn).
    /*0x03A*/ s16 switchSound;
    /*0x03C*/ u8 switchNo;                // 0xFF: none
    /*0x03D*/ u8 switchStageNo;
    /*0x03E*/ u8 switchRoomNo;
    /*0x03F*/ u8 switchType;              // the GC switches on type - 1
    /*0x040*/ u8 switchNo2;
    /*0x041*/ u8 filler041;
    /*0x042*/ u8 unk042;
    /*0x043*/ u8 filler043[0x104 - 0x043];
    /*0x104*/ u8 unk104;                  // cleared while a menu or event runs on the GC
    /*0x105*/ u8 filler105[0x10E - 0x105];
    /*0x10E*/ u8 unk10E;
    /*0x10F*/ u8 filler10F[0x121 - 0x10F];
    /*0x121*/ u8 unk121;
    /*0x122*/ u8 filler122[0x138 - 0x122];
    /*0x138*/ u8 unk138;                  // sent as GbaFlg.unk2
    /*0x139*/ u8 filler139[0x1CC - 0x139];
    /*0x1CC*/ u8 unk1CC;
    /*0x1CD*/ u8 filler1CD[0x1FA - 0x1CD];
    /*0x1FA*/ u16 unk1FA;
    /*0x1FC*/ s16 rupees;                 // shown on screen; counts toward flags.rupees
    /*0x1FE*/ s16 rupeeDelta;             // flags.rupees - rupees
    /*0x200*/ u8 filler200[0x206 - 0x200];
    /*0x206*/ u16 unk206;
    /*0x208*/ u8 filler208[0x210 - 0x208];
    /*0x210*/ u16 timer;                  // flags.timer
    /*0x212*/ u8 filler212[0x21C - 0x212];
    /*0x21C*/ u8 unk21C;
    /*0x21D*/ u8 filler21D[0x221 - 0x21D];
    /*0x221*/ u8 fadeTimer;               // frames to the next mosaic/brightness step
    /*0x222*/ u8 filler222[0x224 - 0x222];
    /*0x224*/ u8 unk224;
    /*0x225*/ u8 shake;                   // bit 1 shakes the BGs by a line in VBlankIntr
    /*0x226*/ u8 filler226[0x22D - 0x226];
    /*0x22D*/ u8 scrollTableNext;
    /*0x22E*/ u8 scrollTable;             // which per-line scroll table the HBlank DMA reads
    /*0x22F*/ u8 filler22F[0x233 - 0x22F];
    /*0x233*/ u8 dungeonChests;           // flags.dungeonChests
    /*0x234*/ u8 filler234[2];
    /*0x236*/ u8 isFree;                  // flags.isFree
    /*0x237*/ u8 filler237;
    /*0x238*/ u8 unk238;                  // flags.unkB_7
    /*0x239*/ u8 filler239[2];
    /*0x23B*/ u8 flagsReceived;
    /*0x23C*/ u8 filler23C[4];
};

extern struct GameState gGameState;

// EWRAM below the program: per-line BG0HOFS tables that VBlankIntr's HBlank DMA reads
// (gGameState.scrollTable picks one).
extern u16 gBgScrollTables[][160];

// Graphics in .rodata (LZ77-compressed except the palettes).
extern const u8 gMode1BgTiles[];
extern const u16 gMode1BgPalette[0x100];
extern const u8 gMode1BgMap[];
extern const u8 gMode1SeaMap[];
extern const u8 gMode2BgMap[];
extern const u8 gMode1ObjTiles[];
extern const u16 gMode1ObjPalette[0x100];

void Mode1VBlank(void);
void Func_0201A570(void);
// The definition narrows id to u16, but callers pass s16 values sign-extended.
void GamePlaySound(s32 id, u8 arg);
void Func_02020644(void);
void Func_0201A224(void);
void Func_0201CFE8(void);

#endif // GUARD_GAME_H
