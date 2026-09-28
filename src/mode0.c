#include "global.h"
#include "main.h"
#include "game.h"
#include "sound.h"
#include "gba/syscall.h"
#include "gba_com.h"

// Game mode 0, the title screen while the GC sends the boot data: the message (state 1), the
// name, slot 14 and in a dungeon the map image (state 2), then the markers (state 3), after
// which the game switches to mode 1. MainMode0 runs one state per frame.

void Mode0State0(void);
void Mode0State1(void);
void Mode0State2(void);
void Mode0State3(void);
void Mode1RecvMarkers(void);

#ifndef VERSION_GZLJ01
// 0xFF-terminated byte string used by Mode0State0; not in the Japanese build.
#if PAL_LANGUAGE == 1
// "Verbindungsaufbau..."
const u8 gMode0UsString[] = {0x16, 0x1F, 0x2C, 0x1C, 0x23, 0x28, 0x1E, 0x2F, 0x28, 0x21, 0x2D, 0x1B,
                             0x2F, 0x20, 0x1C, 0x1B, 0x2F, 0x5F, 0x5F, 0x5F, 0xFF};
#elif PAL_LANGUAGE == 2
// "Appel en cours" and 0x65, an ellipsis
const u8 gMode0UsString[] = {0x01, 0x2A, 0x2A, 0x1F, 0x26, 0xFD, 0x1F, 0x28, 0xFD, 0x1D, 0x29, 0x2F,
                             0x2C, 0x2D, 0x65, 0xFF};
#elif PAL_LANGUAGE == 3
// "LLAMANDO"
const u8 gMode0UsString[] = {0x0C, 0x0C, 0x01, 0x0D, 0x01, 0x0E, 0x04, 0x0F, 0xFF};
#elif PAL_LANGUAGE == 4
// "Chiamata in corso..."
const u8 gMode0UsString[] = {0x03, 0x22, 0x23, 0x1B, 0x27, 0x1B, 0x2E, 0x1B, 0xFD, 0x23, 0x28, 0xFD,
                             0x1D, 0x29, 0x2C, 0x2D, 0x29, 0x5F, 0x5F, 0x5F, 0xFF};
#else
// "Calling..."
const u8 gMode0UsString[] = {0x03, 0x1B, 0x26, 0x26, 0x23, 0x28, 0x21, 0x5F, 0x5F, 0x5F, 0xFF};
#endif
#endif

// The title's pre-drawn tiles from 0x1388, centred on BG map row 10.
#if PAL_LANGUAGE == 1 || PAL_LANGUAGE == 4  // German, Italian
#define TITLE_TILES 16
#define TITLE_MAP 0x0600628E
#elif PAL_LANGUAGE == 2  // French
#define TITLE_TILES 13
#define TITLE_MAP 0x06006290
#elif PAL_LANGUAGE == 3  // Spanish
#define TITLE_TILES 7
#define TITLE_MAP 0x06006296
#else
#define TITLE_TILES 9
#define TITLE_MAP 0x06006294
#endif

// Title sprite bob, indexed by (frameCount >> 4) & 3.
const s16 gMode0BobOffsets[4] = {0, -1, 0, 1};

void (*const gMode0StateFuncs[])(void) =
{
    Mode0State0,
    Mode0State1,
    Mode0State2,
    Mode0State3,
};

// Title screen: load the graphics, write the title text, fade from a blend (state 1).
void Mode0State0(void)
{
#ifdef VERSION_GZLJ01
    u16 *map = (u16 *)0x06006296;
#else
    u16 tile = 0x1388;
    u16 *map;
    s32 i;
#endif

    u8 blend;

    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x1100;  // BG0 and OBJ
    gMain.bgcnt[0] = 0x4C08;
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuFill16(0x109D, (void *)0x06006000, 0x800);
#ifdef VERSION_GZLJ01
    *map++ = 0x1188;
    *map++ = 0x1182;
    *map++ = 0x1181;
    *map++ = 0x1181;
    *map++ = 0x118F;
    *map++ = 0x1193;
    *map = 0x118E;
#else
    map = (u16 *)TITLE_MAP;
    for (i = TITLE_TILES; i != 0; i--)
        *map++ = tile++;
    gGameState.textColor = 6;
    gGameState.glyphWidth = 6;
    DrawString(gMode0UsString, 0, 0, 0);
#endif
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.modeState++;
    gMode0Work.blend = blend = 0;
    gMain.bldcnt = 0x2071;  // BG0 over BG0-3, OBJ and backdrop
    gMain.bldalpha = ((16 - blend) << 8) | blend;
    gMode0Work.timer = 1;
    gMode0Work.fadingOut = 0;
    gMode0Work.linkTimeout = 120;
    gGameState.spriteY = 0x20;
    gGameState.spriteX = -24;
    SoundPlay(SE_TITLE, 0);
}
// The title sprite scrolls right (every other frame) and bobs.
void Mode0DrawTitleSprite(void)
{
    s16 bob = gMode0BobOffsets[(gMain.frameCount & 0x30) >> 4];

    if (gMain.frameCount & 1)
    {
        if (++gGameState.spriteX > 0xFF)
            gGameState.spriteX = -24;
    }
    DrawSpriteAt(gTitleSprite, 0, gGameState.spriteX, bob + gGameState.spriteY, 0, 0, 0);
}
// Every other frame, step the title's blend (gMode0Work.blend, 1-16) toward the end it is
// heading for (gMode0Work.fadingOut); hold for 8 frames at full.
void Mode0UpdateBlend(void)
{
    if (--gMode0Work.timer == 0)
    {
        gMode0Work.timer = 2;
        if (gMode0Work.fadingOut == 0)
        {
            gMode0Work.blend++;
            if (gMode0Work.blend == 16)
            {
                gMode0Work.timer = 8;
                gMode0Work.fadingOut ^= 1;
            }
        }
        else
        {
            gMode0Work.blend--;
            if (gMode0Work.blend == 1)
                gMode0Work.fadingOut ^= 1;
        }
        gMain.bldalpha = ((16 - gMode0Work.blend) << 8) | gMode0Work.blend;
    }
    Mode0DrawTitleSprite();
}
// Wait for the message data (slot 0, msg_LZ.bin) and unpack it to the start of EWRAM; give
// up (mode 2) if the slot stays idle for 120 frames.
void Mode0State1(void)
{
    Mode0UpdateBlend();
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MESSAGE))
    {
        LZ77UnCompWram(gGbaComMessageBuf, (void *)0x02000000);
        GbaComDataStatusReset(GBACOM_SLOT_MESSAGE);
        gMode0Work.linkTimeout = 120;
        gMain.modeState++;
    }
    if (GbaComCheckDataStatus(GBACOM_SLOT_MESSAGE, GBACOM_STATUS_IDLE))
    {
        if (--gMode0Work.linkTimeout == 0)
        {
            GbaComStop();
            gMain.mode = 2;
            gMain.modeState = 0;
        }
    }
    SoundPlayLoop(5, 5, 0);
}
// Wait for the name (slot 10) and slot 14, and in a dungeon for the map image (slot 2).
void Mode0State2(void)
{
    Mode0UpdateBlend();
    if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
    {
        gGameState.mapLoaded = 0;
        GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
    }
    if (!gGameState.flags.noMap)
    {
        if (gGameState.flags.dungeonNo == 0)
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_NAME) && GbaComRecvStatusCheck(GBACOM_SLOT_CHART_POINTS))
                gMain.modeState++;
        }
        else
        {
            if (GbaComRecvStatusCheck(GBACOM_SLOT_MAP_IMAGE) && GbaComRecvStatusCheck(GBACOM_SLOT_NAME)
             && GbaComRecvStatusCheck(GBACOM_SLOT_CHART_POINTS))
            {
                gGameState.mapLoaded = 1;
                LoadMapImage();
                gMode0Work.linkTimeout = 120;
                gMain.modeState++;
            }
            if (GbaComCheckDataStatus(GBACOM_SLOT_MAP_IMAGE, GBACOM_STATUS_IDLE))
            {
                if (--gMode0Work.linkTimeout == 0)
                {
                    GbaComStop();
                    gMain.mode = 2;
                    gMain.modeState = 0;
                }
            }
        }
    }
    SoundPlayLoop(5, 5, 0);
}
// Wait for the markers (slot 3), take the name and slot 14, and start mode 1.
void Mode0State3(void)
{
#if PAL_LANGUAGE == 1
    s32 i;

#endif
    if (GbaComRecvStatusCheck(GBACOM_SLOT_CONTROL))
    {
        gGameState.mapLoaded = 0;
        GbaComDataStatusReset(GBACOM_SLOT_CONTROL);
    }
    if (GbaComRecvStatusCheck(GBACOM_SLOT_MARKERS))
    {
        Mode1RecvMarkers();
        GbaComRecvDataGet(gGameState.playerName, GBACOM_SLOT_NAME, sizeof(gGameState.playerName));
#if PAL_LANGUAGE == 1
        // German: does the name end in S, X, Z, s, x or z (the genitive is then "Links'")?
        for (i = 0; i < 8; i++)
        {
            u8 c = gGameState.playerName[i];

            // (with `if (c == 0xFF) break;` first, agbcc copies that test in front of the loop)
            if (c != 0xFF)
            {
                if (c == 0x13 || c == 0x18 || c == 0x1A || c == 0x2D || c == 0x32 || c == 0x34)
                    gGameState.nameSibilant = 1;
                else
                    gGameState.nameSibilant = 0;
            }
            else
            {
                break;
            }
        }
#endif
        GbaComRecvDataGet(gChartPoints, GBACOM_SLOT_CHART_POINTS, sizeof(gChartPoints));
        gMain.mode = 1;
        gMain.modeState = 0;
        gGameState.mapLoaded = 1;
        gGameState.rupees = gGameState.flags.rupees;
        gGameState.rupeeDelta = 0;
        gGameState.appeared = 1;
        gGameState.shopUnlocked = gGameState.flags.shopUnlocked;
        gGameState.balloonDiscount = gGameState.flags.balloonDiscount;
        gGameState.tingDiscount = gGameState.flags.tingDiscount;
        SoundPlay(SE_LINKED, 0);
    }
    else
    {
        SoundPlayLoop(5, 5, 0);
    }
    if (GbaComCheckDataStatus(GBACOM_SLOT_MARKERS, GBACOM_STATUS_IDLE))
    {
        if (--gMode0Work.linkTimeout == 0)
        {
            GbaComStop();
            gMain.mode = 2;
            gMain.modeState = 0;
        }
    }
}

void MainMode0(void)
{
    gMode0StateFuncs[gMain.modeState]();
}
