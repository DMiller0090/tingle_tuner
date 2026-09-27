#ifndef GUARD_GAME_H
#define GUARD_GAME_H

#include "global.h"
#include "gba_com.h"

// One map marker, as slot 3 sends them (8 bytes each after a count byte and one more).
struct MapMarker
{
    /*0x0*/ s16 x;       // 8.8? compared by the high byte
    /*0x2*/ s16 y;
    /*0x4*/ u8 unk4;
    /*0x5*/ u8 type:6;   // 1 and 3 are looked up by Mode1RecvMarkers
            u8 unk5_6:2;
    /*0x6*/ u8 unk6;
    /*0x7*/ u8 unk7;
};

// A sprite drawn over the map image (GameState.icons); type picks its update function and
// sprite (1: follows the player marker, 2: cursor, 3: blinking).
struct MapIcon
{
    /*0x0*/ s16 x;      // 12.4 map pixels
    /*0x2*/ s16 y;
    /*0x4*/ u8 type;    // 0: none
    /*0x5*/ u8 frame;
    /*0x6*/ u8 timer;
    /*0x7*/ u8 unk7;
};

// Game state in IWRAM (0x030050E0 in every version), 0x240 bytes up to gGbaComDataStatus.
// The struct body is generated from this list by tools/genstruct.py.
// genstruct GameState 0x240
// 0x000 struct GbaComFlags flags size=12  // last slot 9 packet from the GC
// 0x00C struct MapImage* mapImage  // the slot 2 map image (Func_0201A69C)
// 0x010 u32 unk010
// 0x014 s16 mapX  // top left of the map view in the map image (pixels)
// 0x016 s16 mapY
// 0x018 u8 unk018  // cleared by Func_0201A224
// 0x022 u16 unk022  // set to 240 when a target is placed (Func_0201AD90)
// 0x024 u16 targetScale  // pulse of the target compass sprite (Func_0201A944)
// 0x019 u8 unk019  // cleared when unk222 turns on (R)
// 0x026 u16 unk026
// 0x028 u16 seaAnimTimer[4] size=8  // per quadrant of the view (Func_0201A224)
// 0x030 u16 unk030
// 0x032 s16 unk032  // title sprite x
// 0x034 s16 unk034  // title sprite y
// 0x03A u16 switchSound  // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
// 0x03C u8 switchNo  // 0xFF: none
// 0x03D u8 switchStageNo
// 0x03E u8 switchRoomNo
// 0x03F u8 switchType  // the GC switches on type - 1
// 0x040 u8 switchNo2
// 0x042 u8 unk042
// 0x044 struct MapIcon icons[4] size=0x20  // drawn over the map by Func_0201AF48
// 0x064 struct MapMarker markers[16] size=0x80  // from slot 3 (Mode1RecvMarkers)
// 0x0E4 u8 markerCount
// 0x0E5 u8 unk0E5
// 0x0E8 u8 seaTileSet[4]  // gSeaSquareTileSets of each quadrant's square
// 0x0EC u8 seaAnimFrame[4]  // index into gSeaAnimFrames
// 0x0F0 u8 seaSquare[4]  // sea chart square of each quadrant
// 0x0F4 u8 unk0F4[8] size=8  // slot 11 (MainMode1 copies it here); one bit per sea chart square (7x7)
// 0x0FC u8 unk0FC
// 0x0FD u8 unk0FD
// 0x101 u8 targetBlinkOn
// 0x102 u8 targetDir  // direction (0-7) last announced by Func_0201A944; 0xFF: none
// 0x103 u8 targetBlinkTimer
// 0x104 u8 unk104  // cleared while a menu or event runs on the GC
// 0x105 u8 unk105
// 0x106 u8 unk106
// 0x107 u8 targetNear  // the near sound has played
// 0x108 u8 unk108
// 0x10A u8 unk10A
// 0x10C u8 unk10C
// 0x10E u8 unk10E
// 0x10F u8 unk10F
// 0x110 u8 unk110
// 0x113 u8 unk113
// 0x114 u8 unk114
// 0x115 u8 unk115
// 0x116 u8 unk116
// 0x117 u8 unk117
// 0x118 u8 unk118
// 0x119 u8 unk119  // index into markers
// 0x11A u8 unk11A
// 0x11B u8 unk11B
// 0x11C u8 unk11C
// 0x11D u8 unk11D
// 0x11E u8 unk11E
// 0x11F u8 mapScrolling  // L pressed to scroll the map (Func_0201A224)
// 0x120 u8 unk120
// 0x121 u8 unk121
// 0x122 u8 unk122
// 0x123 u8 unk123
// 0x124 u8 unk124
// 0x12B u8 unk12B
// 0x133 u8 unk133  // slot 3 byte 1
// 0x136 u8 unk136
// 0x138 bits u8 unk138_0:2 unk138_2:1 unk138_3:1 unk138_4:1 unk138_5:1 unk138_6:2  // sent as GbaFlg.unk2
// 0x13C u8 unk13C
// 0x13E u16 unk13E
// 0x140 s16 unk140
// 0x142 s16 unk142
// 0x14A u8 unk14A
// 0x14B u8 unk14B
// 0x156 u8 unk156
// 0x157 u8 unk157
// 0x15A u8 unk15A
// 0x15C u8 effect[4] size=4  // slot 7: item use result
// 0x1C8 u16 unk1C8
// 0x1CA u16 unk1CA
// 0x1CC u8 unk1CC
// 0x1E0 u8 unk1E0
// 0x1DE u8 unk1DE
// 0x1E1 u8 unk1E1  // chest bits that changed
// 0x1E3 u8 unk1E3  // chests opened
// 0x1E4 u8 unk1E4
// 0x1E5 bits u8 unk1E5_0:1 unk1E5_1:1 unk1E5_2:1 unk1E5_3:1 unk1E5_4:1 unk1E5_5:3
// 0x1EE u8 unk1EE
// 0x1F8 s16 unk1F8
// 0x1FB u8 unk1FB
// 0x1FA u8 unk1FA  // MainLoop tests 0x1FA-0x1FB as one u16
// 0x1FC s16 rupees  // shown on screen; counts toward flags.rupees
// 0x1FE s16 rupeeDelta  // flags.rupees - rupees
// 0x206 u16 unk206
// 0x20A u16 waveAmplitude  // intro wave (Mode1State1), counts down to 0
// 0x20E u16 wavePhase  // 8.8; the high byte indexes gSinTable
// 0x210 u16 timer  // flags.timer
// 0x212 u8 playerName[8]  // slot 10
// 0x21C u8 unk21C
// 0x21D u8 unk21D
// 0x21E u8 unk21E
// 0x21F u8 unk21F
// 0x220 u8 unk220
// 0x221 u8 fadeTimer  // frames to the next mosaic/brightness step
// 0x222 u8 unk222
// 0x223 u8 unk223
// 0x224 u8 unk224
// 0x225 u8 shake  // bit 1 shakes the BGs by a line in VBlankIntr
// 0x226 u8 unk226
// 0x227 u8 unk227
// 0x228 u8 unk228
// 0x22D u8 scrollTableNext
// 0x22E u8 scrollTable  // which per-line scroll table the HBlank DMA reads
// 0x230 u8 life  // flags.life as last seen
// 0x231 u8 lowLifeWarned  // the low-life sound has played
// 0x232 u8 chimed  // the clock sound for this hour has played
// 0x233 u8 dungeonChests  // flags.dungeonChests
// 0x235 u8 unk235  // gMain.unk457 at the last fade out
// 0x236 u8 isFree  // flags.isFree
// 0x237 u8 unk237
// 0x238 u8 unk238  // flags.unkB_7
// 0x239 u8 unk239  // flags.unkB_7 changed
// 0x23B u8 flagsReceived
// 0x23C u8 unk23C
// end genstruct
struct GameState
{
    /*0x000*/ struct GbaComFlags flags;       // last slot 9 packet from the GC
    /*0x00C*/ struct MapImage* mapImage;      // the slot 2 map image (Func_0201A69C)
    /*0x010*/ u32 unk010;
    /*0x014*/ s16 mapX;                       // top left of the map view in the map image (pixels)
    /*0x016*/ s16 mapY;
    /*0x018*/ u8 unk018;                      // cleared by Func_0201A224
    /*0x019*/ u8 unk019;                      // cleared when unk222 turns on (R)
    /*0x01A*/ u8 filler01A[0x022 - 0x01A];
    /*0x022*/ u16 unk022;                     // set to 240 when a target is placed (Func_0201AD90)
    /*0x024*/ u16 targetScale;                // pulse of the target compass sprite (Func_0201A944)
    /*0x026*/ u16 unk026;
    /*0x028*/ u16 seaAnimTimer[4];            // per quadrant of the view (Func_0201A224)
    /*0x030*/ u16 unk030;
    /*0x032*/ s16 unk032;                     // title sprite x
    /*0x034*/ s16 unk034;                     // title sprite y
    /*0x036*/ u8 filler036[0x03A - 0x036];
    /*0x03A*/ u16 switchSound;                // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
    /*0x03C*/ u8 switchNo;                    // 0xFF: none
    /*0x03D*/ u8 switchStageNo;
    /*0x03E*/ u8 switchRoomNo;
    /*0x03F*/ u8 switchType;                  // the GC switches on type - 1
    /*0x040*/ u8 switchNo2;
    /*0x041*/ u8 filler041[0x042 - 0x041];
    /*0x042*/ u8 unk042;
    /*0x043*/ u8 filler043[0x044 - 0x043];
    /*0x044*/ struct MapIcon icons[4];        // drawn over the map by Func_0201AF48
    /*0x064*/ struct MapMarker markers[16];   // from slot 3 (Mode1RecvMarkers)
    /*0x0E4*/ u8 markerCount;
    /*0x0E5*/ u8 unk0E5;
    /*0x0E6*/ u8 filler0E6[0x0E8 - 0x0E6];
    /*0x0E8*/ u8 seaTileSet[4];               // gSeaSquareTileSets of each quadrant's square
    /*0x0EC*/ u8 seaAnimFrame[4];             // index into gSeaAnimFrames
    /*0x0F0*/ u8 seaSquare[4];                // sea chart square of each quadrant
    /*0x0F4*/ u8 unk0F4[8];                   // slot 11 (MainMode1 copies it here); one bit per sea chart square (7x7)
    /*0x0FC*/ u8 unk0FC;
    /*0x0FD*/ u8 unk0FD;
    /*0x0FE*/ u8 filler0FE[0x101 - 0x0FE];
    /*0x101*/ u8 targetBlinkOn;
    /*0x102*/ u8 targetDir;                   // direction (0-7) last announced by Func_0201A944; 0xFF: none
    /*0x103*/ u8 targetBlinkTimer;
    /*0x104*/ u8 unk104;                      // cleared while a menu or event runs on the GC
    /*0x105*/ u8 unk105;
    /*0x106*/ u8 unk106;
    /*0x107*/ u8 targetNear;                  // the near sound has played
    /*0x108*/ u8 unk108;
    /*0x109*/ u8 filler109[0x10A - 0x109];
    /*0x10A*/ u8 unk10A;
    /*0x10B*/ u8 filler10B[0x10C - 0x10B];
    /*0x10C*/ u8 unk10C;
    /*0x10D*/ u8 filler10D[0x10E - 0x10D];
    /*0x10E*/ u8 unk10E;
    /*0x10F*/ u8 unk10F;
    /*0x110*/ u8 unk110;
    /*0x111*/ u8 filler111[0x113 - 0x111];
    /*0x113*/ u8 unk113;
    /*0x114*/ u8 unk114;
    /*0x115*/ u8 unk115;
    /*0x116*/ u8 unk116;
    /*0x117*/ u8 unk117;
    /*0x118*/ u8 unk118;
    /*0x119*/ u8 unk119;                      // index into markers
    /*0x11A*/ u8 unk11A;
    /*0x11B*/ u8 unk11B;
    /*0x11C*/ u8 unk11C;
    /*0x11D*/ u8 unk11D;
    /*0x11E*/ u8 unk11E;
    /*0x11F*/ u8 mapScrolling;                // L pressed to scroll the map (Func_0201A224)
    /*0x120*/ u8 unk120;
    /*0x121*/ u8 unk121;
    /*0x122*/ u8 unk122;
    /*0x123*/ u8 unk123;
    /*0x124*/ u8 unk124;
    /*0x125*/ u8 filler125[0x12B - 0x125];
    /*0x12B*/ u8 unk12B;
    /*0x12C*/ u8 filler12C[0x133 - 0x12C];
    /*0x133*/ u8 unk133;                      // slot 3 byte 1
    /*0x134*/ u8 filler134[0x136 - 0x134];
    /*0x136*/ u8 unk136;
    /*0x137*/ u8 filler137[0x138 - 0x137];
    /*0x138*/ u8 unk138_0:2;                  // sent as GbaFlg.unk2
              u8 unk138_2:1;
              u8 unk138_3:1;
              u8 unk138_4:1;
              u8 unk138_5:1;
              u8 unk138_6:2;
    /*0x139*/ u8 filler139[0x13C - 0x139];
    /*0x13C*/ u8 unk13C;
    /*0x13D*/ u8 filler13D[0x13E - 0x13D];
    /*0x13E*/ u16 unk13E;
    /*0x140*/ s16 unk140;
    /*0x142*/ s16 unk142;
    /*0x144*/ u8 filler144[0x14A - 0x144];
    /*0x14A*/ u8 unk14A;
    /*0x14B*/ u8 unk14B;
    /*0x14C*/ u8 filler14C[0x156 - 0x14C];
    /*0x156*/ u8 unk156;
    /*0x157*/ u8 unk157;
    /*0x158*/ u8 filler158[0x15A - 0x158];
    /*0x15A*/ u8 unk15A;
    /*0x15B*/ u8 filler15B[0x15C - 0x15B];
    /*0x15C*/ u8 effect[4];                   // slot 7: item use result
    /*0x160*/ u8 filler160[0x1C8 - 0x160];
    /*0x1C8*/ u16 unk1C8;
    /*0x1CA*/ u16 unk1CA;
    /*0x1CC*/ u8 unk1CC;
    /*0x1CD*/ u8 filler1CD[0x1DE - 0x1CD];
    /*0x1DE*/ u8 unk1DE;
    /*0x1DF*/ u8 filler1DF[0x1E0 - 0x1DF];
    /*0x1E0*/ u8 unk1E0;
    /*0x1E1*/ u8 unk1E1;                      // chest bits that changed
    /*0x1E2*/ u8 filler1E2[0x1E3 - 0x1E2];
    /*0x1E3*/ u8 unk1E3;                      // chests opened
    /*0x1E4*/ u8 unk1E4;
    /*0x1E5*/ u8 unk1E5_0:1;
              u8 unk1E5_1:1;
              u8 unk1E5_2:1;
              u8 unk1E5_3:1;
              u8 unk1E5_4:1;
              u8 unk1E5_5:3;
    /*0x1E6*/ u8 filler1E6[0x1EE - 0x1E6];
    /*0x1EE*/ u8 unk1EE;
    /*0x1EF*/ u8 filler1EF[0x1F8 - 0x1EF];
    /*0x1F8*/ s16 unk1F8;
    /*0x1FA*/ u8 unk1FA;                      // MainLoop tests 0x1FA-0x1FB as one u16
    /*0x1FB*/ u8 unk1FB;
    /*0x1FC*/ s16 rupees;                     // shown on screen; counts toward flags.rupees
    /*0x1FE*/ s16 rupeeDelta;                 // flags.rupees - rupees
    /*0x200*/ u8 filler200[0x206 - 0x200];
    /*0x206*/ u16 unk206;
    /*0x208*/ u8 filler208[0x20A - 0x208];
    /*0x20A*/ u16 waveAmplitude;              // intro wave (Mode1State1), counts down to 0
    /*0x20C*/ u8 filler20C[0x20E - 0x20C];
    /*0x20E*/ u16 wavePhase;                  // 8.8; the high byte indexes gSinTable
    /*0x210*/ u16 timer;                      // flags.timer
    /*0x212*/ u8 playerName[8];               // slot 10
    /*0x21A*/ u8 filler21A[0x21C - 0x21A];
    /*0x21C*/ u8 unk21C;
    /*0x21D*/ u8 unk21D;
    /*0x21E*/ u8 unk21E;
    /*0x21F*/ u8 unk21F;
    /*0x220*/ u8 unk220;
    /*0x221*/ u8 fadeTimer;                   // frames to the next mosaic/brightness step
    /*0x222*/ u8 unk222;
    /*0x223*/ u8 unk223;
    /*0x224*/ u8 unk224;
    /*0x225*/ u8 shake;                       // bit 1 shakes the BGs by a line in VBlankIntr
    /*0x226*/ u8 unk226;
    /*0x227*/ u8 unk227;
    /*0x228*/ u8 unk228;
    /*0x229*/ u8 filler229[0x22D - 0x229];
    /*0x22D*/ u8 scrollTableNext;
    /*0x22E*/ u8 scrollTable;                 // which per-line scroll table the HBlank DMA reads
    /*0x22F*/ u8 filler22F[0x230 - 0x22F];
    /*0x230*/ u8 life;                        // flags.life as last seen
    /*0x231*/ u8 lowLifeWarned;               // the low-life sound has played
    /*0x232*/ u8 chimed;                      // the clock sound for this hour has played
    /*0x233*/ u8 dungeonChests;               // flags.dungeonChests
    /*0x234*/ u8 filler234[0x235 - 0x234];
    /*0x235*/ u8 unk235;                      // gMain.unk457 at the last fade out
    /*0x236*/ u8 isFree;                      // flags.isFree
    /*0x237*/ u8 unk237;
    /*0x238*/ u8 unk238;                      // flags.unkB_7
    /*0x239*/ u8 unk239;                      // flags.unkB_7 changed
    /*0x23A*/ u8 filler23A[0x23B - 0x23A];
    /*0x23B*/ u8 flagsReceived;
    /*0x23C*/ u8 unk23C;
    /*0x23D*/ u8 filler23D[0x240 - 0x23D];
};

extern struct GameState gGameState;

// EWRAM below the program: per-line BG0HOFS tables that VBlankIntr's HBlank DMA reads
// (gGameState.scrollTable picks one).
extern u16 gBgScrollTables[][160];
// The same tables by constant address, as the game code addresses them (see GBACOM_BUF_3).
#define BG_SCROLL_TABLE_0 ((u16 *)(PROG_BASE - 0x4C0))
#define BG_SCROLL_TABLE_1 ((u16 *)(PROG_BASE - 0x380))

// Graphics in .rodata (LZ77-compressed except the palettes).
extern const u8 gMode1BgTiles[];
extern const u16 gMode1BgPalette[0x100];
extern const u8 gMode1BgMap[];
extern const u8 gMode1SeaMap[];
extern const u8 gMode2BgMap[];
extern const u8 gMode1ObjTiles[];
extern const u16 gMode1ObjPalette[0x100];
extern const u8 gSprite_0202C438[];
extern const u8 gSprite_0202C938[];
extern const u8 *const gSpriteTable[];

void Mode1VBlank(void);
void Func_0201A570(void);
// The definition narrows id to u16, but callers see an s16 parameter (they sign-extend).
void GamePlaySound(s16 id, u8 arg);
bool32 Func_0201A6D4(void);
void Func_0201A540(void);
bool32 StepToward(s16 *pos, s16 target, s16 speed);
void Func_020209E0(void);
// The definition takes u16 x and y; callers see s16 (they sign-extend).
void DrawSprite(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u16 a6, u16 a7, u16 a8,
                u8 a9, u8 a10);
// The definition takes u16 x and y; callers see s16 (they sign-extend).
void DrawSpriteAt(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u8 a6);
bool32 Func_0201A69C(void);  // returns nothing (falls off the end)
void Func_0201E190(void);
void Func_02020BEC(void);
bool32 Func_0201D8AC(void);
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude);

// 16 objects in IWRAM (0x03005650).
struct Obj
{
    /*0x00*/ u32 unk00;
    /*0x04*/ u32 unk04;
    /*0x08*/ u32 unk08;
    /*0x0C*/ u16 unk0C[9];
    /*0x1E*/ u8 unk1E;
    /*0x1F*/ u8 active;
    /*0x20*/ u8 unk20;
};

extern struct Obj gObjs[16];

// 0x03005640, just before gObjs: the title screen's blend ramp.
struct Mode0Work
{
    /*0x0*/ u8 blend;      // BLDALPHA EVA (EVB = 16 - blend)
    /*0x1*/ u8 timer;      // frames to the next step
    /*0x2*/ u8 fadingOut;  // 0: blend counts up to 16, 1: down to 1
    /*0x3*/ u8 linkTimeout;  // frames left to receive the message (slot 0)
    /*0x4*/ u8 filler4[0xC];
};

extern struct Mode0Work gMode0Work;

// One entry of the slot 14 data: an island or other named place on the sea chart.
struct SeaChartEntry
{
    /*0x0*/ u8 x;       // pixels within its square?
    /*0x1*/ u8 y;
    /*0x2*/ u8 unk2;
    /*0x3*/ u8 square;  // sea chart square (row * 7 + column); its name sound is 0x14E + square
};

// Slot 14 data (IWRAM 0x03002FB0), received once at boot by Mode0State3: one entry per
// sea chart square.
extern struct SeaChartEntry gSlot14Data[49];

#ifndef VERSION_GZLJ01
void DrawString(const u8 *str, u8 a1, u8 a2, u8 a3);
#endif

void ObjReset(struct Obj *obj);
void ObjResetActive(void);  // ObjReset on every active entry
void Func_0201A224(void);
void Func_0201CFE8(void);
void Func_0201B7D4(void);
void Func_0201DA50(void);
void Func_0201E384(u8 arg);
void Func_0201B0F4(void);
void Func_0201B8C8(void);
u8 FindMarker(u8 type);
void Func_0201DEAC(void);
void Func_02020C10(void);
u8 Func_0201B3A4(void);
void Func_0201C484(u8 id);
void Func_0201C568(void);
void Func_0201DD20(u8 a, u8 b);
void Func_0201E5E0(void);
void Func_0201EBAC(void);
void Func_0201EBD4(void);
void Func_0201EDC4(void);
void Func_0201F240(void);
void Func_0201F274(void);
void Func_0201F2C4(void);
void Func_0201F420(void);
void Func_02020150(void);
void Func_02020298(void);
extern const u8 gSprite_0202CA08[];
extern const u8 gSprite_0202CA40[];

// ---- game.c part A (GZLJ01 02018884-0201A224) ----

// ---- game.c part B (GZLJ01 0201A224-0201AFCC) ----

// The map image the GC sends in slot 2, in the message buffer (gGbaComMessageBuf). Its two
// data pointers arrive as offsets from the header; Func_0201A69C turns them into addresses.
struct MapImage
{
    /*0x00*/ u16 width;       // pixels
    /*0x02*/ u16 height;
    /*0x04*/ u8 filler4[2];
    /*0x06*/ s8 floor;        // dungeon floor (< 0: basement); 0: none
    /*0x07*/ u8 filler7;
    /*0x08*/ u8 *tiles;
    /*0x0C*/ s32 tilesSize;   // bytes
    /*0x10*/ u8 filler10[0x20];
    /*0x30*/ u8 mapWidth;     // tiles per map row
    /*0x31*/ u8 filler31[3];
    /*0x34*/ u16 *map;
};

#define GBACOM_BUF_MESSAGE ((u8 *)(PROG_BASE - 0xA800))

u16 Sqrt(u32 x);
void Func_02019F5C(void);
void Func_0201A0A8(void);
void Func_02019350(struct GameState *state);
void Func_020192D8(s16 y, s16 x, u16 *map);
void Func_0201AC94(struct MapIcon *icon);
void Func_0201ACD0(void);
void Func_0201ACF8(struct MapIcon *icon);
void Func_0201AD24(void);
void Func_0201AD4C(void);
void Func_0201AD90(struct MapIcon *icon);
void Func_0201AF48(void);
u32 Func_0201A8AC(u8 arg);
extern void (*const gMapIconFuncs[])(struct MapIcon *icon);
extern const u8 *const gMapIconSprites[];
extern const s16 gSinTable[];
extern const u8 gTargetBlinkPeriods[5];
extern const u8 gSprite_0202C3F8[];
extern const u8 gSprite_0202BDC8[];
extern const u16 gSeaQuadrantOffsets[4][2];
extern const u8 gSeaSquareTileSets[49];
extern const u8 gSeaAnimFrames[5];
extern const u8 gSeaAnimPeriods[3];
extern const u8 gSeaAnimTiles[0x2000];
void Func_0201B2C0(void);
void Func_02019CAC(void);
void Func_02019510(struct GameState *state, u8 frameCount);
u16 ArcTan2(s32 x, s32 y);

// ---- game.c part C (GZLJ01 0201AFCC-0201C660) ----

// ---- game.c part D (GZLJ01 0201C660-0201DEAC) ----

// ---- game.c part E (GZLJ01 0201DEAC-0201F308) ----

// ---- game.c part F (GZLJ01 0201F308-02020E28) ----

#endif // GUARD_GAME_H
