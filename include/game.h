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

// One item (gGameState.items, indexed by item number; 0 is none). ItemMenuInit copies all
// twelve from gShopItemsInit.
struct ItemSlot
{
    /*0x0*/ u16 tile;        // BG map entry of the icon's top-left tile (2x2 tiles, palette on top)
    /*0x2*/ u16 objTile;     // OBJ tile of the icon as a sprite (DrawItemSprite)
    /*0x4*/ s16 unk4;        // passed to MsgStartSilent
    /*0x6*/ u8 price;        // rupees (0, 10-80); DrawItemCount shows it
    /*0x7*/ u8 objPalette;
};

// Game state in IWRAM (0x030050E0 in every version), 0x240 bytes up to gGbaComDataStatus.
// The struct body is generated from this list by tools/genstruct.py.
// genstruct GameState 0x240
// 0x000 struct GbaComFlags flags size=12  // last slot 9 packet from the GC
// 0x00C struct MapImage* mapImage  // the slot 2 map image (LoadMapImage)
// 0x010 u32 unk010
// 0x014 s16 mapX  // top left of the map view in the map image (pixels)
// 0x016 s16 mapY
// 0x018 u8 unk018  // cleared by UpdateMap
// 0x022 u16 unk022  // set to 240 when a target is placed (MapIconCursor)
// 0x024 u16 targetScale  // pulse of the target compass sprite (UpdateTargetCompass)
// 0x019 u8 unk019  // cleared when unk222 turns on (R)
// 0x026 u16 unk026
// 0x028 u16 seaAnimTimer[4] size=8  // per quadrant of the view (UpdateMap)
// 0x030 u16 unk030
// 0x032 s16 unk032  // title sprite x
// 0x034 s16 unk034  // title sprite y
// 0x036 s16 unk036
// 0x038 s16 unk038
// 0x03A u16 switchSound  // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
// 0x03C u8 switchNo  // 0xFF: none
// 0x03D u8 switchStageNo
// 0x03E u8 switchRoomNo
// 0x03F u8 switchType  // the GC switches on type - 1
// 0x040 u8 switchNo2
// 0x042 u8 unk042
// 0x044 struct MapIcon icons[4] size=0x20  // drawn over the map by UpdateMapIcons
// 0x064 struct MapMarker markers[16] size=0x80  // from slot 3 (Mode1RecvMarkers)
// 0x0E4 u8 markerCount
// 0x0E5 u8 unk0E5
// 0x0E8 u8 seaTileSet[4]  // gSeaSquareTileSets of each quadrant's square
// 0x0EC u8 seaAnimFrame[4]  // index into gSeaAnimFrames
// 0x0F0 u8 seaSquare[4]  // sea chart square of each quadrant
// 0x0F4 u8 unk0F4[8] size=8  // slot 11 (MainMode1 copies it here); one bit per sea chart square (7x7)
// 0x0FC u8 unk0FC
// 0x0FD u8 unk0FD
// 0x0FF u8 unk0FF  // frames left in the frame of gSprite_0202C400
// 0x100 u8 unk100  // its frame (0-3)
// 0x101 u8 targetBlinkOn
// 0x102 u8 targetDir  // direction (0-7) last announced by UpdateTargetCompass; 0xFF: none
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
// 0x11F u8 mapScrolling  // L pressed to scroll the map (UpdateMap)
// 0x120 u8 unk120
// 0x121 u8 unk121
// 0x122 u8 unk122
// 0x123 u8 unk123
// 0x124 u8 unk124
// 0x126 u8 unk126
// 0x128 u8 unk128
// 0x12A u8 unk12A
// 0x12B u8 unk12B  // item effect step (Effect5, Effect8, Effect12)
// 0x12C u8 unk12C
// 0x12D u8 unk12D
// 0x12E u8 unk12E
// 0x12F u8 unk12F
// 0x130 u8 unk130
// 0x131 u8 unk131
// 0x132 u8 unk132
// 0x133 u8 unk133  // slot 3 byte 1
// 0x134 u8 unk134
// 0x136 u8 unk136
// 0x138 bits u8 unk138_0:2 unk138_2:1 unk138_3:1 unk138_4:1 unk138_5:1 unk138_6:2  // sent as GbaFlg.unk2
// 0x13C u8 unk13C  // a sound is playing (GamePlaySound)
// 0x13D s8 msgBoxPos  // message box slide, steps toward 10 while unk13C is set, else 0
// 0x13E u16 unk13E
// 0x140 s16 unk140
// 0x142 s16 unk142  // message id (MsgStart)
// 0x144 const u8* msgPtr  // message text (in the EWRAM message block at 0x02000000); US Mode2State0 sets it too
// 0x148 u8 textX  // text cursor, pixels
// 0x149 u8 textY
// 0x14A u8 unk14A
// 0x14B u8 unk14B
// 0x14C u8 unk14C
// 0x14D u8 unk14D
// 0x14E u8 unk14E
// 0x14F u8 unk14F
// 0x150 u8 unk150
// 0x151 u8 unk151
// 0x152 u8 unk152
// 0x153 u8 unk153
// 0x156 u8 unk156
// 0x157 u8 unk157
// 0x158 u8 unk158
// 0x159 u8 unk159
// 0x15A u8 unk15A
// 0x15C u8 effect[4] size=4  // slot 7: item use result
// 0x160 struct ItemSlot items[12] size=0x60
// 0x1C0 u16 menuCursorX  // item menu cursor position (8.8), moving toward the slot
// 0x1C2 u16 menuCursorY
// 0x1C4 u16 menuCursorDX
// 0x1C6 u16 menuCursorDY
// 0x1C8 u16 unk1C8
// 0x1CA u16 unk1CA
// 0x1CC u8 unk1CC
// 0x1CD u8 menuCursor  // item menu slot (5 x 3; row 2 is the dungeon chest row)
// 0x1CE u8 menuKeyRepeat  // frames to the next d-pad repeat
// 0x1CF u8 slotItems[13] size=13  // item shown in each menu slot (SetSlotItem); rows 0-1 index items
// 0x1E0 u8 unk1E0
// 0x1DE u8 unk1DE
// 0x1DF u8 unk1DF
// 0x1E1 u8 unk1E1  // chest bits that changed
// 0x1E2 u8 unk1E2  // item effect step (Effect7, Effect11)
// 0x1E3 u8 unk1E3  // chests opened
// 0x1E4 u8 unk1E4
// 0x1E5 bits u8 unk1E5_0:1 unk1E5_1:1 unk1E5_2:1 unk1E5_3:1 unk1E5_4:1 unk1E5_5:3
// 0x1E8 u16 unk1E8  // select window edge x (0xF8 closed, 0xFFF8 fully across)
// 0x1EA s16 unk1EA  // select window edge y
// 0x1EE u8 unk1EE  // UpdateSelectWindow step (0: closed)
// 0x1F4 u8 unk1F4  // DrawStatusIcon: which icon
// 0x1F5 u8 unk1F5
// 0x1F8 s16 unk1F8
// 0x1FB u8 unk1FB
// 0x1FA u8 unk1FA  // MainLoop tests 0x1FA-0x1FB as one u16
// 0x1FC s16 rupees  // shown on screen; counts toward flags.rupees
// 0x1FE s16 rupeeDelta  // flags.rupees - rupees
// 0x202 u16 unk202  // Tingle animation: frames left in the current frame
// 0x204 u16 unk204
// 0x206 u16 unk206
// 0x208 u16 unk208
// 0x20A u16 waveAmplitude  // intro wave (Mode1State1), counts down to 0
// 0x20E u16 wavePhase  // 8.8; the high byte indexes gSinTable
// 0x210 u16 timer  // flags.timer
// 0x212 u8 playerName[8]  // slot 10
// 0x21A u8 playerNameIndex  // next letter of playerName for message code 0xFB
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
// 0x229 u8 unk229
// 0x22A u8 unk22A
// 0x22B u8 unk22B
// 0x22C u8 unk22C
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
    /*0x00C*/ struct MapImage* mapImage;      // the slot 2 map image (LoadMapImage)
    /*0x010*/ u32 unk010;
    /*0x014*/ s16 mapX;                       // top left of the map view in the map image (pixels)
    /*0x016*/ s16 mapY;
    /*0x018*/ u8 unk018;                      // cleared by UpdateMap
    /*0x019*/ u8 unk019;                      // cleared when unk222 turns on (R)
    /*0x01A*/ u8 filler01A[0x022 - 0x01A];
    /*0x022*/ u16 unk022;                     // set to 240 when a target is placed (MapIconCursor)
    /*0x024*/ u16 targetScale;                // pulse of the target compass sprite (UpdateTargetCompass)
    /*0x026*/ u16 unk026;
    /*0x028*/ u16 seaAnimTimer[4];            // per quadrant of the view (UpdateMap)
    /*0x030*/ u16 unk030;
    /*0x032*/ s16 unk032;                     // title sprite x
    /*0x034*/ s16 unk034;                     // title sprite y
    /*0x036*/ s16 unk036;
    /*0x038*/ s16 unk038;
    /*0x03A*/ u16 switchSound;                // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
    /*0x03C*/ u8 switchNo;                    // 0xFF: none
    /*0x03D*/ u8 switchStageNo;
    /*0x03E*/ u8 switchRoomNo;
    /*0x03F*/ u8 switchType;                  // the GC switches on type - 1
    /*0x040*/ u8 switchNo2;
    /*0x041*/ u8 filler041[0x042 - 0x041];
    /*0x042*/ u8 unk042;
    /*0x043*/ u8 filler043[0x044 - 0x043];
    /*0x044*/ struct MapIcon icons[4];        // drawn over the map by UpdateMapIcons
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
    /*0x0FE*/ u8 filler0FE[0x0FF - 0x0FE];
    /*0x0FF*/ u8 unk0FF;                      // frames left in the frame of gSprite_0202C400
    /*0x100*/ u8 unk100;                      // its frame (0-3)
    /*0x101*/ u8 targetBlinkOn;
    /*0x102*/ u8 targetDir;                   // direction (0-7) last announced by UpdateTargetCompass; 0xFF: none
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
    /*0x11F*/ u8 mapScrolling;                // L pressed to scroll the map (UpdateMap)
    /*0x120*/ u8 unk120;
    /*0x121*/ u8 unk121;
    /*0x122*/ u8 unk122;
    /*0x123*/ u8 unk123;
    /*0x124*/ u8 unk124;
    /*0x125*/ u8 filler125[0x126 - 0x125];
    /*0x126*/ u8 unk126;
    /*0x127*/ u8 filler127[0x128 - 0x127];
    /*0x128*/ u8 unk128;
    /*0x129*/ u8 filler129[0x12A - 0x129];
    /*0x12A*/ u8 unk12A;
    /*0x12B*/ u8 unk12B;                      // item effect step (Effect5, Effect8, Effect12)
    /*0x12C*/ u8 unk12C;
    /*0x12D*/ u8 unk12D;
    /*0x12E*/ u8 unk12E;
    /*0x12F*/ u8 unk12F;
    /*0x130*/ u8 unk130;
    /*0x131*/ u8 unk131;
    /*0x132*/ u8 unk132;
    /*0x133*/ u8 unk133;                      // slot 3 byte 1
    /*0x134*/ u8 unk134;
    /*0x135*/ u8 filler135[0x136 - 0x135];
    /*0x136*/ u8 unk136;
    /*0x137*/ u8 filler137[0x138 - 0x137];
    /*0x138*/ u8 unk138_0:2;                  // sent as GbaFlg.unk2
              u8 unk138_2:1;
              u8 unk138_3:1;
              u8 unk138_4:1;
              u8 unk138_5:1;
              u8 unk138_6:2;
    /*0x139*/ u8 filler139[0x13C - 0x139];
    /*0x13C*/ u8 unk13C;                      // a sound is playing (GamePlaySound)
    /*0x13D*/ s8 msgBoxPos;                   // message box slide, steps toward 10 while unk13C is set, else 0
    /*0x13E*/ u16 unk13E;
    /*0x140*/ s16 unk140;
    /*0x142*/ s16 unk142;                     // message id (MsgStart)
    /*0x144*/ const u8* msgPtr;               // message text (in the EWRAM message block at 0x02000000); US Mode2State0 sets it too
    /*0x148*/ u8 textX;                       // text cursor, pixels
    /*0x149*/ u8 textY;
    /*0x14A*/ u8 unk14A;
    /*0x14B*/ u8 unk14B;
    /*0x14C*/ u8 unk14C;
    /*0x14D*/ u8 unk14D;
    /*0x14E*/ u8 unk14E;
    /*0x14F*/ u8 unk14F;
    /*0x150*/ u8 unk150;
    /*0x151*/ u8 unk151;
    /*0x152*/ u8 unk152;
    /*0x153*/ u8 unk153;
    /*0x154*/ u8 filler154[0x156 - 0x154];
    /*0x156*/ u8 unk156;
    /*0x157*/ u8 unk157;
    /*0x158*/ u8 unk158;
    /*0x159*/ u8 unk159;
    /*0x15A*/ u8 unk15A;
    /*0x15B*/ u8 filler15B[0x15C - 0x15B];
    /*0x15C*/ u8 effect[4];                   // slot 7: item use result
    /*0x160*/ struct ItemSlot items[12];
    /*0x1C0*/ u16 menuCursorX;                // item menu cursor position (8.8), moving toward the slot
    /*0x1C2*/ u16 menuCursorY;
    /*0x1C4*/ u16 menuCursorDX;
    /*0x1C6*/ u16 menuCursorDY;
    /*0x1C8*/ u16 unk1C8;
    /*0x1CA*/ u16 unk1CA;
    /*0x1CC*/ u8 unk1CC;
    /*0x1CD*/ u8 menuCursor;                  // item menu slot (5 x 3; row 2 is the dungeon chest row)
    /*0x1CE*/ u8 menuKeyRepeat;               // frames to the next d-pad repeat
    /*0x1CF*/ u8 slotItems[13];               // item shown in each menu slot (SetSlotItem); rows 0-1 index items
    /*0x1DC*/ u8 filler1DC[0x1DE - 0x1DC];
    /*0x1DE*/ u8 unk1DE;
    /*0x1DF*/ u8 unk1DF;
    /*0x1E0*/ u8 unk1E0;
    /*0x1E1*/ u8 unk1E1;                      // chest bits that changed
    /*0x1E2*/ u8 unk1E2;                      // item effect step (Effect7, Effect11)
    /*0x1E3*/ u8 unk1E3;                      // chests opened
    /*0x1E4*/ u8 unk1E4;
    /*0x1E5*/ u8 unk1E5_0:1;
              u8 unk1E5_1:1;
              u8 unk1E5_2:1;
              u8 unk1E5_3:1;
              u8 unk1E5_4:1;
              u8 unk1E5_5:3;
    /*0x1E6*/ u8 filler1E6[0x1E8 - 0x1E6];
    /*0x1E8*/ u16 unk1E8;                     // select window edge x (0xF8 closed, 0xFFF8 fully across)
    /*0x1EA*/ s16 unk1EA;                     // select window edge y
    /*0x1EC*/ u8 filler1EC[0x1EE - 0x1EC];
    /*0x1EE*/ u8 unk1EE;                      // UpdateSelectWindow step (0: closed)
    /*0x1EF*/ u8 filler1EF[0x1F4 - 0x1EF];
    /*0x1F4*/ u8 unk1F4;                      // DrawStatusIcon: which icon
    /*0x1F5*/ u8 unk1F5;
    /*0x1F6*/ u8 filler1F6[0x1F8 - 0x1F6];
    /*0x1F8*/ s16 unk1F8;
    /*0x1FA*/ u8 unk1FA;                      // MainLoop tests 0x1FA-0x1FB as one u16
    /*0x1FB*/ u8 unk1FB;
    /*0x1FC*/ s16 rupees;                     // shown on screen; counts toward flags.rupees
    /*0x1FE*/ s16 rupeeDelta;                 // flags.rupees - rupees
    /*0x200*/ u8 filler200[0x202 - 0x200];
    /*0x202*/ u16 unk202;                     // Tingle animation: frames left in the current frame
    /*0x204*/ u16 unk204;
    /*0x206*/ u16 unk206;
    /*0x208*/ u16 unk208;
    /*0x20A*/ u16 waveAmplitude;              // intro wave (Mode1State1), counts down to 0
    /*0x20C*/ u8 filler20C[0x20E - 0x20C];
    /*0x20E*/ u16 wavePhase;                  // 8.8; the high byte indexes gSinTable
    /*0x210*/ u16 timer;                      // flags.timer
    /*0x212*/ u8 playerName[8];               // slot 10
    /*0x21A*/ u8 playerNameIndex;             // next letter of playerName for message code 0xFB
    /*0x21B*/ u8 filler21B[0x21C - 0x21B];
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
    /*0x229*/ u8 unk229;
    /*0x22A*/ u8 unk22A;
    /*0x22B*/ u8 unk22B;
    /*0x22C*/ u8 unk22C;
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
void DrawFloorNumber(void);
// Shows message id (0-0x437) with Tingle's voice; arg goes to gGameState.unk226 (his animation).
// The definition narrows id to u16, but callers see an s16 parameter (they sign-extend).
bool32 GamePlaySound(s16 id, u8 arg);
bool32 InitMapView(void);
void Func_0201A540(void);
bool32 StepToward(s16 *pos, s16 target, s16 speed);
void UpdateSelectWindow(void);
// Callers see s16 x and y (DrawTimerCount sign-extends them).
void DrawSprite(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u16 a6, s16 a7, s16 a8,
                u8 a9, u8 a10);
// The definition takes u16 x and y; callers see s16 (they sign-extend).
void DrawSpriteAt(const void *sprite, u8 frame, s16 x, s16 y, u8 a4, u8 a5, u8 a6);
bool32 LoadMapImage(void);  // returns nothing (falls off the end)
bool32 ItemMenuInit(void);
void ResetSelectWindow(void);
bool32 MsgReset(void);
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
void UpdateMap(void);
void Func_0201CFE8(void);
void Func_0201B7D4(void);
void MsgClose(void);
bool32 UseItem(u8 item);
void Func_0201B0F4(void);
void Func_0201B8C8(void);
u8 FindMarker(u8 type);
void ItemMenuUpdate(void);
void DrawStatusIcon(void);
u8 Func_0201B3A4(void);
void Func_0201C484(u8 id);
void Func_0201C568(void);
void SetSlotItem(u8 a, u8 b);
void UpdateHudPanels(void);
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

// The map image the GC sends in slot 2, in the message buffer (gGbaComMessageBuf). Its two
// data pointers arrive as offsets from the header; LoadMapImage turns them into addresses.
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
void MapIconFollowMarker(struct MapIcon *icon);
void InitMarkerIcon(void);
void MapIconBlink(struct MapIcon *icon);
void InitTargetIcon(void);
void InitCursorIcon(void);
void MapIconCursor(struct MapIcon *icon);
void UpdateMapIcons(void);
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
// Text: glyphs are drawn at gGameState.textX/textY (pixels) in a window 0x60 wide and 0x50
// high; unk157 is the glyph width (6 or 8) and unk15A doubles the size.
bool32 Func_0201C60C(void);  // wraps the cursor to the next line; FALSE when the window is full
void Func_0201BCC0(u8 x, u8 y, u8 a2, u16 ch);
void Func_0201BDE4(u8 x, u8 y, u8 a2, u16 ch);
void Func_0201C0E8(u8 arg);
s32 Func_0201C204(void);
void DrawChestRow(void);
bool32 SendItemUse(u8 a, u8 b);
void DrawGlyphRun(u8 id);
bool32 DrawChar(u8 ch);
#ifdef VERSION_GZLJ01
#define GLYPH_DIGIT_0 0x76
#define DRAW_CHAR DrawChar
#else
#define GLYPH_DIGIT_0 0x35
bool32 UsDrawChar(u8 ch);
#define DRAW_CHAR UsDrawChar  // the US callers go through this wrapper
#endif
s32 MsgStep(void);
void MsgStart(s16 id);
void DrawItemIcon(u8 item, u16 *map);
void DrawItemCount(u8 item, u16 *map);
void SetSelectedItem(u8 item);

// One OAM entry (the hardware attribute layout), as DrawItemSprite fills gOamBuffer.
struct OamData
{
    /*0x0*/ u32 y:8;
            u32 affineMode:2;
            u32 objMode:2;
            u32 mosaic:1;
            u32 bpp:1;
            u32 shape:2;
    /*0x2*/ u32 x:9;
            u32 matrixNum:3;
            u32 hFlip:1;
            u32 vFlip:1;
            u32 size:2;
    /*0x4*/ u16 tileNum:10;
            u16 priority:2;
            u16 paletteNum:4;
    /*0x6*/ u16 affineParam;
};

// gGameState.items ... slotItems (0x160-0x1DB) seen as one block: SetSlotItem addresses
// slotItems through a pointer to it (base + 0x6F), which is how the original loads it.
struct ItemMenu
{
    /*0x00*/ struct ItemSlot items[13];
    /*0x68*/ u16 unk68;  // gGameState.unk1C8
    /*0x6A*/ u16 unk6A;  // gGameState.unk1CA
    /*0x6C*/ u8 unk6C;   // gGameState.unk1CC
    /*0x6D*/ u8 filler6D[2];
    /*0x6F*/ u8 slotItems[13];
};

extern const u16 gMsgTingleAnims[];  // message code 0xF5: argument -> gGameState.unk226
extern const u16 gGlyphRunStart[];   // DrawGlyphRun: first glyph of each run
extern const u8 gGlyphRunLength[];
extern const u8 gGlyphTable_0202438E[];  // DrawChar: glyphs for codes 0xE0-0xE5 and 0xEB-
extern const u8 gDayNameChars[7 * 2];    // two glyphs per weekday (flags.dayOfWeek), JP only
extern void (*const gMsgCmdFuncs[])(void);  // message code 0xF3: MsgCmd00 ... MsgCmd12
extern const u8 gSprite_0202BDB8[];  // menu slot cursor (DrawSlotCursor)
extern const u8 gSprite_0202BDF0[];  // "more text" arrow (MsgStep)
extern const u8 gSprite_0202C130[];

void Mode1StartFadeOut(void);
void Func_02018988(s8 *pos, u8 target, s8 speed);  // steps *pos toward target
void Func_0201BA2C(s8 pos);
void Func_0201BA9C(s8 pos);
void Func_0201BEA0(s8 pos);
// Tingle's animations (gGameState.unk226), each a list of sprites (gGameState.unk227 picks one).
extern const u8 *const *const gTingleAnims[21];
extern const u8 gSprite_0202C400[];
extern const u8 gSprite_0202C650[];
extern const u8 gSprite_0202C740[];

// ---- game.c part E (GZLJ01 0201DEAC-0201F308) ----

extern const struct ItemSlot gShopItemsInit[12];

#define GBACOM_BUF_ITEM ((u8 *)(PROG_BASE - 0x6C0))

// One frame of a sprite animation: the sprite data and how many frames it shows.
struct SpriteAnimFrame
{
    /*0x0*/ const void *sprite;
    /*0x4*/ u8 duration;
};

// Animations (they overlap: each starts somewhere in one frame table).
extern const struct SpriteAnimFrame gSprite_0202C928[];
extern const struct SpriteAnimFrame gSprite_0202C940[];
extern const struct SpriteAnimFrame gSprite_0202C950[];
extern const u8 gSprite_0202CC60[];

void Func_0201E584(void);
void Func_0201EC10(void);
void Func_0201ECC4(void);
void Func_0201ED08(void);
void Func_0201EDF8(s16 x, s16 y, u8 kind);
void Func_0201F2A8(void);
// The definition takes a u16; callers see s16 (they sign-extend).
bool32 MsgStartSilent(s16 id);
void DrawSlotCursor(u8 cursor);
void DrawItemSprite(u8 item, u8 x, u8 y, u8 mosaic);
void ChestCounterMain(void);
u8 Func_02018A90(u8 *timer);  // counts *timer down to 0 and returns it
extern const s16 gChestRowIds[5];
extern const u16 gHudPanelTiles[3];
u32 MarkerDistance(u8 arg);
void Func_0201FCA0(void);
void Func_0201F878(void);
// The definition takes u16 x and y; callers see s16 (they sign-extend).

// ---- game.c part F (GZLJ01 0201F308-02020E28) ----

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
void SelectWindowWipe(u8 open);
#define GBACOM_BUF_SHOP     ((u8 *)(PROG_BASE - 0x570))
#define GBACOM_BUF_ITEM_BUY ((u8 *)(PROG_BASE - 0x560))
// Shop purchase requests (slot 12), 3 bytes each.
extern const u8 gShopRequests[9];
extern const s16 gBobOffsets_020244DA[4];
extern const u8 gSprite_0202C490[];
// The definition narrows x and y to u16; callers see s16 (they sign-extend).
void Func_0201F8C4(s16 x, s16 y, u8 kind);
void Func_0201FEEC(void);
void DrawTimerCount(s16 x, s16 y);
// The definition narrows x and y to u16; callers see s16 (they sign-extend).
void Func_020190A8(const void *sprite, s16 x, s16 y, u8 a3, u8 a4);
extern const u8 gSprite_0202CC50[];
extern const u8 gSprite_0202C420[];
extern const u8 gSprite_0202C428[];
extern const u8 gSprite_0202C430[];
extern const u8 gSprite_0202C440[];
extern const u8 gSprite_0202C448[];
extern const u8 gSprite_0202C498[];
extern const u8 gSprite_0202C4A8[];

#endif // GUARD_GAME_H
