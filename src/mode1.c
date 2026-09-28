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
void ResetMapState(void);

// Not referenced by any code. It ends in 0xFF, like a short text in the message font's glyph
// codes (see MsgStep).
const u8 gMode1Unused_02023E38[12] = {0x7D, 0x7A, 0x01, 0x00, 0xB2, 0xB3, 0x00, 0x2B, 0x00, 0x00, 0x00, 0xFF};

// Message per dungeon number (gGameState.flags.dungeonNo), shown by Mode1State1.
const s16 gDungeonMsgIds[18] = {50, 51, 52, 53, 54, 55, 82, 83, 84, 85, 86, 86, 138, 142, 135, 137, 141, 57};

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
// Message per dungeon chest bit (flags.dungeonChests: WT, ET, TOTG, FW, DRC).
const s16 gChestMsgIds[5] = {0xE6, 0xE7, 0xE8, 0xEA, 0xE9};

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
void UpdateRupeeCounter(void)
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
            {
#ifdef SE_RUPEE_COUNT_END
                SoundPlay(SE_RUPEE_COUNT_END, 0);
#endif
            }
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

// Set up the map BGs: on the sea the sea chart (BG2, gMode1SeaMap) and the floor number box,
// in a dungeon BG3 for the map image.
void InitMapBgs(void)
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
#ifdef VERSION_PAL
        CpuCopy16((void *)0x06007AC0, (void *)0x06006840, 0x80);
#endif
        gMain.bgcnt[3] = 0x0B41;
    }
}

// Load mode 1's OBJ and BG tiles, palettes and BG map, then the map BGs.
void LoadMode1Graphics(void)
{
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1BgMap, (void *)0x06006000);
    InitMapBgs();
}
// Set up the screen and the game state, then start the intro wave (state 1).
// squareMsgPending is cleared twice, like msgLoopSound and nextMsg: flow deletes the first
// store as dead, but its address (computed there, into r6) is what the second one uses.
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
    gGameState.aButtonAction = 1;
    gGameState.hudLeftSlide = 0;
    gGameState.hudRightSlide = 0;
    gGameState.squareMsgPending = 0;
    gGameState.targetDir = 0xFF;
    gGameState.hudLeftHidden = 0;
    gGameState.hudRightHidden = 0;
    gGameState.dungeonChests = gGameState.flags.dungeonChests;
#ifdef VERSION_GZLJ01
    gGameState.glyphWidth = 8;
#else
    gGameState.glyphWidth = 6;
#endif
    gGameState.hudChartLabel = 0;
    gGameState.msgLoopSound = 0xFFFF;
    gGameState.nextMsg = -1;
    gGameState.holdState = 0;
    gGameState.selectedType = 0;
    gGameState.selection = 0;
    gGameState.revealedType13 = 0;
    gGameState.revealedType5 = 0;
    gGameState.revealedType12 = 0;
    gGameState.squareMsgPending = 0;
    gGameState.squareMsgTimer = 0;
    gGameState.viewLocked = 0;
    gGameState.switchMsg = -1;
    gGameState.switchType = 0;
    gGameState.msgLoopSound = -1;
    gGameState.nextMsg = -1;
    gGameState.awayFromType20 = 0;
    gGameState.chestState = 0;
    for (i = 0; i < 16; i++)
        gObjs[i].active = 0;
    gGameState.type9Timer = gSpriteTable[9][4];
    gGameState.type9Frame = 0;
    gGameState.type13Timer = gSpriteTable[13][4];
    gGameState.type13Frame = 0;
    gGameState.type16Timer = gSpriteTable[16][4];
    gGameState.type16Frame = 0;
    gGameState.friendTimer = gSpriteTable[19][4];
    gGameState.friendFrame = 0;
    LoadMode1Graphics();
    if (gGameState.flags.noMap)
    {
        gMain.mapImageMode = 1;
        gGameState.mapLoaded = 0;
    }
    else
    {
        gMain.mapImageMode = gMain.inDungeon;
        gGameState.mapLoaded = 1;
    }
    ResetSelectWindow();
    InitMapView();
    if (gGameState.flags.dungeonNo != 0)
    {
        gGameState.playerBlinkTimer = 0;
        gGameState.enemyBlinkTimer = 0;
        gGameState.cursorBlinkTimer = 0;
        if (gGameState.mapLoaded)
            UpdateMap();
    }
    else
    {
        UpdateMap();
    }
    MsgReset();
    gGameState.switchMsg = 0xFFFF;
    gGameState.unk21C = 0;
    gGameState.tingleScale = 0x300;
    ItemMenuInit();
    gGameState.itemsEnabled = 1;
    gGameState.effectStep = 0;
    gGameState.inMessage = 0;
    gGameState.selectWinOpen = 0;
    gGameState.chartOpen = 0;
    gGameState.rupeeDelta = 0;
    UpdateRupeeCounter();
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
        ShowMessage(gDungeonMsgIds[gGameState.flags.dungeonNo], 2);
        SoundPlay(0, 0);
        gGameState.unk21C = 1;
        gMain.modeState = 2;
        if (gGameState.flags.dungeonNo == 0)
        {
            gMain.dispcnt = 0x1F00;  // BG0-3 and OBJ
            gGameState.afterLinkLost = 0;
        }
        else if (gGameState.afterLinkLost == 0)
        {
            gMain.dispcnt = 0x1B00;  // BG0, BG1, BG3, OBJ
        }
        else
        {
            gMain.dispcnt = 0x1300;  // BG0, BG1, OBJ
        }
    }
    UpdateMapView();
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
            if (gGameState.holdState)
            {
                ShowMessage(0x63, 1);
                gGameState.holdState = 0;
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
    UpdateMsgAndTingle();
}
// Send the switch packet (5 bytes and a checksum of its two words) and show its message.
void Mode1SendSwitch(u8 soundArg)
{
    GBACOM_BUF_SWITCH[0] = gGameState.switchNo;
    GBACOM_BUF_SWITCH[1] = gGameState.switchStageNo;
    GBACOM_BUF_SWITCH[2] = gGameState.switchRoomNo;
    GBACOM_BUF_SWITCH[3] = gGameState.switchType;
    GBACOM_BUF_SWITCH[4] = gGameState.switchNo2;
    GbaComSendDataSet(GBACOM_BUF_SWITCH, 6, GBACOM_SLOT_SWITCH,
                      ((u32 *)GBACOM_BUF_SWITCH)[0] + ((u32 *)GBACOM_BUF_SWITCH)[1]);
    ShowMessage(gGameState.switchMsg, soundArg);
    gGameState.unk21C = 1;
    gGameState.tingleScale = 0x100;
    if ((u8)(gGameState.switchType - 1) > 2 && gGameState.switchType != 14 && gGameState.switchType != 15)
    {
        gGameState.unk042 = 0;
        gGameState.switchType = 0;
    }
}
// The main state: sounds for the GC's flags, the timed marker/switch action (slot 3 marker
// type 7 sends a switch), the BG1 slide, then the START/A/B/R keys.
void Mode1State2(void)
{
    u32 keys;

    if (gGameState.flags.cursorInZone && gGameState.menuOpen == 0 && gGameState.selectWinStep == 0
     && gGameState.chartOpen == 0)
        ShowMessage(0x51, 0xF);
    if (gGameState.flags.moveSpeed && !gGameState.flags.busy && gGameState.menuOpen == 0
     && gGameState.selectWinStep == 0)
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
            v = (gGameState.flags.moveSpeed - 1) ^ 3;
            params[1] = (params[1] & mask) | v;
            SoundPlayLoop(9, 9, params);
        }
    }
    if (gGameState.flags.isFree != gGameState.isFree)
    {
        if (gGameState.flags.isFree)
            SoundPlay(0x16 - SOUND_ID_SHIFT, 0);
        else
            SoundPlay(SE_FREE_OFF, 0);
    }
    UpdateSelectWindow();
    if (gGameState.selectWinStep != 0)
        return;
    gMain.bgOfs[0][0] = 0;
    if (gGameState.inMessage)
    {
        if (gGameState.msgStatus == 3 && gGameState.flags.gcInput)
        {
            if (gGameState.flags.cursorInZone)
                gGameState.msgWaitTimer = 0x12B;
            else
                gGameState.msgWaitTimer++;
        }
        if (gGameState.msgStatus == 3)
        {
            if (gGameState.reactionTimer != 0)
            {
                gGameState.reactionTimer--;
                SoundPlayLoop(6, 6, 0);
                if (gGameState.reactionTimer == 0)
                    gGameState.msgWaitTimer = 300;
            }
            else if (!gGameState.flags.cursorInZone && gGameState.effectStep != 1)
            {
                if ((gGameState.chartOpen == 0 && (gMain.newKeys & 0x30F) && gGameState.msgId != 0x50)
                 || gGameState.msgWaitTimer > 0x12B)
                {
                    gGameState.msgWaitTimer = 0;
                    if ((u16)(gGameState.msgId - 0x15) <= 2)
                    {
                        if (gGameState.markers[gGameState.selectedMarker].type == 7)
                            Mode1SendSwitch(3);
                        else
#ifdef VERSION_PAL
                            ShowMessage(gGameState.pendingMsg, 1);
#else
                            ShowSelectionMsg();
#endif
                        SoundPlay(0x21 - SOUND_ID_SHIFT, 0);
                        gMain.newKeys &= 0xF0;
                    }
                    else if ((u16)(gGameState.msgId - 0xE6) <= 4)
                    {
                        ShowMessage(gGameState.chestCount + 0xEA, 1);
                        gMain.newKeys &= 0xF0;
                    }
                    else if (gGameState.nextMsg != -1)
                    {
                        ShowMessage(gGameState.nextMsg, 1);
                        gGameState.nextMsg = 0xFFFF;
                        gMain.newKeys &= 0xF0;
                    }
                    else
                    {
#ifdef VERSION_PAL
                        if (gGameState.selection == 2 || gGameState.pendingReveal)
                        {
                            u8 t = gGameState.pendingType;

                            if (t == 5)
                                gGameState.revealedType5 |= 1 << gGameState.pendingParam;
                            else if (t == 13)
                                gGameState.revealedType13 |= 1;
                            else if (t == 12)
                                gGameState.revealedType12 |= 1;
                            else if (t == 16)
                                gGameState.revealedType16 |= 1 << gGameState.pendingParam;
                            gGameState.pendingReveal = 0;
                            gGameState.selection = 1;
                        }
#else
                        if (gGameState.selection == 2)
                        {
                            u8 t = gGameState.selectedType;

                            if (t == 5)
                                gGameState.revealedType5 |= 1 << gGameState.markers[gGameState.selectedMarker].param;
                            else if (t == 13)
                                gGameState.revealedType13 |= 1;
                            else if (t == 12)
                                gGameState.revealedType12 |= 1;
                            else if (t == 16)
                                gGameState.revealedType16 |= 1 << gGameState.markers[gGameState.selectedMarker].param;
                            gGameState.selection = 1;
                        }
#endif
                        MsgClose();
                    }
                }
            }
        }
    }
    UpdateMsgAndTingle();
    if (gGameState.unk138_0)
    {
        UpdateMap();
    }
    else
    {
        u16 target = (gGameState.menuOpen | gGameState.chartOpen) ? -4 : 0x7C;

        if (StepToward((s16 *)&gMain.bgOfs[1][0], target, 16))
        {
            gGameState.menuSliding = 0;
            if ((gMain.newKeys & 8) && !gGameState.flags.restarting && gGameState.chartOpen == 0)
            {
                if (gGameState.inMessage == 0)
                {
                    if (gGameState.itemsEnabled && gGameState.chestState == 0
                     && !(gGameState.menuOpen && gGameState.itemFlying))
                    {
                        gGameState.menuOpen ^= 1;
                        gGameState.unk223 = 0;
                        if (gGameState.menuOpen == 0)
                        {
                            SoundPlay(0x2A - SOUND_ID_SHIFT, 0);
                            gGameState.menuSliding = 1;
                            gGameState.msgOpen = 0;
                            gGameState.active = 1;
                        }
                        else
                        {
                            gMain.bgOfs[1][1] = -4;
                            SoundPlay(0x2B - SOUND_ID_SHIFT, gMain.soundParams);
                            if (gGameState.unk21C != 0)
                                gGameState.unk21C = 0;
                            gGameState.inMessage = 0;
                            gGameState.tingleAnim = 1;
                            gGameState.tingleAnimPart = 0;
                            gGameState.tingleFrame = 0;
                        }
                    }
                }
                else if (gGameState.mapLoaded)
                {
                    UpdateMap();
                }
            }
            else if (gGameState.menuOpen == 0)
            {
                if (gGameState.mapLoaded)
                    UpdateMap();
                // (Through a u32 local: tested directly, agbcc copies the 2 before the and.)
                keys = gMain.newKeys;
                if ((keys & 2) && gGameState.itemsEnabled && gGameState.active
                 && gGameState.hudRightHidden == 0 && gGameState.holdState == 0)
                    UseItem(gGameState.selectedItem);
                if (!gMain.mapImageMode)
                    UpdateChartOverlay();
                if (gGameState.chartOpen == 0)
                {
                    if (gMain.newKeys & 1)
                    {
                        if (!gGameState.flags.busy && gGameState.itemsEnabled && gGameState.active)
                        {
                            if (gGameState.inMessage == 0)
                            {
                                if (gGameState.aButtonAction == 0)
                                    ExamineSelection();
                                else if (gGameState.aButtonAction == 1)
                                    UseItem(0x10);
                            }
                        }
                        else if (gGameState.inMessage == 0)
                        {
                            SoundPlay(SE_KEY_BLOCKED, 0);
                        }
                    }
                    if (gGameState.placeTargetAtCursor)
                    {
                        u8 i = FindMarker(3);

                        gGameState.icons[2].type = 3;
                        gGameState.icons[2].x = gGameState.markers[i].x;
                        gGameState.icons[2].y = gGameState.markers[i].y;
                        SoundPlay(0xB - SOUND_ID_SHIFT, 0);
                        gGameState.placeTargetAtCursor = 0;
                    }
                }
                if ((gMain.newKeys & 0x100) && !gGameState.flags.busy && gGameState.itemsEnabled
                 && gGameState.active && gMain.linked)
                {
                    gGameState.rToggle ^= 1;
                    if (gGameState.rToggle)
                        gGameState.followStillCount = 0;
                }
            }
            else
            {
                ItemMenuUpdate();
                UpdateMapView();
            }
        }
        else
        {
            gGameState.menuSliding = 1;
        }
    }
    DrawStatusIcon();
}
// Fade out: step the mosaic and brightness up every other frame while BG1 slides to
// its target, then go to state 4.
void Mode1State3(void)
{
    u16 target = gGameState.menuOpen ? -4 : 0x7C;

    if (gGameState.selectWinOpen)
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
            if (!gMain.mapImageMode)
                gMain.dispcnt &= ~0x0C00;  // BG2 and BG3 off
            else
                gMain.dispcnt &= ~0x0800;  // BG3 off
            gGameState.afterLinkLost = 0;
            gGameState.markerDelay = 20;
            gMain.modeState = 4;
        }
        else
        {
            gGameState.fadeTimer = 2;
        }
    }
    UpdateMap();
    UpdateMsgAndTingle();
    StepToward((s16 *)&gMain.bgOfs[1][0], target, 16);
}
// Slot 3: the map markers. Byte 0 is the count, byte 1 goes to gcActive and active, then
// 8-byte markers (struct MapMarker, unaligned here so read as bytes).
// The US builds check the new marker position instead of the old one. (The gcActive flag's
// setcc insns, gone after combine, still lengthen gGameState's live range: without it the
// US build gives m and gGameState each other's registers.)
void Mode1RecvMarkers(void)
{
    u8 count = GBACOM_BUF_MARKERS[0];
    u8 *m;
    s32 i;
    s32 j;
    bool32 gcActive;

    if (gMain.mode == 1)
    {
        gcActive = GBACOM_BUF_MARKERS[1] != 0;
        if (gcActive)
        {
            // Both searches' results are unused. (do/while (1): a for or while loop
            // gets its exit test copied to the bottom, the original's doesn't.)
            m = GBACOM_BUF_MARKERS + 2;
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
        if (!gMain.mapImageMode)
        {
            s16 x;

            m = GBACOM_BUF_MARKERS + 2;
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
                    gGameState.squareMsgPending = 1;
                    gGameState.squareMsgTimer = (gMain.random % 6 + 3) * 60;
                }
#else
                if ((u16)x <= 0x6FF && *(u16 *)(m + 2) <= 0x6FF)
                {
                    gGameState.squareMsgPending = 1;
                    gGameState.squareMsgTimer = (gMain.random % 6 + 3) * 60;
                }
#endif
                gGameState.revealedType16 = 0;
                gGameState.revealedType13 = 0;
                gGameState.revealedType5 = 0;
                gGameState.selectedType = 0;
                gGameState.selection = 0;
                if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
                    GbaComDataStatusReset(GBACOM_SLOT_MAIL);
                gGameState.switchMsg = 0xFFFF;
                gGameState.switchType = 0;
            }
        }
    }
    gGameState.markerCount = GBACOM_BUF_MARKERS[0];
    gGameState.gcActive = GBACOM_BUF_MARKERS[1];
    if (gGameState.flags.busy)
        gGameState.active = 0;
    else if (gGameState.gcActive == 0)
    {
        if (gGameState.inMessage == 0 && gGameState.selectWinOpen == 0 && gGameState.chartOpen == 0
         && gGameState.menuOpen == 0)
            gGameState.active = 1;
        else
            gGameState.active = GBACOM_BUF_MARKERS[1];
    }
    else
    {
        gGameState.active = gGameState.gcActive;
    }
    if (gGameState.flags.isFree || gGameState.selection)
    {
        // The first two markers swap places.
        CpuCopy16(&GBACOM_BUF_MARKERS[2], &gGameState.markers[1], 8);
        CpuCopy16(&GBACOM_BUF_MARKERS[10], &gGameState.markers[0], 8);
        if (gGameState.markerCount > 2)
            CpuCopy16(&GBACOM_BUF_MARKERS[18], &gGameState.markers[2], (gGameState.markerCount - 2) * 8);
    }
    else
    {
        CpuCopy16(&GBACOM_BUF_MARKERS[2], gGameState.markers, gGameState.markerCount * 8);
    }
    GbaComDataStatusReset(GBACOM_SLOT_MARKERS);
}

// Reset the per-map state (Mode1State4 after a map change). The object loop counts up: agbcc
// reverses it into the original's pointer countdown, a countdown in C keeps the counter.
void ResetMapState(void)
{
    s32 i;

    gGameState.selectedType = 0;
    gGameState.selection = 0;
    gGameState.mapLoaded = 1;
    gGameState.revealedType13 = 0;
    gGameState.revealedType5 = 0;
    gGameState.revealedType12 = 0;
    gGameState.rToggle = 1;
    gMain.unk404 = 0;
    gGameState.squareMsgPending = 0;
    gGameState.squareMsgTimer = 0;
    gGameState.viewLocked = 0;
    gGameState.textBig = 0;
    gGameState.switchMsg = 0xFFFF;
    gGameState.switchType = 0;
    gGameState.msgLoopSound = -1;
    gGameState.nextMsg = -1;
    gGameState.hudChartLabel = 0;
    gGameState.awayFromType20 = 0;
    gGameState.chestState = 0;
    for (i = 0; i < 16; i++)
        gObjs[i].active = 0;
    if (!gGameState.itemsEnabled)
    {
        gGameState.itemsEnabled = 1;
        gGameState.effectActive = 0;
        gGameState.effectStep = 0;
    }
    InitMapView();
}
// Wait for the GC to switch maps (slots 1-3), then load the new one and fade in (state 5).
void Mode1State4(void)
{
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAP_IMAGE))
    {
        gGameState.mapLoaded = 1;
        gGameState.markerDelay = 20;
        LoadMapImage();
    }
    if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
    {
        GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
        if (gGameState.mapLoaded)
            gGameState.mapLoaded = 0;
        gGameState.markerDelay = 20;
    }
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
        GbaComDataStatusReset(GBACOM_SLOT_MAIL);
    if (!gGameState.flags.noMap)
    {
        if (gMain.newKeys)
            SoundPlay(SE_KEY_BLOCKED, 0);
        if (gGameState.flags.restarting)
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                Mode1RecvMarkers();
        }
        else if (!(gGameState.flags.busy & 2))  // no event running
        {
            if (!gMain.inDungeon)
            {
                if (GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_IDLE)
                 || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVED))
                {
                    if (gGameState.markerDelay != 0)
                    {
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                            Mode1RecvMarkers();
                        gGameState.markerDelay--;
                    }
                    else if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                    {
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_MAP_IMAGE))
                            GbaComDataStatusReset(GBACOM_SLOT_MAP_IMAGE);
                        if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
                            GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
                        gMain.mapImageMode = gMain.inDungeon;
                        ItemMenuInit();
                        InitMapBgs();
                        Mode1RecvMarkers();
                        ResetMapState();
                        gMain.bgcnt[2] |= 0x40;
                        gMain.bgcnt[3] |= 0x40;
                        gMain.mosaic = 0xFFFF;
                        gMain.dispcnt = 0x1F00;
                        gGameState.fadeTimer = 2;
                        gMain.modeState = 5;
                    }
                }
            }
            else if (gGameState.mapLoaded)
            {
                if (gGameState.markerDelay != 0)
                {
                    if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                        Mode1RecvMarkers();
                    gGameState.markerDelay--;
                }
                else if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                {
                    Mode1RecvMarkers();
                    gMain.mapImageMode = gMain.inDungeon;
                    ItemMenuInit();
                    InitMapBgs();
                    ResetMapState();
                    gMain.bgcnt[3] |= 0x40;
                    gMain.mosaic = 0xFFFF;
                    gMain.dispcnt = 0x1B00;
                    gGameState.fadeTimer = 2;
                    gMain.modeState = 5;
                }
            }
        }
    }
    DrawSprite(gWaitSprite, 0, 120, 80, 0, 0, 0, 0, 0, 0, 0);
    UpdateMsgAndTingle();
}
void Mode1StartFadeOut(void)
{
    ObjResetActive();
    gGameState.mapLoaded = 0;
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
    if (gGameState.dungeonChests != gGameState.flags.dungeonChests && gGameState.chestState == 0)
    {
        gGameState.newChests = gGameState.flags.dungeonChests ^ gGameState.dungeonChests;
        gGameState.chestState = 1;
        gMain.bgOfs[1][1] = -4;
    }
#ifdef VERSION_GZLJ01
    if (gGameState.chestState == 1 && gGameState.inMessage == 0 && gGameState.active != 0)
#else
    // The US builds wait for no message, Select window, menu or event instead. agbcc's
    // fold_truthop merges the two adjacent byte tests into one halfword load (written as
    // `!*(u16 *)&inMessage`, the code is the same but chest ends up in r8).
    if (gGameState.chestState == 1 && gGameState.inMessage == 0 && gGameState.selectWinOpen == 0
     && !gGameState.flags.busy)
#endif
    {
        gGameState.chestTimer = 0;
        gGameState.menuOpen = 1;
        gGameState.inMessage = 0;
        gGameState.tingleAnim = 0;
        gGameState.tingleAnimPart = 0;
        gGameState.tingleFrame = 0;
        gGameState.chestState += 1;
        gGameState.chestCount = 0;
        for (i = 0; i <= 4; i++)
        {
            if (gGameState.flags.dungeonChests & (1 << i))
                gGameState.chestCount++;
            if (gGameState.newChests & (1 << i))
                chest = i;
        }
        ShowMessage(gChestMsgIds[chest], 1);
    }

    if (gGameState.hasMagic != gGameState.flags.hasMagic)
        gGameState.hasMagicChanged = 1;
    if (gGameState.hasMagicChanged && gGameState.active && gMain.modeState == 2 && gGameState.inMessage == 0)
    {
        gGameState.hasMagicChanged = 0;
        ShowMessage(0x10, 1);
        SetSlotItem(4, 5);
        SetSlotItem(8, 9);
        SetSlotItem(9, 10);
    }

    if (gMain.modeState == 2)
    {
        // Life: a message when it drops to 7 or below (once), PlayMsgSound(15) on other drops.
        if (gGameState.inMessage == 0 && gGameState.menuOpen == 0)
        {
            if (gGameState.flags.restarting)
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
                    PlayMsgSound(15);
                }
            }
            else if (gGameState.life < gGameState.flags.life)
            {
                if (gGameState.flags.life > 7)
                    gGameState.lowLifeWarned = 0;
                gGameState.life = gGameState.flags.life;
            }
        }
        else if ((u16)(gGameState.msgId - 0x10E) <= 2)
        {
            gGameState.life = gGameState.flags.life;
            if (gGameState.flags.life > 7)
                gGameState.lowLifeWarned = 0;
        }

        // On the sea: a clock sound at 6:00, 12:00 and 20:00 that depends on the weekday.
        if (gGameState.flags.dungeonNo == 0 && gGameState.inMessage == 0 && gGameState.menuOpen == 0
         && gGameState.chartOpen == 0)
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
    if (gGameState.flags.noMap || gMain.inDungeon != gMain.mapImageMode
     || GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL)
     || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVING)
     || GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_RECEIVED)
     || !gGameState.mapLoaded)
    {
        if (gMain.modeState == 2)
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
                GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
            Mode1StartFadeOut();
            gGameState.mapLoaded = 0;
            gGameState.chartOpen = 0;
            gGameState.lastMapImageMode = gMain.mapImageMode;
            if (gGameState.menuOpen || gGameState.inMessage)
            {
                MsgClose();
                gGameState.msgId = 0xFFFF;
                if (gGameState.inMessage)
                    gGameState.unk21C = 0;
                if (gGameState.menuOpen)
                    gGameState.menuOpen = 0;
            }
            if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
                GbaComDataStatusReset(GBACOM_SLOT_MARKERS);
        }
    }
    else if (gMain.modeState != 4 && GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
    {
        Mode1RecvMarkers();
    }

    if (GbaComRecvStatusCheck(GBACOM_SLOT_SE))
    {
        PlayMsgSound(GBACOM_BUF_SE[0] - 1);
        GbaComDataStatusReset(GBACOM_SLOT_SE);
    }

    if (GbaComRecvStatusCheck(GBACOM_SLOT_MAIL))
    {
        if (gMain.modeState == 2)
        {
            if (GBACOM_BUF_MAIL[7])
                PlayMsgSound(GBACOM_BUF_MAIL[7] - 1);
            if (gGameState.inMessage == 0 && gGameState.menuOpen == 0 && gGameState.chartOpen == 0
             && gGameState.selectWinOpen == 0)
            {
                GbaComRecvDataGet(&gGameState.switchMsg, GBACOM_SLOT_MAIL, 8);
                if (gGameState.switchType == 5 || gGameState.switchType == 6
                 || gGameState.switchType == 9 || gGameState.switchType == 7
                 || gGameState.switchType == 11 || gGameState.switchType == 12
                 || (gGameState.switchType == 13 && !gGameState.flags.isFree
                     && !gGameState.flags.followTarget))
                {
                    if (gGameState.switchType == 12)
                    {
                        if (gGameState.holdState == 0)
                        {
                            if (!gGameState.flags.cursorHeld)
                                gGameState.flags.cursorHeld = 1;
                            gGameState.holdState = 1;
                            gGameState.holdMsgIndex = 0;
                            gGameState.holdFrame = 0;
                            gGameState.holdTimer = gCursorHeldSprite[4];
                        }
                        else if (gGameState.holdState == 1)
                        {
                            gGameState.holdState = 2;
                            gGameState.holdFrame = 0;
                            gGameState.holdTimer = gCursorReleaseSprite[4];
                            ShowMessage(0x5B, 0);
                        }
                    }
                    if (gGameState.holdState <= 1)
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
                GbaComRecvDataGet(&gGameState.switchMsg, GBACOM_SLOT_MAIL, 8);
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
            gGameState.effectStep = 0;
            EndItemEffect();
        }
        else
        {
            GbaComRecvDataGet(gGameState.effect, GBACOM_SLOT_EFFECT, 4);
            gGameState.itemsEnabled = 0;
            gGameState.effectActive = 1;
            if (gGameState.effect[0] == 2 || gGameState.effect[0] == 15 || gGameState.effect[0] == 21)
            {
                gGameState.effect[0] = 2;
                StartBombBlast();
                SoundPlay(0x23 - SOUND_ID_SHIFT, 0);
            }
            else if (gGameState.effect[0] == 16)
            {
                StartLookEffect();
                SoundPlay(0x23 - SOUND_ID_SHIFT, 0);
                if (gGameState.holdState)
                {
                    ShowMessage(gGameState.holdMsgIndex + 0x60, 1);
                    if (gGameState.holdMsgIndex <= 2)
                        gGameState.holdMsgIndex++;
                }
            }
            else if (gGameState.effect[0] == 3)
                ShowMessage(0x24, 1);
            else if (gGameState.effect[0] == 4)
                ShowMessage(0x26, 1);
            else if (gGameState.effect[0] == 5)
                StartRandomEffect();
            else if (gGameState.effect[0] == 7)
                StartShopEffect();
            else if (gGameState.effect[0] == 8 || gGameState.effect[0] == 9 || gGameState.effect[0] == 10)
                gGameState.effectStep = 7;
            else if (gGameState.effect[0] == 13)
                ShowMessage(0x26, 1);
            else if (gGameState.effect[0] == 14)
                StartBombCountdown();
            else if (gGameState.effect[0] == 17)
            {
                gGameState.effect[0] = 3;
                StartBalloonEffect();
            }
            else if (gGameState.effect[0] == 18)
            {
                gGameState.effect[0] = 4;
                StartShieldEffect();
            }
            else if (gGameState.effect[0] == 20)
                gGameState.effect[0] = gGameState.savedEffect;
            else if (gGameState.effect[0] == 19)
            {
                ShowMessage(0x2D, 1);
                gGameState.effect[0] = 5;
                gGameState.effectStep = 6;
            }
            else
                gGameState.effectTimer = 0;
        }
    }

    // Slot 11: 8 bytes to visitedSquares.
    if (GbaComRecvStatusCheck(GBACOM_SLOT_VISITED))
    {
        CpuCopy16(GBACOM_BUF_VISITED, gGameState.visitedSquares, 8);
        GbaComDataStatusReset(GBACOM_SLOT_VISITED);
    }

    if (gMain.modeState == 2)
    {
        gGameState.selectedMarker = UpdateSelection();
        if (gGameState.inMessage == 0 && gGameState.menuOpen == 0 && gGameState.itemsEnabled)
        {
            if (gGameState.selection && gGameState.chartOpen == 0)
            {
                if (gGameState.tingleAnim != 11)
                {
                    gGameState.tingleAnim = 11;
                    StartTingleAnim();
                }
            }
            else if (gGameState.tingleAnim)
            {
                gGameState.tingleAnim = 0;
                StartTingleAnim();
            }
        }
        // Darken the screen and show the wait sprite while a menu or event runs on the GC.
        if ((gGameState.selectWinOpen | gGameState.inMessage | gGameState.chartOpen | gGameState.menuOpen) == 0
         && gGameState.flags.busy)
        {
            gMain.bldcnt = 0x2FEF;
            gMain.bldy = 7;
            DrawSprite(gWaitSprite, 0, 120, 80, 0, 0, 0, 0, 0, 0, 0);
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

        UpdateRupeeCounter();
        map = (u16 *)0x0600644C;
        map[0] = gRupeeIconTiles[gGameState.flags.walletSize];
        map[0x20] = gRupeeIconTiles[gGameState.flags.walletSize] + 1;
    }

    // 3-8 s after Link's marker moved to another sea chart square (Mode1RecvMarkers), name
    // the square he is in (message 0x406 + square).
    if (gMain.modeState == 2 && gGameState.squareMsgPending)
    {
        if (gGameState.squareMsgTimer != 0)
            gGameState.squareMsgTimer--;
        else if (gGameState.menuOpen == 0 && gGameState.inMessage == 0 && gGameState.selectWinOpen == 0
              && gGameState.chartOpen == 0)
        {
            gGameState.squareMsgPending = 0;
            i = 0;
            do
            {
                if (gGameState.markers[i].type == 1 || i >= gGameState.markerCount)
                    break;
                i++;
            } while (1);
            // The square (7 per row) goes into i too: a separate local shifts the registers.
            i = (gGameState.markers[i].y >> 8) * 7 + (gGameState.markers[i].x >> 8);
            if (i == 25 && gGameState.flags.towerRaised)
                ShowMessage(0x437, 1);
            else
                ShowMessage(i + 0x406, 1);
        }
    }
}
