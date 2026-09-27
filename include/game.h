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

// Game state in IWRAM (0x030050E0 in every version), 0x240 bytes up to gGbaComDataStatus.
// The struct body is generated from this list by tools/genstruct.py.
// genstruct GameState 0x240
// 0x000 struct GbaComFlags flags size=12  // last slot 9 packet from the GC
// 0x010 u32 unk010
// 0x019 u8 unk019  // cleared when unk222 turns on (R)
// 0x026 u16 unk026
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
// 0x054 u16 unk054
// 0x056 u16 unk056
// 0x058 u8 unk058
// 0x064 struct MapMarker markers[16] size=0x80  // from slot 3 (Mode1RecvMarkers)
// 0x0E4 u8 markerCount
// 0x0E5 u8 unk0E5
// 0x0F4 u8 unk0F4[8] size=8  // slot 11 (MainMode1 copies it here)
// 0x0FC u8 unk0FC
// 0x0FD u8 unk0FD
// 0x102 u8 unk102
// 0x104 u8 unk104  // cleared while a menu or event runs on the GC
// 0x106 u8 unk106
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
// 0x120 u8 unk120
// 0x121 u8 unk121
// 0x122 u8 unk122
// 0x123 u8 unk123
// 0x124 u8 unk124
// 0x12B u8 unk12B
// 0x12E u8 unk12E
// 0x12F u8 unk12F
// 0x132 u8 unk132
// 0x133 u8 unk133  // slot 3 byte 1
// 0x136 u8 unk136
// 0x138 bits u8 unk138_0:2 unk138_2:1 unk138_3:1 unk138_4:1 unk138_5:1 unk138_6:2  // sent as GbaFlg.unk2
// 0x13C u8 unk13C
// 0x13E u16 unk13E
// 0x140 s16 unk140
// 0x142 s16 unk142
// 0x144 const u8 * unk144  // US: a message in the message data (MSG_DATA)
// 0x14A u8 unk14A
// 0x14B u8 unk14B
// 0x14E u8 unk14E
// 0x14F u8 unk14F
// 0x150 u8 unk150
// 0x151 u8 unk151
// 0x156 u8 unk156
// 0x157 u8 unk157
// 0x15A u8 unk15A
// 0x15C u8 effect[4] size=4  // slot 7: item use result
// 0x18E u8 unk18E
// 0x1C8 u16 unk1C8
// 0x1CA u16 unk1CA
// 0x1CC u8 unk1CC
// 0x1E0 u8 unk1E0
// 0x1DE u8 unk1DE
// 0x1E1 u8 unk1E1  // chest bits that changed
// 0x1E2 u8 unk1E2
// 0x1E3 u8 unk1E3  // chests opened
// 0x1E4 u8 unk1E4
// 0x1E5 bits u8 unk1E5_0:1 unk1E5_1:1 unk1E5_2:1 unk1E5_3:1 unk1E5_4:1 unk1E5_5:3
// 0x1E8 u16 unk1E8
// 0x1EA s16 unk1EA
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
    /*0x00C*/ u8 filler00C[0x010 - 0x00C];
    /*0x010*/ u32 unk010;
    /*0x014*/ u8 filler014[0x019 - 0x014];
    /*0x019*/ u8 unk019;                      // cleared when unk222 turns on (R)
    /*0x01A*/ u8 filler01A[0x026 - 0x01A];
    /*0x026*/ u16 unk026;
    /*0x028*/ u8 filler028[0x030 - 0x028];
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
    /*0x043*/ u8 filler043[0x054 - 0x043];
    /*0x054*/ u16 unk054;
    /*0x056*/ u16 unk056;
    /*0x058*/ u8 unk058;
    /*0x059*/ u8 filler059[0x064 - 0x059];
    /*0x064*/ struct MapMarker markers[16];   // from slot 3 (Mode1RecvMarkers)
    /*0x0E4*/ u8 markerCount;
    /*0x0E5*/ u8 unk0E5;
    /*0x0E6*/ u8 filler0E6[0x0F4 - 0x0E6];
    /*0x0F4*/ u8 unk0F4[8];                   // slot 11 (MainMode1 copies it here)
    /*0x0FC*/ u8 unk0FC;
    /*0x0FD*/ u8 unk0FD;
    /*0x0FE*/ u8 filler0FE[0x102 - 0x0FE];
    /*0x102*/ u8 unk102;
    /*0x103*/ u8 filler103[0x104 - 0x103];
    /*0x104*/ u8 unk104;                      // cleared while a menu or event runs on the GC
    /*0x105*/ u8 filler105[0x106 - 0x105];
    /*0x106*/ u8 unk106;
    /*0x107*/ u8 filler107[0x108 - 0x107];
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
    /*0x11F*/ u8 filler11F[0x120 - 0x11F];
    /*0x120*/ u8 unk120;
    /*0x121*/ u8 unk121;
    /*0x122*/ u8 unk122;
    /*0x123*/ u8 unk123;
    /*0x124*/ u8 unk124;
    /*0x125*/ u8 filler125[0x12B - 0x125];
    /*0x12B*/ u8 unk12B;
    /*0x12C*/ u8 filler12C[0x12E - 0x12C];
    /*0x12E*/ u8 unk12E;
    /*0x12F*/ u8 unk12F;
    /*0x130*/ u8 filler130[0x132 - 0x130];
    /*0x132*/ u8 unk132;
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
    /*0x144*/ const u8 * unk144;              // US: a message in the message data (MSG_DATA)
    /*0x148*/ u8 filler148[0x14A - 0x148];
    /*0x14A*/ u8 unk14A;
    /*0x14B*/ u8 unk14B;
    /*0x14C*/ u8 filler14C[0x14E - 0x14C];
    /*0x14E*/ u8 unk14E;
    /*0x14F*/ u8 unk14F;
    /*0x150*/ u8 unk150;
    /*0x151*/ u8 unk151;
    /*0x152*/ u8 filler152[0x156 - 0x152];
    /*0x156*/ u8 unk156;
    /*0x157*/ u8 unk157;
    /*0x158*/ u8 filler158[0x15A - 0x158];
    /*0x15A*/ u8 unk15A;
    /*0x15B*/ u8 filler15B[0x15C - 0x15B];
    /*0x15C*/ u8 effect[4];                   // slot 7: item use result
    /*0x160*/ u8 filler160[0x18E - 0x160];
    /*0x18E*/ u8 unk18E;
    /*0x18F*/ u8 filler18F[0x1C8 - 0x18F];
    /*0x1C8*/ u16 unk1C8;
    /*0x1CA*/ u16 unk1CA;
    /*0x1CC*/ u8 unk1CC;
    /*0x1CD*/ u8 filler1CD[0x1DE - 0x1CD];
    /*0x1DE*/ u8 unk1DE;
    /*0x1DF*/ u8 filler1DF[0x1E0 - 0x1DF];
    /*0x1E0*/ u8 unk1E0;
    /*0x1E1*/ u8 unk1E1;                      // chest bits that changed
    /*0x1E2*/ u8 unk1E2;
    /*0x1E3*/ u8 unk1E3;                      // chests opened
    /*0x1E4*/ u8 unk1E4;
    /*0x1E5*/ u8 unk1E5_0:1;
              u8 unk1E5_1:1;
              u8 unk1E5_2:1;
              u8 unk1E5_3:1;
              u8 unk1E5_4:1;
              u8 unk1E5_5:3;
    /*0x1E6*/ u8 filler1E6[0x1E8 - 0x1E6];
    /*0x1E8*/ u16 unk1E8;
    /*0x1EA*/ s16 unk1EA;
    /*0x1EC*/ u8 filler1EC[0x1EE - 0x1EC];
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
void Func_0201A6D4(void);
void Func_0201A540(void);
bool32 StepToward(s16 *pos, s16 target, s16 speed);
void Func_020209E0(void);
void DrawSprite(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u16 a6, s16 a7, s16 a8,
                u8 a9, u8 a10);
// The definition takes u16 x and y; callers see s16 (they sign-extend).
void DrawSpriteAt(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u8 a6);
void Func_0201A69C(void);
void Func_0201E190(void);
void Func_02020BEC(void);
bool32 Func_0201D8AC(void);
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude);

// 16 objects in IWRAM (0x03005650).
struct Obj
{
    /*0x00*/ s32 x;        // 8.8
    /*0x04*/ s32 y;        // 8.8
    /*0x08*/ s32 z;        // 8.8 height, <= 0; drawn at y + z
    /*0x0C*/ s16 vx;
    /*0x0E*/ s16 vy;
    /*0x10*/ s16 vz;
    /*0x12*/ u16 unk12;
    /*0x14*/ u16 unk14;
    /*0x16*/ u16 unk16;
    /*0x18*/ s16 unk18;
    /*0x1A*/ s16 unk1A;
    /*0x1C*/ u16 unk1C;
    /*0x1E*/ u8 unk1E;
    /*0x1F*/ u8 active;    // 0: free, else 1 + index into the type's state table
    /*0x20*/ u8 type;      // index into gObjFuncs
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

// Slot 14 data (IWRAM 0x03002FB0), received once at boot by Mode0State3.
extern u8 gSlot14Data[0xC4];

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
void UpdateItemEffect(void);
extern const u8 gSprite_0202CA08[];
extern const u8 gSprite_0202CA40[];

// ---- game.c part A (GZLJ01 02018884-0201A224) ----

// ---- game.c part B (GZLJ01 0201A224-0201AFCC) ----

// ---- game.c part C (GZLJ01 0201AFCC-0201C660) ----

// ---- game.c part D (GZLJ01 0201C660-0201DEAC) ----

// ---- game.c part E (GZLJ01 0201DEAC-0201F308) ----

// ---- game.c part F (GZLJ01 0201F308-02020E28) ----

extern const s16 gSinTable[];
extern void (*const gMode2StateFuncs[])(void);
// Per object type: its function, called every frame while the object is active.
extern void (*const gObjFuncs[])(struct Obj *obj);

void ObjDecelX(struct Obj *obj, s16 decel);
void ObjDecelY(struct Obj *obj, s16 decel);
void ObjFall(struct Obj *obj, s16 gravity);
void ObjUpdateAll(void);
s32 ObjFindFree(void);
bool32 ObjSpawn(u8 type);
extern const u8 gSprite_0202BD78[];
extern const u8 gSprite_0202C848[];
void Mode2State0(void);
void Mode2State1(void);
// The message data (slot 0, msg_LZ.bin), unpacked to the start of EWRAM by Mode0State1.
#define MSG_DATA ((const u8 *)0x02000000)
// Object type 0 (ObjBurst): five pieces, per piece (unk1C) an offset and a velocity.
extern const s16 gObjBurstOfsX[5];
extern const s16 gObjBurstOfsY[5];
extern const s16 gObjBurstVelX[5];
extern const s16 gObjBurstVelY[5];
extern void (*const gObjBurstStateFuncs[])(struct Obj *obj);
extern const u8 gSprite_0202C150[];
void ObjBurstInit(struct Obj *obj);
void ObjBurstFly(struct Obj *obj);
void ObjBurst(struct Obj *obj);
void Func_0201BAD8(void);
#ifndef VERSION_GZLJ01
extern const u8 gMode2UsString[];
#endif
// Per item effect (GameState.effect[0], slot 7), run every frame by UpdateItemEffect.
extern void (*const gItemEffectFuncs[18])(void);
extern const u8 gSprite_0202C928[];
void Func_0201ECC4(void);
void UpdateItemEffect(void);
void Func_0201F878(void);
void Func_0201FCA0(void);
void Func_020208FC(u8 open);
void Func_0201ED08(void);
void SendItemUse(u8 item, u8 arg);
#define GBACOM_BUF_SHOP     ((u8 *)(PROG_BASE - 0x570))
#define GBACOM_BUF_ITEM_BUY ((u8 *)(PROG_BASE - 0x560))
// Shop purchase requests (slot 12), 3 bytes each.
extern const u8 gShopRequests[9];

#endif // GUARD_GAME_H
