#include "global.h"
#include "game.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

void srand(unsigned int seed);
int rand(void);

// Game mode 1 (the Tingle Tuner screen proper). MainMode1 runs one state per frame.

void Mode1State0(void);
void Mode1State1(void);
void Mode1State2(void);
void Mode1State3(void);
void Mode1State4(void);
void Mode1State5(void);
void Mode1SendSwitch(u8 soundArg);
void Mode1RecvMarkers(void);
void Mode1Func_02016350(void);

// Not referenced by any code. It ends in 0xFF, like a short text in the message font's glyph
// codes (see MsgStep).
const u8 gMode1Unused_02023E38[12] = {0x7D, 0x7A, 0x01, 0x00, 0xB2, 0xB3, 0x00, 0x2B, 0x00, 0x00, 0x00, 0xFF};

// Sound per dungeon number (gGameState.flags.dungeonNo), played by Mode1State1.
const s16 gDungeonSounds[18] = {50, 51, 52, 53, 54, 55, 82, 83, 84, 85, 86, 86, 138, 142, 135, 137, 141, 57};

void (*const gMode1StateFuncs[])(void) =
{
    Mode1State0,
    Mode1State1,
    Mode1State2,
    Mode1State3,
    Mode1State4,
    Mode1State5,
};

// BG map entry of the rupee icon (tile 0x44) per wallet size: the palette gives its colour.
const u16 gRupeeIconTiles[3] = {0xC044, 0xA044, 0x4044};
// Sound per dungeon chest bit (flags.dungeonChests: WT, ET, TOTG, FW, DRC).
const s16 gChestSounds[5] = {0xE6, 0xE7, 0xE8, 0xEA, 0xE9};

void Mode1SendSwitch(u8 soundArg);

void InitDisplayIntr(void)
{
    REG_IME = 0;
    REG_DISPCNT = 0x80;  // forced blank
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IE |= INTR_FLAG_VBLANK;
    REG_IME = 1;
}

void SeedRandom(void)
{
    srand(1);
}

void UpdateRandom(void)
{
    gMain.random = rand();
}

// Step the rupee counter toward the real count and draw it (up to 4 digits, two tile
// rows, no leading zeros).
void Mode1Func_020150A4(void)
{
    u16 *map = (u16 *)0x06006450;
    s32 digits[4];
    s32 step;
    s32 i;
    s32 drawn;
    s32 shown;

    if (gGameState.rupeeDelta != 0)
    {
        step = 1;
        if (gGameState.rupeeDelta < 0)
            step = -1;
        gGameState.rupees += step;
        gGameState.rupeeDelta -= step;
        if (gMain.modeState == 2)
        {
            if (gGameState.rupeeDelta == 0)
                SoundPlay(SE_RUPEE_COUNT_END, 0);
            else if (gGameState.rupeeDelta & 1)
            {
                if (step < 0)
                    SoundPlay(SE_RUPEE_COUNT_DOWN, 0);
                else
                    SoundPlay(SE_RUPEE_COUNT_UP, 0);
            }
        }
    }
    digits[0] = (s16)(gGameState.rupees / 1000) * 2;
    digits[1] = (s16)((s16)(gGameState.rupees / 100) % 10) * 2;
    digits[2] = (s16)((s16)(gGameState.rupees / 10) % 10) * 2;
    digits[3] = (s16)(gGameState.rupees % 10) * 2;
    drawn = 0;
    shown = 0;
    for (i = 0; i < 4; i++)
    {
        if (i == 0)
        {
            if (digits[i] != 0)
            {
                map[0] = digits[i] + 0xB030;
                map[0x20] = digits[i] + 0xB031;
                map++;
                shown++;
                drawn++;
            }
        }
        else
        {
            if (digits[i] != 0 || i == 3 || shown != 0)
            {
                map[0] = digits[i] + 0xB030;
                map[0x20] = digits[i] + 0xB031;
                shown++;
            }
            else
            {
                map[0] = 0xB000;
                map[0x20] = 0xB000;
            }
            map++;
            drawn++;
        }
    }
    if (drawn != 4)
    {
        map[0] = 0xB000;
        map[0x20] = 0xB000;
    }
}
void Mode1Func_0201521C(void)
{
    if (gGameState.flags.dungeonNo == 0)
    {
        CpuCopy16((void *)0x06007B40, (void *)0x06006840, 0x80);
        DrawFloorNumber();
        LZ77UnCompVram(gMode1SeaMap, (void *)0x06002000);
        gMain.bgcnt[2] = 0xC409;
        gMain.bgcnt[3] = 0xC809;
    }
    else
    {
        gMain.bgcnt[3] = 0x0B41;
    }
}

void Mode1Func_02015298(void)
{
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1BgMap, (void *)0x06006000);
    Mode1Func_0201521C();
}
// Set up the screen and the game state, then start the intro wave (state 1).
// Near-miss: the original computes &gGameState.unk106 early, into r6 (right after the
// unk21E store, though unk106 is stored much later), which shifts r6/r7 through the rest.
/*
void Mode1State0(void)
{
    s32 i;

    if (!gGameState.flagsReceived)
        return;
    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x100;  // BG0
    gMain.bgcnt[0] = 0x4C08;
    gMain.bgcnt[1] = 0x0E08;
    gMain.bgOfs[3][0] = 0;
    gMain.bgOfs[3][1] = 0;
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.bgOfs[1][0] = 0x7C;
    gMain.bgOfs[1][1] = -4;
    gMain.bgOfs[2][0] = 0;
    gMain.bgOfs[2][1] = 0;
    gGameState.unk0FC = 1;
    gGameState.unk21D = 0;
    gGameState.unk21E = 0;
    gGameState.targetDir = 0xFF;
    gGameState.unk21F = 0;
    gGameState.unk220 = 0;
    gGameState.dungeonChests = gGameState.flags.dungeonChests;
    gGameState.unk157 = 8;
    gGameState.unk0FD = 0;
    gGameState.unk140 = 0xFFFF;
    gGameState.unk1F8 = -1;
    gGameState.unk121 = 0;
    gGameState.unk11B = 0;
    gGameState.unk11A = 0;
    gGameState.unk11D = 0;
    gGameState.unk010 = 0;
    gGameState.unk11C = 0;
    gGameState.unk106 = 0;
    gGameState.unk030 = 0;
    gGameState.unk120 = 0;
    gGameState.switchSound = -1;
    gGameState.switchType = 0;
    gGameState.unk140 = -1;
    gGameState.unk1F8 = -1;
    gGameState.unk136 = 0;
    gGameState.unk1E0 = 0;
    for (i = 0; i < 16; i++)
        gObjs[i].active = 0;
    gGameState.unk10F = gSpriteTable[9][4];
    gGameState.unk110 = 0;
    gGameState.unk113 = gSpriteTable[13][4];
    gGameState.unk114 = 0;
    gGameState.unk115 = gSpriteTable[16][4];
    gGameState.unk116 = 0;
    gGameState.unk117 = gSpriteTable[19][4];
    gGameState.unk118 = 0;
    Mode1Func_02015298();
    if (gGameState.flags.unkA_0)
    {
        gMain.unk457 = 1;
        gGameState.unk10E = 0;
    }
    else
    {
        gMain.unk457 = gMain.inDungeon;
        gGameState.unk10E = 1;
    }
    ResetSelectWindow();
    InitMapView();
    if (gGameState.flags.dungeonNo != 0)
    {
        gGameState.unk108 = 0;
        gGameState.unk10A = 0;
        gGameState.unk10C = 0;
        if (gGameState.unk10E)
            UpdateMap();
    }
    else
    {
        UpdateMap();
    }
    MsgReset();
    gGameState.switchSound = 0xFFFF;
    gGameState.unk21C = 0;
    gGameState.unk206 = 0x300;
    ItemMenuInit();
    gGameState.unk1E5_1 = 1;
    gGameState.unk12B = 0;
    gGameState.unk1FA = 0;
    gGameState.unk1FB = 0;
    gGameState.unk224 = 0;
    gGameState.rupeeDelta = 0;
    Mode1Func_020150A4();
    gMain.bldcnt = 0;
    gMain.bldalpha = 0;
    gGameState.wavePhase = 0;
    gGameState.scrollTableNext = 1;
    gGameState.scrollTable = 0;
    gGameState.waveAmplitude = 0x60;
    BuildWaveScrollTable(BG_SCROLL_TABLE_0, 0, 0x60);
    gMain.hblankScroll = TRUE;
    gMain.modeState = 1;
}
*/
INCLUDE_ASM(Mode1State0);
// The screen comes in with a wave (per-line BG0HOFS by HBlank DMA, double-buffered) that
// dies down; then the dungeon music starts and the game goes to state 2.
void Mode1State1(void)
{
    u16 *table;

    if (gGameState.scrollTableNext)
        table = BG_SCROLL_TABLE_1;
    else
        table = BG_SCROLL_TABLE_0;
    gGameState.wavePhase += (gGameState.waveAmplitude << 4) + 0x100;
    BuildWaveScrollTable(table, gGameState.wavePhase >> 8, gGameState.waveAmplitude);
    gGameState.scrollTableNext ^= 1;
    if (--gGameState.waveAmplitude == 0)
    {
        gMain.hblankScroll = FALSE;
        ShowMessage(gDungeonSounds[gGameState.flags.dungeonNo], 2);
        SoundPlay(0, 0);
        gGameState.unk21C = 1;
        gMain.modeState = 2;
        if (gGameState.flags.dungeonNo == 0)
        {
            gMain.dispcnt = 0x1F00;  // BG0-3 and OBJ
            gGameState.unk23C = 0;
        }
        else if (gGameState.unk23C == 0)
        {
            gMain.dispcnt = 0x1B00;  // BG0, BG1, BG3, OBJ
        }
        else
        {
            gMain.dispcnt = 0x1300;  // BG0, BG1, OBJ
        }
    }
    Func_0201A540();
}
// Fade in: step the mosaic and brightness back down every other frame, then go to state 2.
void Mode1State5(void)
{
    if (--gGameState.fadeTimer == 0)
    {
        gMain.mosaic -= 0x1111;
        gMain.bldy = (gMain.mosaic & 0xF) >> 1;
        if (gMain.mosaic == 0)
        {
            if (gMain.bgcnt[0] & 0x40)
                gMain.bgcnt[0] ^= 0x40;
            if (gMain.bgcnt[1] & 0x40)
                gMain.bgcnt[1] ^= 0x40;
            if (gMain.bgcnt[2] & 0x40)
                gMain.bgcnt[2] ^= 0x40;
            if (gMain.bgcnt[3] & 0x40)
                gMain.bgcnt[3] ^= 0x40;
            if (gGameState.unk121)
            {
                ShowMessage(0x63, 1);
                gGameState.unk121 = 0;
            }
            gMain.bldcnt = 0;
            gMain.bldy = 0;
            gMain.modeState = 2;
        }
        else
        {
            gGameState.fadeTimer = 2;
        }
    }
    UpdateMap();
    Func_0201CFE8();
}
// Near-miss: the original loads GBACOM_BUF_SWITCH + 1 into r2 after the first store; this
// loads it first (assigning p after the store puts it in r1 instead).
/*
void Mode1SendSwitch(u8 soundArg)
{
    u8 *p;

    p = GBACOM_BUF_SWITCH + 1;
    GBACOM_BUF_SWITCH[0] = gGameState.switchNo;
    *p++ = gGameState.switchStageNo;
    *p++ = gGameState.switchRoomNo;
    *p++ = gGameState.switchType;
    *p = gGameState.switchNo2;
    GbaComSendDataSet(GBACOM_BUF_SWITCH, 6, GBACOM_SLOT_SWITCH,
                      ((u32 *)GBACOM_BUF_SWITCH)[0] + *(u32 *)p);
    ShowMessage(gGameState.switchSound, soundArg);
    gGameState.unk21C = 1;
    gGameState.unk206 = 0x100;
    if ((u8)(gGameState.switchType - 1) > 2 && gGameState.switchType != 14 && gGameState.switchType != 15)
    {
        gGameState.unk042 = 0;
        gGameState.switchType = 0;
    }
}
*/
INCLUDE_ASM(Mode1SendSwitch);
// The main state: sounds for the GC's flags, the timed marker/switch action (slot 3 marker
// type 7 sends a switch), the BG1 slide, then the START/A/B/R keys.
void Mode1State2(void)
{
    u32 keys;

    if (gGameState.flags.unk3_2 && gGameState.unk1CC == 0 && gGameState.unk1EE == 0
     && gGameState.unk224 == 0)
        ShowMessage(0x51, 0xF);
    if (gGameState.flags.unk5_2 && !gGameState.flags.busy && gGameState.unk1CC == 0
     && gGameState.unk1EE == 0)
    {
        if (gGameState.flags.isFree)
            SoundPlayLoop(7, 7, 0);
        else if (!gGameState.flags.followTarget)
        {
            // An 8-byte parameter block; only the low half of word 1 is set (the rest is
            // left as it was on the stack). Written as a bitfield store (struct with
            // u32 x:16), agbcc loads the mask later and permutes the registers.
            u32 params[2];
            u32 mask = 0xFFFF0000;
            u16 v;

            params[0] = 0;
            v = (gGameState.flags.unk5_2 - 1) ^ 3;
            params[1] = (params[1] & mask) | v;
            SoundPlayLoop(9, 9, params);
        }
    }
    if (gGameState.flags.isFree != gGameState.isFree)
    {
        if (gGameState.flags.isFree)
            SoundPlay(0x16 - SOUND_ID_SHIFT, 0);
        else
            SoundPlay(7 - SOUND_ID_SHIFT, 0);
    }
    UpdateSelectWindow();
    if (gGameState.unk1EE != 0)
        return;
    gMain.bgOfs[0][0] = 0;
    if (gGameState.unk1FA)
    {
        if (gGameState.unk14B == 3 && gGameState.flags.unk3_1)
        {
            if (gGameState.flags.unk3_2)
                gGameState.unk13E = 0x12B;
            else
                gGameState.unk13E++;
        }
        if (gGameState.unk14B == 3)
        {
            if (gGameState.unk156 != 0)
            {
                gGameState.unk156--;
                SoundPlayLoop(6, 6, 0);
                if (gGameState.unk156 == 0)
                    gGameState.unk13E = 300;
            }
            else if (!gGameState.flags.unk3_2 && gGameState.unk12B != 1)
            {
                if ((gGameState.unk224 == 0 && (gMain.newKeys & 0x30F) && gGameState.unk142 != 0x50)
                 || gGameState.unk13E > 0x12B)
                {
                    gGameState.unk13E = 0;
                    if ((u16)(gGameState.unk142 - 0x15) <= 2)
                    {
                        if (gGameState.markers[gGameState.unk119].type == 7)
                            Mode1SendSwitch(3);
                        else
                            Func_0201B7D4();
                        SoundPlay(0x21 - SOUND_ID_SHIFT, 0);
                        gMain.newKeys &= 0xF0;
                    }
                    else if ((u16)(gGameState.unk142 - 0xE6) <= 4)
                    {
                        ShowMessage(gGameState.unk1E3 + 0xEA, 1);
                        gMain.newKeys &= 0xF0;
                    }
                    else if (gGameState.unk1F8 != -1)
                    {
                        ShowMessage(gGameState.unk1F8, 1);
                        gGameState.unk1F8 = 0xFFFF;
                        gMain.newKeys &= 0xF0;
                    }
                    else
                    {
                        if (gGameState.unk11A == 2)
                        {
                            u8 t = gGameState.unk11B;

                            if (t == 5)
                                gGameState.unk010 |= 1 << gGameState.markers[gGameState.unk119].unk6;
                            else if (t == 13)
                                gGameState.unk11D |= 1;
                            else if (t == 12)
                                gGameState.unk11C |= 1;
                            else if (t == 16)
                                gGameState.unk026 |= 1 << gGameState.markers[gGameState.unk119].unk6;
                            gGameState.unk11A = 1;
                        }
                        MsgClose();
                    }
                }
            }
        }
    }
    Func_0201CFE8();
    if (gGameState.unk138_0)
    {
        UpdateMap();
    }
    else
    {
        u16 target = (gGameState.unk1CC | gGameState.unk224) ? -4 : 0x7C;

        if (StepToward((s16 *)&gMain.bgOfs[1][0], target, 16))
        {
            gGameState.unk1E5_4 = 0;
            if ((gMain.newKeys & 8) && !gGameState.flags.unkA_3 && gGameState.unk224 == 0)
            {
                if (gGameState.unk1FA == 0)
                {
                    if (gGameState.unk1E5_1 && gGameState.unk1E0 == 0
                     && !(gGameState.unk1CC && gGameState.unk1E5_0))
                    {
                        gGameState.unk1CC ^= 1;
                        gGameState.unk223 = 0;
                        if (gGameState.unk1CC == 0)
                        {
                            SoundPlay(0x2A - SOUND_ID_SHIFT, 0);
                            gGameState.unk1E5_4 = 1;
                            gGameState.unk13C = 0;
                            gGameState.unk104 = 1;
                        }
                        else
                        {
                            gMain.bgOfs[1][1] = -4;
                            SoundPlay(0x2B - SOUND_ID_SHIFT, gMain.unk40C);
                            if (gGameState.unk21C != 0)
                                gGameState.unk21C = 0;
                            gGameState.unk1FA = 0;
                            gGameState.unk226 = 1;
                            gGameState.unk227 = 0;
                            gGameState.unk228 = 0;
                        }
                    }
                }
                else if (gGameState.unk10E)
                {
                    UpdateMap();
                }
            }
            else if (gGameState.unk1CC == 0)
            {
                if (gGameState.unk10E)
                    UpdateMap();
                // (Through a u32 local: tested directly, agbcc copies the 2 before the and.)
                keys = gMain.newKeys;
                if ((keys & 2) && gGameState.unk1E5_1 && gGameState.unk104
                 && gGameState.unk220 == 0 && gGameState.unk121 == 0)
                    UseItem(gGameState.unk1DE);
                if (!gMain.unk457)
                    Func_0201B0F4();
                if (gGameState.unk224 == 0)
                {
                    if (gMain.newKeys & 1)
                    {
                        if (!gGameState.flags.busy && gGameState.unk1E5_1 && gGameState.unk104)
                        {
                            if (gGameState.unk1FA == 0)
                            {
                                if (gGameState.unk0FC == 0)
                                    Func_0201B8C8();
                                else if (gGameState.unk0FC == 1)
                                    UseItem(0x10);
                            }
                        }
                        else if (gGameState.unk1FA == 0)
                        {
                            SoundPlay(SE_KEY_BLOCKED, 0);
                        }
                    }
                    if (gGameState.unk11E)
                    {
                        u8 i = FindMarker(3);

                        gGameState.icons[2].type = 3;
                        gGameState.icons[2].x = gGameState.markers[i].x;
                        gGameState.icons[2].y = gGameState.markers[i].y;
                        SoundPlay(0xB - SOUND_ID_SHIFT, 0);
                        gGameState.unk11E = 0;
                    }
                }
                if ((gMain.newKeys & 0x100) && !gGameState.flags.busy && gGameState.unk1E5_1
                 && gGameState.unk104 && gMain.linked)
                {
                    gGameState.unk222 ^= 1;
                    if (gGameState.unk222)
                        gGameState.unk019 = 0;
                }
            }
            else
            {
                ItemMenuUpdate();
                Func_0201A540();
            }
        }
        else
        {
            gGameState.unk1E5_4 = 1;
        }
    }
    DrawStatusIcon();
}
// Fade out: step the mosaic and brightness up every other frame while BG1 slides to
// its target, then go to state 4.
void Mode1State3(void)
{
    u16 target = gGameState.unk1CC ? -4 : 0x7C;

    if (gGameState.unk1FB)
    {
        UpdateSelectWindow();
        return;
    }
    if (--gGameState.fadeTimer == 0)
    {
        gMain.mosaic += 0x1111;
        gMain.bldy = (gMain.mosaic & 0xF) >> 1;
        if (gMain.mosaic == 0xFFFF)
        {
            if (gMain.bgcnt[0] & 0x40)
                gMain.bgcnt[0] ^= 0x40;
            if (gMain.bgcnt[1] & 0x40)
                gMain.bgcnt[1] ^= 0x40;
            if (gMain.bgcnt[2] & 0x40)
                gMain.bgcnt[2] ^= 0x40;
            if (gMain.bgcnt[3] & 0x40)
                gMain.bgcnt[3] ^= 0x40;
            if (!gMain.unk457)
                gMain.dispcnt &= ~0x0C00;  // BG2 and BG3 off
            else
                gMain.dispcnt &= ~0x0800;  // BG3 off
            gGameState.unk23C = 0;
            gGameState.unk237 = 20;
            gMain.modeState = 4;
        }
        else
        {
            gGameState.fadeTimer = 2;
        }
    }
    UpdateMap();
    Func_0201CFE8();
    StepToward((s16 *)&gMain.bgOfs[1][0], target, 16);
}
// Slot 3: the map markers. Byte 0 is the count, byte 1 goes to unk133 and unk104, then
// 8-byte markers (struct MapMarker, unaligned here so read as bytes).
// The US builds check the new marker position instead of the old one.
void Mode1RecvMarkers(void)
{
    u8 count = GBACOM_BUF_3[0];
    u8 *m;
    s32 i;
    s32 j;
#ifndef VERSION_GZLJ01
    s16 time;
    u16 time2;
#endif

    if (gMain.mode == 1)
    {
        if (GBACOM_BUF_3[1] != 0)
        {
            // Both searches' results are unused. (do/while (1): a for or while loop
            // gets its exit test copied to the bottom, the original's doesn't.)
            m = GBACOM_BUF_3 + 2;
            j = 0;
            do
            {
                if ((m[5] & 0x3F) == 3 || j >= count)
                    break;
                j++;
                m += 8;
            } while (1);
            i = 0;
            do
            {
                if (gGameState.markers[i].type == 3 || i >= gGameState.markerCount)
                    break;
                i++;
            } while (1);
        }
        if (!gMain.unk457)
        {
            s16 x;

            m = GBACOM_BUF_3 + 2;
            j = 0;
            do
            {
                if ((m[5] & 0x3F) == 1 || j >= count)
                    break;
                j++;
                m += 8;
            } while (1);
            i = 0;
            x = *(s16 *)m;
            do
            {
                if (gGameState.markers[i].type == 1 || i >= gGameState.markerCount)
                    break;
                i++;
            } while (1);
            if ((gGameState.markers[i].x >> 8) != (x >> 8)
             || (gGameState.markers[i].y >> 8) != (*(s16 *)(m + 2) >> 8))
            {
#ifdef VERSION_GZLJ01
                if (gGameState.markers[i].x <= 0x6FF && gGameState.markers[i].y <= 0x6FF)
                {
                    gGameState.unk106 = 1;
                    gGameState.unk030 = (gMain.random % 6 + 3) * 60;
                }
#else
                if ((u16)x <= 0x6FF && *(u16 *)(m + 2) <= 0x6FF)
                {
                    gGameState.unk106 = 1;
                    // FAKEMATCH: the two temporaries lengthen gGameState's live range just
                    // enough that m gets r5 and gGameState r6 (a tie in allocation priority).
                    time = (gMain.random % 6 + 3) * 60;
                    time2 = time;
                    gGameState.unk030 = time2;
                }
#endif
                gGameState.unk026 = 0;
                gGameState.unk11D = 0;
                gGameState.unk010 = 0;
                gGameState.unk11B = 0;
                gGameState.unk11A = 0;
                if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
                    GbaComDataStatusReset(GBACOM_SLOT_MAIL);
                gGameState.switchSound = 0xFFFF;
                gGameState.switchType = 0;
            }
        }
    }
    gGameState.markerCount = GBACOM_BUF_3[0];
    gGameState.unk133 = GBACOM_BUF_3[1];
    if (gGameState.flags.busy)
        gGameState.unk104 = 0;
    else if (gGameState.unk133 == 0)
    {
        if (*(u16 *)&gGameState.unk1FA == 0 && gGameState.unk224 == 0 && gGameState.unk1CC == 0)
            gGameState.unk104 = 1;
        else
            gGameState.unk104 = GBACOM_BUF_3[1];
    }
    else
    {
        gGameState.unk104 = gGameState.unk133;
    }
    if (gGameState.flags.isFree || gGameState.unk11A)
    {
        // The first two markers swap places.
        CpuCopy16(&GBACOM_BUF_3[2], &gGameState.markers[1], 8);
        CpuCopy16(&GBACOM_BUF_3[10], &gGameState.markers[0], 8);
        if (gGameState.markerCount > 2)
            CpuCopy16(&GBACOM_BUF_3[18], &gGameState.markers[2], (gGameState.markerCount - 2) * 8);
    }
    else
    {
        CpuCopy16(&GBACOM_BUF_3[2], gGameState.markers, gGameState.markerCount * 8);
    }
    GbaComDataStatusReset(GBACOM_SLOT_3);
}

// Reset the per-map state (Mode1State4 after a map change). The object loop counts up: agbcc
// reverses it into the original's pointer countdown, a countdown in C keeps the counter.
void Mode1Func_02016350(void)
{
    s32 i;

    gGameState.unk11B = 0;
    gGameState.unk11A = 0;
    gGameState.unk10E = 1;
    gGameState.unk11D = 0;
    gGameState.unk010 = 0;
    gGameState.unk11C = 0;
    gGameState.unk222 = 1;
    gMain.unk404 = 0;
    gGameState.unk106 = 0;
    gGameState.unk030 = 0;
    gGameState.unk120 = 0;
    gGameState.unk15A = 0;
    gGameState.switchSound = 0xFFFF;
    gGameState.switchType = 0;
    gGameState.unk140 = -1;
    gGameState.unk1F8 = -1;
    gGameState.unk0FD = 0;
    gGameState.unk136 = 0;
    gGameState.unk1E0 = 0;
    for (i = 0; i < 16; i++)
        gObjs[i].active = 0;
    if (!gGameState.unk1E5_1)
    {
        gGameState.unk1E5_1 = 1;
        gGameState.unk1E5_2 = 0;
        gGameState.unk12B = 0;
    }
    InitMapView();
}
// Wait for the GC to switch maps (slots 1-3), then load the new one and fade in (state 5).
void Mode1State4(void)
{
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAP_IMAGE))
    {
        gGameState.unk10E = 1;
        gGameState.unk237 = 20;
        LoadMapImage();
    }
    if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
    {
        GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
        if (gGameState.unk10E)
            gGameState.unk10E = 0;
        gGameState.unk237 = 20;
    }
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
        GbaComDataStatusReset(GBACOM_SLOT_MAIL);
    if (!gGameState.flags.unkA_0)
    {
        if (gMain.newKeys)
            SoundPlay(SE_KEY_BLOCKED, 0);
        if (gGameState.flags.unkA_3)
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                Mode1RecvMarkers();
        }
        else if (!(gGameState.flags.busy & 2))  // no event running
        {
            if (!gMain.inDungeon)
            {
                if (GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_IDLE)
                 || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVED))
                {
                    if (gGameState.unk237 != 0)
                    {
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                            Mode1RecvMarkers();
                        gGameState.unk237--;
                    }
                    else if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                    {
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_MAP_IMAGE))
                            GbaComDataStatusReset(GBACOM_SLOT_MAP_IMAGE);
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
                            GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
                        gMain.unk457 = gMain.inDungeon;
                        ItemMenuInit();
                        Mode1Func_0201521C();
                        Mode1RecvMarkers();
                        Mode1Func_02016350();
                        gMain.bgcnt[2] |= 0x40;
                        gMain.bgcnt[3] |= 0x40;
                        gMain.mosaic = 0xFFFF;
                        gMain.dispcnt = 0x1F00;
                        gGameState.fadeTimer = 2;
                        gMain.modeState = 5;
                    }
                }
            }
            else if (gGameState.unk10E)
            {
                if (gGameState.unk237 != 0)
                {
                    if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                        Mode1RecvMarkers();
                    gGameState.unk237--;
                }
                else if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                {
                    Mode1RecvMarkers();
                    gMain.unk457 = gMain.inDungeon;
                    ItemMenuInit();
                    Mode1Func_0201521C();
                    Mode1Func_02016350();
                    gMain.bgcnt[3] |= 0x40;
                    gMain.mosaic = 0xFFFF;
                    gMain.dispcnt = 0x1B00;
                    gGameState.fadeTimer = 2;
                    gMain.modeState = 5;
                }
            }
        }
    }
    DrawSprite(gSprite_0202C438, 0, 120, 80, 0, 0, 0, 0, 0, 0, 0);
    Func_0201CFE8();
}
void Mode1StartFadeOut(void)
{
    ObjResetActive();
    gGameState.unk10E = 0;
    gMain.bgcnt[2] |= 0x40;  // mosaic
    gMain.bgcnt[3] |= 0x40;
    gMain.mosaic = 0;
    gMain.bldcnt = 0x2FEF;  // darken everything
    gMain.bldy = 0;
    gGameState.fadeTimer = 2;
    gMain.modeState = 3;
}
// Per frame in mode 1: react to what the GC's flags (slot 9) changed (chests, life, the
// clock), run the current state, handle the link slots (1/2 map change, 3 markers, 5 mail,
// 7 item use result, 11, 15 SE) and redraw the rupee counter and icon.
void MainMode1(void)
{
    s32 i;
    s32 chest = 0;

    UpdateItemEffect();

    // A dungeon chest was opened: count them and play the newest one's sound.
    if (gGameState.dungeonChests != gGameState.flags.dungeonChests && gGameState.unk1E0 == 0)
    {
        gGameState.unk1E1 = gGameState.flags.dungeonChests ^ gGameState.dungeonChests;
        gGameState.unk1E0 = 1;
        gMain.bgOfs[1][1] = -4;
    }
#ifdef VERSION_GZLJ01
    if (gGameState.unk1E0 == 1 && gGameState.unk1FA == 0 && gGameState.unk104 != 0)
#else
    // The US builds wait for no menu or event instead. FAKEMATCH: `!flags.busy` gives the
    // same code but puts chest in r8 (the extra insns before combine shift the priorities).
    if (gGameState.unk1E0 == 1 && !*(u16 *)&gGameState.unk1FA && !((gGameState.flags.busy << 1) & 6))
#endif
    {
        gGameState.unk1CA = 0;
        gGameState.unk1CC = 1;
        gGameState.unk1FA = 0;
        gGameState.unk226 = 0;
        gGameState.unk227 = 0;
        gGameState.unk228 = 0;
        gGameState.unk1E0 += 1;
        gGameState.unk1E3 = 0;
        for (i = 0; i <= 4; i++)
        {
            if (gGameState.flags.dungeonChests & (1 << i))
                gGameState.unk1E3++;
            if (gGameState.unk1E1 & (1 << i))
                chest = i;
        }
        ShowMessage(gChestSounds[chest], 1);
    }

    if (gGameState.unk238 != gGameState.flags.unkB_7)
        gGameState.unk239 = 1;
    if (gGameState.unk239 && gGameState.unk104 && gMain.modeState == 2 && gGameState.unk1FA == 0)
    {
        gGameState.unk239 = 0;
        ShowMessage(0x10, 1);
        SetSlotItem(4, 5);
        SetSlotItem(8, 9);
        SetSlotItem(9, 10);
    }

    if (gMain.modeState == 2)
    {
        // Life: a sound when it drops to 7 or below (once), Func_0201C484(15) on other drops.
        if (gGameState.unk1FA == 0 && gGameState.unk1CC == 0)
        {
            if (gGameState.flags.unkA_3)
                ShowMessage(0x110, 1);
            else if (gGameState.life > gGameState.flags.life)
            {
                if (gGameState.flags.life <= 7)
                {
                    if (!gGameState.lowLifeWarned)
                    {
                        gGameState.lowLifeWarned = 1;
                        gGameState.life = gGameState.flags.life;
                        ShowMessage(0x10F, 1);
                    }
                }
                else
                {
                    gGameState.life = gGameState.flags.life;
                    Func_0201C484(15);
                }
            }
            else if (gGameState.life < gGameState.flags.life)
            {
                if (gGameState.flags.life > 7)
                    gGameState.lowLifeWarned = 0;
                gGameState.life = gGameState.flags.life;
            }
        }
        else if ((u16)(gGameState.unk142 - 0x10E) <= 2)
        {
            gGameState.life = gGameState.flags.life;
            if (gGameState.flags.life > 7)
                gGameState.lowLifeWarned = 0;
        }

        // On the sea: a clock sound at 6:00, 12:00 and 20:00 that depends on the weekday.
        if (gGameState.flags.dungeonNo == 0 && gGameState.unk1FA == 0 && gGameState.unk1CC == 0
         && gGameState.unk224 == 0)
        {
            if (!gGameState.chimed)
            {
                if (gGameState.flags.hour == 6)
                {
                    if (gGameState.flags.dayOfWeek == 0)
                        ShowMessage(0x145, 1);
                    else if (gGameState.flags.dayOfWeek == 1 || gGameState.flags.dayOfWeek == 3
                          || gGameState.flags.dayOfWeek == 5)
                        ShowMessage(0x146, 1);
                    else
                        ShowMessage(0x147, 1);
                    gGameState.chimed = 1;
                }
                else if (gGameState.flags.hour == 12)
                {
                    if (gGameState.flags.dayOfWeek == 0 || gGameState.flags.dayOfWeek == 6)
                        ShowMessage(0x148, 1);
                    else if (gGameState.flags.dayOfWeek == 1 || gGameState.flags.dayOfWeek == 3
                          || gGameState.flags.dayOfWeek == 5)
                        ShowMessage(0x149, 1);
                    else
                        ShowMessage(0x14A, 1);
                    gGameState.chimed = 1;
                }
                else if (gGameState.flags.hour == 20)
                {
                    if (gGameState.flags.dayOfWeek == 6)
                    {
                        ShowMessage(0x14B, 1);
                        gGameState.chimed = 1;
                    }
                    else if (gGameState.flags.dayOfWeek == 0 || gGameState.flags.dayOfWeek == 2
                          || gGameState.flags.dayOfWeek == 4)
                    {
                        ShowMessage(0x14C, 1);
                        gGameState.chimed = 1;
                    }
                    else if (gGameState.flags.dayOfWeek == 3 || gGameState.flags.dayOfWeek == 5)
                    {
                        ShowMessage(0x14D, 1);
                        gGameState.chimed = 1;
                    }
                    else
                    {
                        gGameState.chimed = 0;
                    }
                }
            }
            else if (gGameState.flags.hour != 6 && gGameState.flags.hour != 12
                  && gGameState.flags.hour != 20)
            {
                gGameState.chimed = 0;
            }
        }
    }

    // Map change: fade out (state 3); Mode1State4 waits for the new map.
    if (gGameState.flags.unkA_0 || gMain.inDungeon != gMain.unk457
     || GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL)
     || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVING)
     || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVED)
     || !gGameState.unk10E)
    {
        if (gMain.modeState == 2)
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
                GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
            Mode1StartFadeOut();
            gGameState.unk10E = 0;
            gGameState.unk224 = 0;
            gGameState.unk235 = gMain.unk457;
            if (gGameState.unk1CC || gGameState.unk1FA)
            {
                MsgClose();
                gGameState.unk142 = 0xFFFF;
                if (gGameState.unk1FA)
                    gGameState.unk21C = 0;
                if (gGameState.unk1CC)
                    gGameState.unk1CC = 0;
            }
            if (GbaComRecvStatusCheck(GBACOM_SLOT_3))
                GbaComDataStatusReset(GBACOM_SLOT_3);
        }
    }
    else if (gMain.modeState != 4 && GbaComRecvStatusCheck(GBACOM_SLOT_3))
    {
        Mode1RecvMarkers();
    }

    if (GbaComRecvStatusCheck(GBACOM_SLOT_SE))
    {
        Func_0201C484(GBACOM_BUF_SE[0] - 1);
        GbaComDataStatusReset(GBACOM_SLOT_SE);
    }

    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
    {
        if (gMain.modeState == 2)
        {
            if (GBACOM_BUF_MAIL[7])
                Func_0201C484(GBACOM_BUF_MAIL[7] - 1);
            if (gGameState.unk1FA == 0 && gGameState.unk1CC == 0 && gGameState.unk224 == 0
             && gGameState.unk1FB == 0)
            {
                GbaComRecvDataGet(&gGameState.switchSound, GBACOM_SLOT_MAIL, 8);
                if (gGameState.switchType == 5 || gGameState.switchType == 6
                 || gGameState.switchType == 9 || gGameState.switchType == 7
                 || gGameState.switchType == 11 || gGameState.switchType == 12
                 || (gGameState.switchType == 13 && !(((u16 *)&gGameState.flags)[5] & 0x180)))
                {
                    if (gGameState.switchType == 12)
                    {
                        if (gGameState.unk121 == 0)
                        {
                            if (!gGameState.flags.unk5_5)
                                gGameState.flags.unk5_5 = 1;
                            gGameState.unk121 = 1;
                            gGameState.unk122 = 0;
                            gGameState.unk124 = 0;
                            gGameState.unk123 = gSprite_0202CA08[4];
                        }
                        else if (gGameState.unk121 == 1)
                        {
                            gGameState.unk121 = 2;
                            gGameState.unk124 = 0;
                            gGameState.unk123 = gSprite_0202CA40[4];
                            ShowMessage(0x5B, 0);
                        }
                    }
                    if (gGameState.unk121 <= 1)
                    {
                        Mode1SendSwitch(0);
                        gGameState.unk0E5 = 0;
                    }
                }
                else if (gGameState.switchType == 0)
                {
                    gGameState.unk0E5 = 0;
                }
                else
                {
                    gGameState.unk042 = 0;
                }
            }
            else if (GBACOM_BUF_MAIL[5] == 0)
            {
                GbaComRecvDataGet(&gGameState.switchSound, GBACOM_SLOT_MAIL, 8);
                gGameState.unk0E5 = 0;
            }
            else
            {
                GbaComDataStatusReset(GBACOM_SLOT_MAIL);
            }
        }
        else
        {
            GbaComDataStatusReset(GBACOM_SLOT_MAIL);
        }
    }

    if (GbaComRecvDataGet(gGameState.effect, GBACOM_SLOT_EFFECT, 4))
    {
        if (gGameState.effect[0] == 0)
        {
            s32 result = gGameState.effect[1];

            if (result == 0)
                SoundPlay(SE_KEY_BLOCKED, 0);
            else if (result == 1)
            {
                ShowMessage(0xF, 1);
                SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
                gGameState.unk21C = result;
            }
            else if (result == 2)
                ShowMessage(0xE, 1);
            else if (result == 3)
                ShowMessage((gGameState.effect[3] << 8) + gGameState.effect[2], 1);
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
        else
        {
            GbaComRecvDataGet(gGameState.effect, GBACOM_SLOT_EFFECT, 4);
            gGameState.unk1E5_1 = 0;
            gGameState.unk1E5_2 = 1;
            if (gGameState.effect[0] == 2 || gGameState.effect[0] == 15 || gGameState.effect[0] == 21)
            {
                gGameState.effect[0] = 2;
                Func_0201EBD4();
                SoundPlay(0x23 - SOUND_ID_SHIFT, 0);
            }
            else if (gGameState.effect[0] == 16)
            {
                Func_0201F274();
                SoundPlay(0x23 - SOUND_ID_SHIFT, 0);
                if (gGameState.unk121)
                {
                    ShowMessage(gGameState.unk122 + 0x60, 1);
                    if (gGameState.unk122 <= 2)
                        gGameState.unk122++;
                }
            }
            else if (gGameState.effect[0] == 3)
                ShowMessage(0x24, 1);
            else if (gGameState.effect[0] == 4)
                ShowMessage(0x26, 1);
            else if (gGameState.effect[0] == 5)
                Func_0201F2C4();
            else if (gGameState.effect[0] == 7)
                Func_0201F420();
            else if (gGameState.effect[0] == 8 || gGameState.effect[0] == 9 || gGameState.effect[0] == 10)
                gGameState.unk12B = 7;
            else if (gGameState.effect[0] == 13)
                ShowMessage(0x26, 1);
            else if (gGameState.effect[0] == 14)
                Func_02020150();
            else if (gGameState.effect[0] == 17)
            {
                gGameState.effect[0] = 3;
                Func_0201EDC4();
            }
            else if (gGameState.effect[0] == 18)
            {
                gGameState.effect[0] = 4;
                Func_0201F240();
            }
            else if (gGameState.effect[0] == 20)
                gGameState.effect[0] = gGameState.unk1E4;
            else if (gGameState.effect[0] == 19)
            {
                ShowMessage(0x2D, 1);
                gGameState.effect[0] = 5;
                gGameState.unk12B = 6;
            }
            else
                gGameState.unk1C8 = 0;
        }
    }

    // Slot 11: 8 bytes to unk0F4.
    if (GbaComRecvStatusCheck(GBACOM_SLOT_11))
    {
        CpuCopy16(GBACOM_BUF_11, gGameState.unk0F4, 8);
        GbaComDataStatusReset(GBACOM_SLOT_11);
    }

    if (gMain.modeState == 2)
    {
        gGameState.unk119 = Func_0201B3A4();
        if (gGameState.unk1FA == 0 && gGameState.unk1CC == 0 && gGameState.unk1E5_1)
        {
            if (gGameState.unk11A && gGameState.unk224 == 0)
            {
                if (gGameState.unk226 != 11)
                {
                    gGameState.unk226 = 11;
                    Func_0201C568();
                }
            }
            else if (gGameState.unk226)
            {
                gGameState.unk226 = 0;
                Func_0201C568();
            }
        }
        // Darken the screen and show the wait sprite while a menu or event runs on the GC.
        if ((gGameState.unk1FB | gGameState.unk1FA | gGameState.unk224 | gGameState.unk1CC) == 0
         && gGameState.flags.busy)
        {
            gMain.bldcnt = 0x2FEF;
            gMain.bldy = 7;
            DrawSprite(gSprite_0202C438, 0, 120, 80, 0, 0, 0, 0, 0, 0, 0);
        }
        else
        {
            gMain.bldcnt = 0;
            gMain.bldy = 0;
        }
    }

    gMode1StateFuncs[gMain.modeState]();
    UpdateHudPanels();
    if (gMain.modeState != 0)
    {
        u16 *map;

        Mode1Func_020150A4();
        map = (u16 *)0x0600644C;
        map[0] = gRupeeIconTiles[gGameState.flags.walletSize];
        map[0x20] = gRupeeIconTiles[gGameState.flags.walletSize] + 1;
    }

    // Some seconds after a marker update (Mode1RecvMarkers), a sound for the chart square
    // that marker 1 (Link?) is in.
    if (gMain.modeState == 2 && gGameState.unk106)
    {
        if (gGameState.unk030 != 0)
            gGameState.unk030--;
        else if (gGameState.unk1CC == 0 && *(u16 *)&gGameState.unk1FA == 0 && gGameState.unk224 == 0)
        {
            gGameState.unk106 = 0;
            i = 0;
            do
            {
                if (gGameState.markers[i].type == 1 || i >= gGameState.markerCount)
                    break;
                i++;
            } while (1);
            // The square (7 per row) goes into i too: a separate local shifts the registers.
            i = (gGameState.markers[i].y >> 8) * 7 + (gGameState.markers[i].x >> 8);
            if (i == 25 && gGameState.flags.unkA_4)
                ShowMessage(0x437, 1);
            else
                ShowMessage(i + 0x406, 1);
        }
    }
}
