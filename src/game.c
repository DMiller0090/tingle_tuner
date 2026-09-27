#include "global.h"
#include "gba/syscall.h"
#include "main.h"
#include "game.h"
#include "gba/syscall.h"
#include "sound.h"
#include "gba_com.h"
#include "gba/syscall.h"

// Game code after heap.c, up to the sound driver (GZLJ01 02018884-02020E28). Not split into
// objects yet: see docs/layout.md for the groups the .rodata order implies.

INCLUDE_ASM(Func_02018884);

INCLUDE_ASM(Func_02018944);

INCLUDE_ASM(Func_02018988);

INCLUDE_ASM(StepToward);

INCLUDE_ASM(Func_02018A38);

INCLUDE_ASM(Func_02018A90);

INCLUDE_ASM(Func_02018AA4);

INCLUDE_ASM(DrawSprite);

INCLUDE_ASM(DrawSpriteAt);

INCLUDE_ASM(Func_020190A8);

INCLUDE_ASM(Func_0201920C);

INCLUDE_ASM(Func_020192D8);

INCLUDE_ASM(Func_02019350);

INCLUDE_ASM(Func_02019510);

INCLUDE_ASM(Func_02019CAC);

INCLUDE_ASM(Func_02019F5C);

INCLUDE_ASM(Func_0201A0A8);

// Per-frame map update. Sea chart mode (gMain.unk457 == 0): find the four sea chart squares
// around the view, step their tile animations and draw the chart. Map mode: while L is held,
// scroll the map image with the D-pad and draw the scroll cursor.
// Nearly matches: all but one instruction's placement. The original computes j * 2 (for
// seaAnimTimer[j]) in the loop body's first block, next to the row read; this computes it
// in the else. Writing `t = j * 2;` after the row and `seaAnimTimer[t / 2]++` gets it within
// one scheduling slot. The locals dx and dy double as the loop's column and row.
//     void UpdateMap(void)
//     {
//         u16 maxX = gGameState.mapImage->width - 120;
//         u16 maxY = gGameState.mapImage->height - 120;
//         s16 dx = 0;
//         s16 dy = 0;
//         u8 i = FindMarker(3);
//
//         if (i == 0xFF)
//             FindMarker(1);
//         gGameState.unk018 = 0;
//         if (gMain.unk457 == 0)
//         {
//             s32 j;
//
//             Func_0201B2C0();
//             Func_02019F5C();
//             for (j = 0; j < 4; j++)
//             {
//                 s32 k;
//
//                 dx = (((s16)gMain.bgOfs[2][0] + 8) >> 8) + gSeaQuadrantOffsets[j][0];
//                 if (dx > 6)
//                     dx = 7;
//                 dy = (((s16)gMain.bgOfs[2][1] + 8) >> 8) + gSeaQuadrantOffsets[j][1];
//                 if (dy > 6)
//                     dy = 7;
//                 k = (dy & 1) * 2 + (dx & 1);
//                 gGameState.seaSquare[k] = dx + dy * 7;
//                 gGameState.seaTileSet[k] = gSeaSquareTileSets[dy * 7 + dx];
//                 if (gGameState.seaAnimTimer[k] >= gSeaAnimPeriods[gGameState.seaTileSet[k]])
//                 {
//                     if (++gGameState.seaAnimFrame[k] > 4)
//                         gGameState.seaAnimFrame[k] = 0;
//                     gGameState.seaAnimTimer[k] = 0;
//                 }
//                 else
//                 {
//                     gGameState.seaAnimTimer[j]++;  // [j], not [k]
//                 }
//             }
//             CpuCopy16(gSeaAnimTiles, (void *)0x0600C000, 0x2000);
//             Func_02019CAC();
//         }
//         else
//         {
//             if (!(gMain.heldKeys & 0x200))
//             {
//                 if (gGameState.mapScrolling == 1)
//                 {
//                     SoundPlay(7 - SOUND_ID_SHIFT, 0);
//                     gGameState.mapScrolling = 0;
//                 }
//                 if (gGameState.unk120 == 0)
//                     Func_0201A0A8();
//             }
//             else if (gGameState.unk1E5_1)
//             {
//                 u8 mosaic;
//                 const u8 *sprite;
//                 u8 frame;
//                 bool32 clamped;
//
//                 if (gMain.newKeys & 0x200)
//                 {
//                     SoundPlay(0x16 - SOUND_ID_SHIFT, 0);
//                     gGameState.mapScrolling = 1;
//                 }
//                 if (gMain.heldKeys & 0x40)
//                     dy = -4;
//                 else if (gMain.heldKeys & 0x80)
//                     dy = 4;
//                 if (gMain.heldKeys & 0x20)
//                     dx = -4;
//                 else if (gMain.heldKeys & 0x10)
//                     dx = 4;
//                 mosaic = gMain.mosaic != 0;
//                 sprite = gSprite_0202BDC8;
//                 frame = 0;
//                 if (!(gMain.frameCount & 0xC))
//                     frame = 1;
//                 DrawSprite(sprite, frame, 0x44, 0x44, 0, mosaic, 0, 0, 0, 0, 0);
//                 clamped = FALSE;
//                 gGameState.mapX += dx;
//                 if (gGameState.mapX < 0)
//                 {
//                     gGameState.mapX = 0;
//                     clamped = TRUE;
//                 }
//                 if (gGameState.mapX > (s16)maxX)
//                 {
//                     gGameState.mapX = maxX;
//                     clamped = TRUE;
//                 }
//                 gGameState.mapY += dy;
//                 if (gGameState.mapY < 0)
//                 {
//                     gGameState.mapY = 0;
//                     clamped = TRUE;
//                 }
//                 if (gGameState.mapY > (s16)maxY)
//                 {
//                     gGameState.mapY = maxY;
//                     clamped = TRUE;
//                 }
//                 if (!clamped && (gMain.heldKeys & 0xF0))
//                     SoundPlayLoop(8, 8, NULL);
//             }
//             Func_02019350(&gGameState);
//         }
//         if (gGameState.unk224 == 0)
//             Func_02019510(&gGameState, gMain.frameCount);
//     }
INCLUDE_ASM(UpdateMap);

void Func_0201A540(void)
{
    if (gMain.unk457 == 0)
    {
        Func_02019F5C();
    }
    else
    {
        Func_0201A0A8();
        Func_02019350(&gGameState);
    }
}

// Tile of the floor number box's edge.
#ifdef VERSION_GZLJ01
#define FLOOR_EDGE_TILE 0x6D
#else
#define FLOOR_EDGE_TILE 0x01
#endif

// Draw the dungeon floor number next to the map (blank outside dungeons or with no floor).
void DrawFloorNumber(void)
{
    u16 *map = (u16 *)0x06006074;
    u8 floor;

    if (gGameState.mapImage->floor == 0 || gGameState.flags.dungeonNo == 0)
    {
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
    }
    else
    {
        map[0] = 0xE458;
        map[1] = 0xE800 + FLOOR_EDGE_TILE;
        map[2] = 0xE800 + FLOOR_EDGE_TILE;
        map[0x20] = 0xE458;
        map[0x40] = 0xE458;
        map[0x60] = 0xE458;
        map[0x61] = 0xE000 + FLOOR_EDGE_TILE;
        map[0x62] = 0xE000 + FLOOR_EDGE_TILE;
        floor = gGameState.mapImage->floor;
        if ((s8)floor < 0)
        {
            u8 n = ~floor * 2 + 2;  // -floor * 2

            map[0x21] = 0xE05C;
            map[0x41] = 0xE05D;
            map[0x22] = n + 0xE030;
            map[0x42] = n + 0xE031;
        }
        else
        {
            map[0x21] = gGameState.mapImage->floor * 2 + (s16)0xE030;
            map[0x41] = gGameState.mapImage->floor * 2 + (s16)0xE031;
            map[0x22] = 0xE05E;
            map[0x42] = 0xE05F;
        }
    }
}

// Point gGameState.mapImage at the received map image and relocate its data offsets (unless
// they are already addresses).
bool32 LoadMapImage(void)
{
    gGameState.mapImage = (struct MapImage *)GBACOM_BUF_MESSAGE;
    if (*(u32 *)(GBACOM_BUF_MESSAGE + 8) <= 0x01FFFFFF)
    {
        gGameState.mapImage->tiles = (u8 *)((u32)gGameState.mapImage->tiles + (u32)gGameState.mapImage);
        gGameState.mapImage->map = (u16 *)((u32)gGameState.mapImage->map + (u32)gGameState.mapImage);
    }
    GbaComDataStatusReset(2);
}

// Set up the map view on the player's marker: in map mode, load the map image's tiles and
// draw the visible part of its map; otherwise just centre the view.
bool32 InitMapView(void)
{
    u8 i = FindMarker(3);
    u16 maxX, maxY;
    s16 x, y;
    s16 yEnd;
    u16 *map;

    if (i == 0xFF)
        i = FindMarker(1);
    if (gMain.unk457 != 0)
    {
        if (gGameState.unk10E == 0)
        {
            CpuFill16(0, GBACOM_BUF_MESSAGE, 0xA000);
            gGameState.mapX = gMain.bgOfs[3][0];
            gGameState.mapY = gMain.bgOfs[3][1];
        }
        else
        {
            CpuCopy16(gGameState.mapImage->tiles, (void *)0x06000000, gGameState.mapImage->tilesSize);
            maxX = gGameState.mapImage->width - 120;
            maxY = gGameState.mapImage->height - 120;
            gGameState.mapX = gGameState.markers[i].x - 60;
            gGameState.mapY = gGameState.markers[i].y - 60;
            if (gGameState.mapX < 0)
                gGameState.mapX = 0;
            if (gGameState.mapX > (s16)maxX)
                gGameState.mapX = maxX;
            if (gGameState.mapY < 0)
                gGameState.mapY = 0;
            if (gGameState.mapY > (s16)maxY)
                gGameState.mapY = maxY;
            gMain.bgOfs[3][0] = gGameState.mapX - 8;
            gMain.bgOfs[3][1] = gGameState.mapY - 8;
            DrawFloorNumber();
            x = gGameState.mapX;
            y = gGameState.mapY;
            yEnd = y + 120;
            map = gGameState.mapImage->map + ((y >> 3) * gGameState.mapImage->mapWidth + (x >> 3));
            CpuFill16(0x2000, (void *)0x06005800, 0x800);
            do
            {
                Func_020192D8(y, x, map);
                map += gGameState.mapImage->mapWidth;
                y += 8;
            } while (y <= yEnd);
        }
    }
    else
    {
        gGameState.mapX = gGameState.markers[i].x - 68;
        gGameState.mapY = gGameState.markers[i].y - 68;
    }
    return TRUE;
}

u8 FindMarker(u8 type)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (type == gGameState.markers[i].type)
            return i;
    }
    return 0xFF;
}

// Distance from the type 1 marker to icons[2] (arg 0) or to the type 3 marker.
u32 MarkerDistance(u8 arg)
{
    u8 i = FindMarker(1);
    s32 dx, dy;

    if (arg == 0)
    {
        dx = gGameState.icons[2].x - gGameState.markers[i].x;
        dy = gGameState.icons[2].y - gGameState.markers[i].y;
    }
    else
    {
        u8 j = FindMarker(3);

        dx = gGameState.markers[j].x - gGameState.markers[i].x;
        dy = gGameState.markers[j].y - gGameState.markers[i].y;
    }
    dx *= dx;
    dy *= dy;
    dx += dy;
    return Sqrt(dx);
}

// The target compass: while a target (icons[2], type 3) is placed, point an arrow at it
// from the player, pulse it faster the closer the player gets, play the near and arrival
// sounds, and (once Link has the Wind Waker, flags.hasTact) announce its direction when
// Link is not facing it (flags.unk2 looks like his heading).
// Nearly matches: the logic and instruction selection are right, but registers differ from
// the start (the ArcTan2 operands land in r4 instead of r2, so the address temporaries shift
// and the targetNear stores of levels 2 and 4 are not cross-jumped):
// void UpdateTargetCompass(void)
// {
//     u8 angle;
//     s32 x, y;
//     s32 level;
//
//     if (gGameState.unk1FA != 0)
//         return;
//     if (gGameState.unk224 != 0)
//         return;
//     if (gGameState.icons[2].type != 3)
//         return;
//     {
//         u8 i = FindMarker(1);
//
//         x = gGameState.icons[2].x - gGameState.markers[i].x;
//         y = gGameState.icons[2].y - gGameState.markers[i].y;
//         angle = ((u16)~ArcTan2(x >> 4, y >> 4) + 0x4001) >> 8;
//     }
//     x = gSinTable[angle];
//     x >>= 3;
//     y = (gSinTable + 64)[angle];
//     y >>= 3;
//     DrawSprite(gMapIconSprites[gGameState.icons[2].type], 0, (u16)(x + 0xB8), (u16)(y + 0x58), 0, 0, 0, 0, 0, 0, 0);
//     level = MarkerDistance(0);
//     if ((u32)level <= 0x18)
//     {
//         if (gGameState.unk224 == 0)
//         {
//             GamePlaySound(0x1A5, 1);
//             gGameState.icons[2].type = 0;
//             gGameState.icons[1].type = 0;
//             gGameState.unk105 = 0;
//         }
//         level = 0;
//         gGameState.targetNear = 0;
//     }
//     else if ((u32)level <= 0x7F)
//     {
//         if (gGameState.unk224 == 0 && gGameState.targetNear == 0)
//         {
//             GamePlaySound(0x1A4, 1);
//             gGameState.targetNear = 1;
//         }
//         level = 1;
//     }
//     else if ((u32)level <= 0x1FF)
//     {
//         level = 2;
//         gGameState.targetNear = 0;
//     }
//     else if ((u32)level <= 0x3FF)
//     {
//         level = 3;
//         gGameState.targetNear = 0;
//     }
//     else
//     {
//         level = 4;
//         gGameState.targetNear = 0;
//     }
//     if (gGameState.targetBlinkOn != 0)
//     {
//         if (--gGameState.targetBlinkTimer == 0)
//             gGameState.targetBlinkOn = 0;
//     }
//     else
//     {
//         if (++gGameState.targetBlinkTimer >= gTargetBlinkPeriods[level])
//         {
//             gGameState.targetBlinkTimer = gTargetBlinkPeriods[level];
//             gGameState.targetBlinkOn = 1;
//         }
//     }
//     gGameState.targetScale = (gGameState.targetBlinkTimer << 7) / gTargetBlinkPeriods[level] + 0x100;
//     DrawSprite(gSprite_0202C3F8, 0, 200, 72, angle + 0x80, 0, gGameState.targetScale, 0, 16, 0, 0);
//     if (gGameState.unk224 == 0 && gGameState.flags.hasTact && level > 1
//      && (u8)(gGameState.flags.unk2 - angle + 9) > 18)
//     {
//         angle = ~angle + 1;
//         if (((angle + 8) & 0x1F) <= 0x10)
//         {
//             u8 dir = (u8)(angle + 0x88) >> 5;
//
//             if (gGameState.targetDir != dir && gGameState.unk1FA == 0)
//             {
//                 GamePlaySound(0x19C + dir, 1);
//                 gGameState.targetDir = dir;
//             }
//         }
//         else
//         {
//             gGameState.targetDir = 0xFF;
//         }
//     }
// }
INCLUDE_ASM(UpdateTargetCompass);

// Copy each view quadrant's current sea animation frame into the BG tiles, then the map image
// tiles (loaded at 0x0600C000) to 0x06004000.
// Nearly matches: only the last two instructions of each tile address differ (the original
// adds the VRAM base into a new register, `mov r2, sl; adds r0, r1, r2`, where this adds it in
// place). The separate a and t are needed to keep 0x300 from being folded into the base.
//     void Mode1VBlank(void)
//     {
//         u8 *dest = (u8 *)0x0600EC00;
//         s32 i;
//         s32 a, t;
//
//         for (i = 0; i < 4; i++)
//         {
//             a = gGameState.seaTileSet[i] * 16;
//             t = gSeaAnimFrames[gGameState.seaAnimFrame[i]] * 4 + 0x300;
//             a += t;
//             CpuCopy32((u8 *)0x06008000 + a * 32, dest, 0x80);
//             dest += 0x80;
//             a = gGameState.seaTileSet[i] * 16;
//             t = ((gSeaAnimFrames[gGameState.seaAnimFrame[i]] + 2) & 3) * 4 + 0x300;
//             a += t;
//             CpuCopy32((u8 *)0x06008000 + a * 32, dest, 0x80);
//             dest += 0x80;
//         }
//         CpuCopy32((void *)0x0600C000, (void *)0x06004000, 0x2000);
//     }
INCLUDE_ASM(Mode1VBlank);

// Icon type 1: follow the player's marker (type 3, else type 1).
void MapIconFollowMarker(struct MapIcon *icon)
{
    u8 i = FindMarker(3);

    if (i == 0xFF)
        i = FindMarker(1);
    icon->x = gGameState.markers[i].x;
    icon->y = gGameState.markers[i].y;
}

void InitMarkerIcon(void)
{
    gGameState.icons[0].type = 1;
    gGameState.icons[0].frame = 0;
    gGameState.icons[0].timer = 0;
    MapIconFollowMarker(&gGameState.icons[0]);
}

// Icon type 3: blink between frames 0 and 1.
void MapIconBlink(struct MapIcon *icon)
{
    if (icon->timer != 0)
    {
        icon->timer--;
    }
    else
    {
        const u8 *const *sprites;
        s32 offset;

        icon->frame ^= 1;
        sprites = gMapIconSprites;
        offset = icon->frame * 8;  // 8 bytes per frame; byte 4 is its duration
        icon->timer = (sprites[3] + offset)[4];
    }
}

void InitTargetIcon(void)
{
    gGameState.icons[2].type = 3;
    gGameState.icons[2].frame = 1;
    gGameState.icons[2].timer = 0;
    MapIconBlink(&gGameState.icons[2]);
}

void InitCursorIcon(void)
{
    gGameState.icons[1].type = 2;
    if (gGameState.icons[2].type != 0)
    {
        gGameState.icons[1].x = gGameState.icons[2].x;
        gGameState.icons[1].y = gGameState.icons[2].y;
    }
    else
    {
        gGameState.icons[1].x = gGameState.icons[0].x;
        gGameState.icons[1].y = gGameState.icons[0].y;
    }
}

// Icon type 2: the cursor. While placing a target (unk105 == 1), move it a sea chart square
// (16 pixels) at a time with the D-pad and place the target (icons[2]) with A. Then name the
// square under the cursor if its unk0F4 bit is set.
void MapIconCursor(struct MapIcon *icon)
{
    u16 keys;
    u16 pos;
    u8 square;
    s32 i;

    if (gGameState.unk105 == 1)
    {
        if ((gMain.newKeys & 0xF0) == 0)
        {
            keys = gMain.heldKeys;
            if (keys & 0xF0)
            {
                if (++icon->timer == 2)
                    icon->timer = 0;
            }
            else
            {
                icon->timer = 0;
            }
        }
        else
        {
            keys = gMain.newKeys;
            icon->timer = 0;
        }
        if (icon->timer == 0 && (keys & 0xF0))
        {
            pos = icon->y >> 4;
            if (keys & 0x40)
            {
                if (pos != 0)
                    icon->y -= 0x10;
            }
            else if (keys & 0x80)
            {
                if (pos != 0x6F)
                    icon->y += 0x10;
            }
            pos = icon->x >> 4;
            if (keys & 0x20)
            {
                if (pos != 0)
                    icon->x -= 0x10;
            }
            else if (keys & 0x10)
            {
                if (pos != 0x6F)
                    icon->x += 0x10;
            }
        }
        if (gMain.newKeys & 1)
        {
            gGameState.icons[2].type = 3;
            gGameState.icons[2].x = icon->x;
            gGameState.icons[2].y = icon->y;
            InitMarkerIcon();
            InitTargetIcon();
            gGameState.unk022 = 240;
            SoundPlay(10 - SOUND_ID_SHIFT, 0);
        }
    }
    square = (icon->x >> 8) + (icon->y >> 8) * 7;
    if ((gGameState.unk0F4[square >> 3] >> (square & 7)) & 1)
    {
        for (i = 0; i < 49; i++)
        {
            if (gSlot14Data[i].square == square)
            {
                if (square == 25 && !gGameState.flags.unkA_4)
                {
                    gGameState.unk13C = 0;
                }
                else
                {
                    if (square == 32 && gGameState.flags.unk5_6)
                        GamePlaySound(0x13E, 1);
                    else
                        GamePlaySound(0x14E + gSlot14Data[i].square, 1);
#ifndef VERSION_GZLJ01
                    gGameState.unk13E = 0;
#endif
                }
            }
        }
    }
    else
    {
        gGameState.unk13C = 0;
    }
}

// Update and draw the map icons.
void UpdateMapIcons(void)
{
    u8 mosaic = gMain.mosaic != 0;
    s32 i;

    for (i = 0; i < 4; i++)
    {
        if (gGameState.icons[i].type != 0)
        {
            gMapIconFuncs[gGameState.icons[i].type](&gGameState.icons[i]);
            DrawSprite(gMapIconSprites[gGameState.icons[i].type], gGameState.icons[i].frame,
                       (gGameState.icons[i].x >> 4) + 12, (gGameState.icons[i].y >> 4) + 12, 0,
                       mosaic, 0, 0, 0, 0, 0);
        }
    }
}

INCLUDE_ASM(Func_0201AFCC);

INCLUDE_ASM(Func_0201B0F4);

INCLUDE_ASM(Func_0201B2C0);

INCLUDE_ASM(Func_0201B3A4);

INCLUDE_ASM(Func_0201B7D4);

INCLUDE_ASM(Func_0201B8C8);

INCLUDE_ASM(Func_0201BA2C);

INCLUDE_ASM(Func_0201BA9C);

INCLUDE_ASM(Func_0201BAD8);

INCLUDE_ASM(Func_0201BAF8);

INCLUDE_ASM(Func_0201BC0C);

INCLUDE_ASM(Func_0201BCC0);

INCLUDE_ASM(Func_0201BDE4);

INCLUDE_ASM(Func_0201BEA0);

INCLUDE_ASM(Func_0201C0E8);

INCLUDE_ASM(Func_0201C164);

INCLUDE_ASM(Func_0201C204);

INCLUDE_ASM(Func_0201C484);

INCLUDE_ASM(Func_0201C568);

INCLUDE_ASM(Func_0201C60C);

// Draw a run of consecutive glyphs (a word stored as one piece in the font).
void DrawGlyphRun(u8 id)
{
    u16 ch = gGlyphRunStart[id];
    s32 n = gGlyphRunLength[id];
#ifndef VERSION_GZLJ01
    // The US builds draw the run 8 pixels per glyph (the width is not restored on an early
    // return).
    u8 width = gGameState.unk157;
#endif
    s32 i;

#ifndef VERSION_GZLJ01
    gGameState.unk157 = 8;
#endif
    for (i = 0; i < n; i++)
    {
        if (!Func_0201C60C())
            return;
        Func_0201BCC0(gGameState.textX, gGameState.textY, gGameState.unk14A, ch);
        ch++;
        if (gGameState.unk15A)
            gGameState.textX += gGameState.unk157 * 2;
        else
            gGameState.textX += gGameState.unk157;
    }
#ifndef VERSION_GZLJ01
    gGameState.unk157 = width;
#endif
}

#ifdef VERSION_GZLJ01
bool32 DrawChar(u8 ch)
{
    if (!Func_0201C60C())
        return FALSE;
    if (ch == 0xBC)
        gGameState.unk159 = 1;
    else
        gGameState.unk159 = 0;
    if (ch > 0xE5)
    {
        Func_0201BDE4(gGameState.textX, gGameState.textY, gGameState.unk14A, 0xC4);
        if (ch <= 0xEA)
            ch += 0x33;
        else
            ch = gGlyphTable_0202438E[ch - 0xEB];
    }
    else if (ch > 0xBC)
    {
        Func_0201BDE4(gGameState.textX, gGameState.textY, gGameState.unk14A, 0xC2);
        if (ch <= 0xC1)
            ch += 0x48;
        else if (ch <= 0xC6)
            ch += 0x48;
        else if (ch <= 0xCB)
            ch += 0x48;
        else if (ch <= 0xD0)
            ch += 0x4D;
        else if (ch <= 0xD5)
            ch += 0x6B;
        else if (ch <= 0xDA)
            ch += 0x6B;
        else if (ch <= 0xDF)
            ch += 0x6B;
        else
            ch = gGlyphTable_0202438E[ch - 0xE0];
    }
    Func_0201BCC0(gGameState.textX, gGameState.textY, gGameState.unk14A, ch);
    if (gGameState.unk15A)
        gGameState.textX += gGameState.unk157 * 2;
    else
        gGameState.textX += gGameState.unk157;
    if ((u16)gGameState.unk140 == 0xFFFF)
        SoundPlay(0x28, 0);
    return TRUE;
}
#else
INCLUDE_ASM(DrawChar);
#endif

#ifndef VERSION_GZLJ01
bool32 UsDrawChar(u8 ch)
{
    if (!Func_0201C60C())
        return FALSE;
    if (ch == 0x3F)
    {
        gGameState.unk159 = 1;
        ch = 0x18;
    }
    else
    {
        gGameState.unk159 = 0;
    }
    return DrawChar(ch);
}
#endif

void MsgCmd00(void)
{
    u16 *map = (u16 *)0x0600704E;

    gGameState.unk138_4 = 1;
    gGameState.items[3].price = 20;
    DrawItemCount(3, map + 0x40);
    map += 3;
    gGameState.items[4].price = 30;
    DrawItemCount(4, map + 0x40);
    map += 3;
    gGameState.items[5].price = 30;
    if (gGameState.slotItems[4] == 5)
        DrawItemCount(5, map + 0x40);
}

void MsgCmd01(void)
{
    u16 *map = (u16 *)0x0600718E;

    gGameState.unk138_5 = 1;
    gGameState.items[8].price = 10;
    DrawItemCount(8, map + 0x40);
    map += 3;
    gGameState.items[9].price = 20;
    gGameState.items[10].price = 40;
    if (gGameState.slotItems[8] == 9)
    {
        DrawItemCount(9, map + 0x40);
        DrawItemCount(10, map + 0x43);
    }
}

#ifndef VERSION_GZLJ01
// The US builds don't draw the weekday name.
void DrawWeekday(void)
{
}
#else
void DrawWeekday(void)
{
    s32 i;

    for (i = 0; i < 2; i++)
    {
        if (DrawChar(gDayNameChars[gGameState.flags.dayOfWeek * 2 + i]))
        {
            if (gGameState.flags.dayOfWeek == 2 || gGameState.flags.dayOfWeek == 6)
                i++;
        }
        else
        {
            return;
        }
    }
}
#endif

void DrawHour(void)
{
#ifdef VERSION_GZLJ01
    // A leading zero is left blank.
    u8 ch = gGameState.flags.hour / 10 + GLYPH_DIGIT_0;

    if (ch != GLYPH_DIGIT_0)
    {
        if (!DRAW_CHAR(ch))
            return;
    }
    else
    {
        gGameState.textX += gGameState.unk157;
    }
    DRAW_CHAR(gGameState.flags.hour % 10 + GLYPH_DIGIT_0);
#else
    if (DRAW_CHAR(gGameState.flags.hour / 10 + GLYPH_DIGIT_0))
        DRAW_CHAR(gGameState.flags.hour % 10 + GLYPH_DIGIT_0);
#endif
}

void DrawMinute(void)
{
    if (DRAW_CHAR(gGameState.flags.unk7_0 / 10 + GLYPH_DIGIT_0))
        DRAW_CHAR(gGameState.flags.unk7_0 % 10 + GLYPH_DIGIT_0);
}

void MsgCmd05(void)
{
    gGameState.unk138_3 = 1;
    SetSlotItem(6, 7);
}

void MsgCmd06(void)
{
    gGameState.unk138_3 = 0;
    SetSlotItem(6, 0);
}

void MsgCmd07(void)
{
}

void MsgCmd08(void)
{
}

void MsgCmd09(void)
{
}

void MsgCmd0A(void)
{
}

void MsgCmd0B(void)
{
}

void MsgCmd0C(void)
{
}

void MsgCmd0D(void)
{
    gGameState.unk158 = 1;
}

void MsgCmd0E(void)
{
    gGameState.unk158 = 0;
}

void MsgCmd0F(void)
{
    gGameState.unk15A = 1;
}

void MsgCmd10(void)
{
    gGameState.unk157 = 6;
}

void MsgCmd11(void)
{
    gGameState.unk157 = 8;
}

void MsgCmd12(void)
{
    SendItemUse(0x15, 0);
    gGameState.unk1E5_1 = 0;
}

// Message interpreter, one step per call: draws the next character of gGameState.msgPtr or
// runs the control codes (0xF2-0xFF) before it. Returns 0 to go on, 2 while it waits (page
// full, or for A), 3 at the end (0xFF), 1 after code 0xF6.
s32 MsgStep(void)
{
    u8 ch = *gGameState.msgPtr;
    u32 mosaic = (gMain.bgcnt[0] >> 6) & 1;
    bool32 repeat = FALSE;  // 0xFB: stay on the code for the next letter of the name
    u8 n;  // line height, then the argument byte of a code (one variable in the original)

    if ((u16)gGameState.unk140 != 0xFFFF)
    {
        if ((u8)(gGameState.unk14B - 2) > 1 || gGameState.unk14D != 0)
            SoundPlayLoop((u16)gGameState.unk140, gGameState.unk140, 0);
        else
            gGameState.unk140 = (u16)gGameState.unk140 | 0xFFFF;
    }
    if (gGameState.unk15A)
        n = 12;
    else
        n = 0;
    if (gGameState.unk159 && (u16)gGameState.unk140 == 0xFFFF)
        SoundPlayLoop(0, 0, 0);
    if (gGameState.unk14D != 0)
    {
        gGameState.unk14D--;  // delay (code 0xF9)
    }
    else
    {
        gGameState.unk14D = gGameState.unk14C;
        if (gGameState.unk14B == 2 || gGameState.textY >= 0x50 - n)
        {
            // Page full: show the arrow until A (or the timeout), then clear the window.
            if (gGameState.flags.unk3_1)
                gGameState.unk13E++;
            if (gGameState.unk13E <= 0x12B && !(gMain.newKeys & 1))
            {
                DrawSprite(gSprite_0202BDF0, (gMain.frameCount & 0xC) == 0, 0xE0, 0x84, 0, mosaic,
                           0, 0, 0, 0, 0);
                return 2;
            }
            gMain.newKeys &= ~1;
            Func_0201C0E8(1);
            gGameState.unk15A = 0;
            gGameState.unk13E = 0;
            gGameState.unk14B = 0;
            return 0;
        }
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && gGameState.unk151 != 0)
            return Func_0201C204();
        while (ch > 0xF1)
        {
            if (ch == 0xFF)  // end
            {
                gGameState.unk158 = 0;
                gGameState.unk159 = 0;
                gGameState.unk15A = 0;
                #ifndef VERSION_GZLJ01
                if (gGameState.unk151)
                    return 1;
#endif
                return 3;
            }
            if (ch == 0xFD || ch == 0xF4)  // space
            {
            space:
                gGameState.msgPtr++;
                if (gGameState.unk15A)
                    gGameState.textX += gGameState.unk157 * 2;
                else
                    gGameState.textX += gGameState.unk157;
                return 0;
            }
            if (ch == 0xFA || ch == 0xFE)  // 0xFA: wait, 0xFE: new line
            {
                if (gGameState.unk15A)
                    n = 24;
                else
                    n = 12;
                if (ch != 0xFE)
                {
                    gGameState.msgPtr++;
                    return 2;
                }
                gGameState.textY += n;
                gGameState.textX = 0;
                n -= 12;
                if (gGameState.textY >= 0x50 - n)
                {
                    gGameState.msgPtr++;
                    return 2;
                }
            }
            else if (ch == 0xFB)  // the player's name, one letter per step
            {
                u8 i = gGameState.playerNameIndex;
                s32 next;

                ch = gGameState.playerName[i];
                next = i + 1;
                if (gGameState.playerName[next] == 0xFF || i == 7)
                {
                    gGameState.playerNameIndex = 0;
                }
                else
                {
                    gGameState.playerNameIndex = next;
                    repeat = TRUE;
                }
                SoundPlay(0x28 - SOUND_ID_SHIFT, 0);
                break;
            }
            else if (ch == 0xF9)  // delay
            {
                gGameState.unk14D = *++gGameState.msgPtr;
                gGameState.msgPtr++;
                // (A return here would be laid out differently.)
                goto end;
            }
            else if (ch == 0xF8)
            {
                n = *++gGameState.msgPtr;
                if (gGameState.unk150 == 0)
                    gGameState.unk150 = gGameState.textY;
                if (n == 0)
                {
                    gGameState.unk14F = gGameState.unk14E;
                    gGameState.unk151 = 1;
                    gGameState.unk153 = n;
                    gGameState.unk152 = gSprite_0202C130[4];
                }
                else if (gGameState.unk14F == 0xFF)
                {
                    gGameState.unk14E++;
                }
                goto space;
            }
            else if (ch == 0xF7)
            {
                n = *++gGameState.msgPtr;
                if (gGameState.unk150 == 0)
                    gGameState.unk150 = gGameState.textY;
                if (n == 0)
                {
                    gGameState.unk14F = gGameState.unk14E;
                    gGameState.unk151 = 2;
                    gGameState.unk153 = n;
                    gGameState.unk152 = gSprite_0202C130[4];
                }
                else if (gGameState.unk14F == 0xFF)
                {
                    gGameState.unk14E++;
                }
                goto space;
            }
            else if (ch == 0xF6)
            {
                n = *++gGameState.msgPtr;
                Func_0201C484(n);
                gGameState.msgPtr++;
                return 1;
            }
            else if (ch == 0xF5)
            {
                gGameState.unk226 = gMsgTingleAnims[*++gGameState.msgPtr];
                Func_0201C568();
            }
            else if (ch == 0xF3)  // call a handler
            {
                gMsgCmdFuncs[*++gGameState.msgPtr]();
            }
            else if (ch == 0xF2)  // a glyph run
            {
                n = *++gGameState.msgPtr;
                DrawGlyphRun(n);
            }
            else if (ch == 0xFC)
            {
                gGameState.unk14A = *++gGameState.msgPtr;
            }
            else
            {
                while (1)
                    ;
            }
            ch = *++gGameState.msgPtr;
        }
        if (!repeat)
            gGameState.msgPtr++;
        if (!DRAW_CHAR(ch))
            return 2;
    }
end:
    return 0;
}

// Per frame in mode 1: slide the message box (msgBoxPos) and step the message (MsgStep, fast
// while A is held), then run Tingle's animation (switch on gGameState.unk226; unk227 picks the
// sprite list, unk228 the frame, unk202 counts the frame's duration, unk206 the scale) and
// draw him, and the sprite at (0xC8, 0x58) while flags.unkB_1 is set.
// Not matched yet. The draft below has the right structure and case order (the jump table
// order is 14,1,2,3+6,4,5,7,8,9,10,11,12,13,16,18+17,20+19,0; 15 returns); what's left is
// register allocation (base r6 / frame r7 / gMain r8) and the per-case frame = unk228 reads:
// the original reloads gGameState from the pool for each of them (the volatile read is a hack
// that gets close), so its tails don't cross-jump the way this draft's do.
/*
void Func_0201CFE8(void)
{
    s32 target;
    u32 mosaic;
    u8 frame;
    s32 x;
    s32 a10;
    const u8 *sprite;
    u8 r;
    u8 anim;

    if (gGameState.unk13C)
        target = 10;
    else
        target = 0;
    mosaic = (gMain.bgcnt[0] >> 6) & 1;
    frame = 0;
    a10 = 0;
    if (gGameState.msgBoxPos != target)
    {
        Func_02018988(&gGameState.msgBoxPos, target, 2);
        if (gGameState.unk13C)
        {
            Func_0201BA2C(gGameState.msgBoxPos);
            gGameState.unk14B = 1;
        }
        else
        {
            Func_0201BA9C(gGameState.msgBoxPos);
            if (gGameState.msgBoxPos == 0)
            {
                if (gGameState.unk21C)
                {
                    gGameState.unk0E5 = 0;
                    gGameState.unk21C = 0;
                }
                gGameState.unk1FA = 0;
                gGameState.unk226 = 0;
                gGameState.unk227 = 0;
                gGameState.unk228 = 0;
                if (gGameState.flags.unkA_3 && gMain.modeState == 2)
                    Mode1StartFadeOut();
                gGameState.unk142 = 0xFFFF;
                gGameState.unk104 = 1;
            }
        }
        Func_0201BEA0(gGameState.msgBoxPos);
    }
    else if (gGameState.unk13C && gGameState.unk142 >= 0)
    {
        do
        {
            r = MsgStep();
            if (gGameState.unk12B == 1 || r != 0)
                break;
        } while ((gMain.newKeys & 1) || gGameState.unk158);
        gGameState.unk14B = r;
        Func_0201BEA0(gGameState.msgBoxPos);
    }
    else if (gGameState.unk1FA)
    {
        gGameState.unk1FA = 0;
    }
    if (gGameState.unk1FA || gGameState.unk1CC || gGameState.unk224
     || (!gGameState.flags.busy && gGameState.unk104))
    {
        gGameState.unk22B = 1;
        anim = gGameState.unk226;
        sprite = gTingleAnims[anim][gGameState.unk227];
        x = 0xB8;
        switch (anim)
        {
        case 14:
            x = 0x98;
            a10 = 2;
        case 1:
            gGameState.unk206 = 0;
            if (gGameState.unk14B)
                goto talk;
            if (gGameState.unk202 == 0)
            {
                if (++gGameState.unk228 > 8)
                    gGameState.unk228 = 0;
                gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 2:
            if (gGameState.unk227 && gGameState.unk14B)
                goto talk;
            if (gGameState.unk202 == 0)
            {
                gGameState.unk228++;
                if (gGameState.unk227)
                {
                    if (gGameState.unk228 > 8)
                        gGameState.unk228 = 0;
                    gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
                }
                else
                {
                    gGameState.unk228 = 0;
                    if (gGameState.unk22A == 0)
                    {
                        if (gGameState.unk206 != 0xA0)
                        {
                            gGameState.unk206 -= 0x20;
                        }
                        else
                        {
                            gGameState.unk202 = 10;
                            gGameState.unk22A = 1;
                        }
                    }
                    else if (gGameState.unk206 != 0x100)
                    {
                        gGameState.unk206 += 0x10;
                    }
                    else
                    {
                        gGameState.unk227++;
                        sprite = gTingleAnims[gGameState.unk226][gGameState.unk227];
                        gGameState.unk202 = sprite[4];
                    }
                }
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 3:
            if (gGameState.unk204 == 0)
            {
                if (gGameState.unk22A == 0)
                {
                    if (gGameState.unk206 != 0xA0)
                        gGameState.unk206 -= 8;
                    else
                        gGameState.unk22A = 1;
                }
                else if (gGameState.unk206 != 0x100)
                {
                    gGameState.unk206 += 8;
                }
                else
                {
                    gGameState.unk204 = 10;
                    gGameState.unk22A = 0;
                }
            }
            else
            {
                gGameState.unk204--;
            }
        case 6:
            if (gGameState.unk14B)
                goto talk;
            if (gGameState.unk202 == 0)
            {
                if (++gGameState.unk228 > 8)
                    gGameState.unk228 = 0;
                gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 4:
            gGameState.unk206 = 0;
            if (gGameState.unk227 && gGameState.unk14B)
                goto talk;
            if (gGameState.unk202 == 0)
            {
                gGameState.unk228++;
                if (gGameState.unk227)
                {
                    if (gGameState.unk228 > 8)
                        gGameState.unk228 = 0;
                }
                else if (gGameState.unk228 > 0x1D)
                {
                    gGameState.unk228 = 0;
                    gGameState.unk227++;
                    sprite = gTingleAnims[gGameState.unk226][gGameState.unk227];
                }
                gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 5:
            gGameState.unk206 = 0;
            if (gGameState.unk202 == 0)
            {
                if (++gGameState.unk228 > 0x1D)
                {
                    if (gGameState.unk1C8)
                    {
                        gGameState.unk228 = 0;
                        gGameState.unk202 = sprite[4];
                    }
                    else
                    {
                        gGameState.unk228 = 0;
                        gGameState.unk226 = 0;
                        gGameState.unk227 = 0;
                        sprite = gTingleAnims[0][0];
                    }
                }
                else
                {
                    gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
                }
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 7:
            gGameState.unk206 = 0;
            if (gGameState.unk202 == 0)
            {
                gGameState.unk228 = (gGameState.unk228 + 1) & 7;
                gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 8:
            gGameState.unk206 = 0;
            if (gGameState.unk202 == 0)
            {
                gGameState.unk228 = (gGameState.unk228 + 1) & 15;
                gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
            }
            else
            {
                gGameState.unk202--;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 9:
            gGameState.unk206 = 0;
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 10:
            if (gGameState.unk202 == 0)
            {
                gGameState.unk228++;
                if (gGameState.unk227)
                {
                    if (gGameState.unk206 > 0x2FF)
                        gGameState.unk206 = 0x300;
                    else
                        gGameState.unk206 += 0x10;
                }
                else
                {
                    if (gGameState.unk228 > 6)
                    {
                        gGameState.unk228 = 0;
                        gGameState.unk206 = 0x100;
                        gGameState.unk227++;
                        sprite = gTingleAnims[gGameState.unk226][gGameState.unk227];
                    }
                    else
                    {
                        gGameState.unk206 = 0;
                    }
                    gGameState.unk202 = sprite[gGameState.unk228 * 8 + 4];
                }
            }
            else
            {
                gGameState.unk202--;
            }
            break;
        case 11:
            gGameState.unk206 = 0;
            if (gGameState.unk22C)
            {
                gGameState.unk208 += 4;
                if (gGameState.unk208 > 0xFF)
                    gGameState.unk22C = 0;
            }
            else
            {
                gGameState.unk208 -= 4;
                if (gGameState.unk208 <= 0xC0)
                    gGameState.unk22C = 1;
            }
            DrawSprite(sprite, 0, 0xCC, 0xE, 0, mosaic, gGameState.unk208, 0, 0, 0, 0);
            frame = 1;
        talk:
            sprite = gSprite_0202C740;
            break;
        case 12:
            gGameState.unk206 = 0;
            frame = 1;
            break;
        case 13:
            gGameState.unk206 = 0;
            frame = 0;
            break;
        case 16:
            sprite = gSprite_0202C650;
            frame = 4;
            break;
        case 18:
            x = 0xA8;
        case 17:
            sprite = gSprite_0202C650;
            frame = 1;
            break;
        case 20:
            x = 0xC8;
        case 19:
            sprite = gSprite_0202C650;
            frame = 7;
            break;
        case 0:
            if (!mosaic && gGameState.unk206 != 0 && gGameState.unk206 != 0x100)
            {
                if (gGameState.unk206 > 0x100)
                    gGameState.unk206 -= 8;
                else
                    gGameState.unk206 += 8;
                if (gGameState.unk206 == 0x100)
                    gGameState.unk206 = 0;
            }
            frame = *(vu8 *)&gGameState.unk228;
            break;
        case 15:
            return;
        }
        DrawSprite(sprite, frame, x, 0x18, gGameState.unk229, 0, gGameState.unk206, 0, 0x10, 0, a10);
    }
    else
    {
        gGameState.unk22B = 0;
    }
    if (gGameState.flags.unkB_1 && gGameState.msgBoxPos == 0)
    {
        if (gGameState.unk0FF == 0)
        {
            gGameState.unk100 = (gGameState.unk100 + 1) & 3;
            gGameState.unk0FF = gSprite_0202C400[gGameState.unk100 * 8 + 4];
        }
        else
        {
            gGameState.unk0FF--;
        }
        DrawSprite(gSprite_0202C400, gGameState.unk100, 0xC8, 0x58, gGameState.flags.unk2 + 0x80, 0,
                   0x100, 0, 0, 0, 0);
    }
}
*/
INCLUDE_ASM(Func_0201CFE8);

bool32 MsgReset(void)
{
    gGameState.unk13C = 0;
    gGameState.msgBoxPos = 0;
    gGameState.unk142 = 0xFFFF;
    return TRUE;
}

void MsgStart(s16 id)
{
    if (gGameState.unk142 != id)
    {
        Func_0201C0E8(0);
        gGameState.unk142 = id;
#ifdef VERSION_GZLJ01
        gGameState.msgPtr = (u8 *)0x02000000 + ((u16 *)0x02000000)[id];
#else
        // The US message block starts with the offset of the text after the table.
        gGameState.msgPtr = (u8 *)(*(u16 *)0x02000000 + (((u16 *)0x02000002)[id] | 0x02000000));
#endif
        gGameState.unk14A = 0;
        gGameState.unk14B = 0;
        gGameState.unk150 = 0;
        gGameState.unk14F = 0xFF;
        gGameState.unk14E = 0;
        gGameState.unk151 = 0;
        gGameState.unk14C = 0;
        gGameState.unk14D = 0;
        gGameState.unk13E = 0;
        gGameState.unk158 = 0;
        gGameState.unk159 = 0;
        gGameState.unk15A = 0;
    }
}

// Play a message sound (0-0x437) with its text. Callers see an s16 parameter (the prototype
// in game.h); it is narrowed to u16 here.
bool32 GamePlaySound_(u16 id, u8 arg) asm("GamePlaySound");
bool32 GamePlaySound_(u16 id, u8 arg)
{
    u32 params[2];

    if (id <= 0x437)
    {
        if (gGameState.unk13C != 1)
        {
            u32 mask = 0xFFFF0000;
            s32 r;

            params[0] = 0;
            r = gMain.random % 3;
            if (r == 0)
                params[1] = params[1] & mask;
            else if (r == 1)
                params[1] = (params[1] & mask) | 1;
            else
                params[1] = (params[1] & mask) | 2;
            SoundPlay(0x13 - SOUND_ID_SHIFT, params);
        }
        gGameState.unk13C = 1;
        gGameState.unk1FA = 1;
        gGameState.unk226 = arg;
        gGameState.unk22B = 1;
        Func_0201C568();
        MsgStart(id);
    }
    return TRUE;
}

bool32 MsgStartSilent(s16 id)
{
    if ((u16)id <= 0x437)
    {
        gGameState.unk13C = 1;
        MsgStart(id);
    }
    return TRUE;
}

void MsgClose(void)
{
    if (gGameState.unk13C)
        SoundPlay(0x17 - SOUND_ID_SHIFT, 0);
    gGameState.unk13C = 0;
}

#ifndef VERSION_GZLJ01
// Draw a whole string at (x, y) at once (the title screen's US text): control codes only
// move the cursor.
void DrawString(const u8 *str, u8 x, u8 y, u8 lineHeight)
{
    u8 ch;

    gGameState.msgPtr = (u8 *)str;
    gGameState.textX = x;
    gGameState.textY = y;
    do
    {
        ch = *gGameState.msgPtr;
        while (ch > 0xF1)
        {
            if (ch == 0xFD || ch == 0xF4)
            {
                gGameState.textX += gGameState.unk157;
            }
            else if (ch == 0xFA || ch == 0xFE)
            {
                if (ch == 0xFE)
                {
                    gGameState.textX = x;
                    gGameState.textY += lineHeight;
                }
            }
            else if (ch == 0xF6)
            {
                gGameState.msgPtr++;
            }
            else if (ch == 0xF5)
            {
                gGameState.msgPtr++;
            }
            else if (ch == 0xF3)
            {
                gGameState.msgPtr++;
            }
            else if (ch == 0xF2)
            {
                s32 n = gGlyphRunLength[*++gGameState.msgPtr];
                s32 i;

                for (i = 0; i < n; i++)
                    gGameState.textX += gGameState.unk157;
            }
            else if (ch == 0xFC)
            {
                gGameState.msgPtr++;
            }
            ch = *++gGameState.msgPtr;
        }
        DrawChar(ch);
    } while (*++gGameState.msgPtr != 0xFF);
}
#endif

// Draw the blinking cursor on menu slot (rows 0-1: the item grid, row 2: the bottom row).
void DrawSlotCursor(u8 slot)
{
    // (row compares signed, so it is an int; the x sums only match in s16.)
    u8 col = slot % 5;
    s32 row = (u8)(slot / 5);
    bool32 mosaic = gMain.mosaic != 0;

    if (row <= 1)
        DrawSprite(gSprite_0202BDB8, (gMain.frameCount >> 4) & 1,
                   (s16)gMain.bgOfs[1][0] + (s16)(col * 24 + 24), row * 40 + 20, 0, mosaic, 0, 0, 0,
                   0, 0);
    else
        DrawSprite(gSprite_0202BDB8, (gMain.frameCount >> 4) & 1,
                   (s16)gMain.bgOfs[1][0] + (s16)(col * 16 + 0x27), 0x6C, 0, mosaic, 0, 0, 0, 0, 0);
}

// Add an item's icon as a 16x16 sprite to gOamBuffer.
void DrawItemSprite(u8 item, u8 x, u8 y, u8 mosaic)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];

    oam->y = y;
    oam->mosaic = mosaic;
    oam->shape = 0;
    oam->x = x;
    oam->hFlip = 0;
    oam->vFlip = 0;
    oam->size = 1;
    oam->tileNum = gGameState.items[item].objTile;
    oam->priority = 0;
    oam->paletteNum = gGameState.items[item].objPalette;
    gMain.unk450++;
}

// Draw an item's 2x2-tile icon at map (a blank tile for item 0).
void DrawItemIcon(u8 item, u16 *map)
{
    struct ItemSlot *slot = &gGameState.items[item];

    if (item == 0)
    {
        map[0] = 0xD09F;
        map[1] = 0xD09F;
        map[0x20] = 0xD09F;
        map[0x21] = 0xD09F;
    }
    else
    {
        map[0] = slot->tile;
        map[1] = slot->tile + 1;
        map[0x20] = slot->tile + 0x10;
        map[0x21] = slot->tile + 0x11;
    }
}

// Draw an item's count as two digits (two tile rows; blank tens digit below 10).
void DrawItemCount(u8 item, u16 *map)
{
    struct ItemSlot *slot = &gGameState.items[item];
    u8 count = slot->price;
    s32 tens = (u8)(count / 10) * 2;
    s32 ones = (u8)(count % 10) * 2;

    if (item == 0)
    {
        map[0] = 0;
        map[1] = 0;
        map[0x20] = 0;
        map[0x21] = 0;
    }
    else
    {
        if (tens != 0)
        {
            map[0] = tens + 0x4030;
            map[0x20] = tens + 0x4031;
        }
        else
        {
            map[0] = 0xD09F;
            map[0x20] = 0xD09F;
        }
        map[1] = ones + 0x4030;
        map[0x21] = ones + 0x4031;
    }
}

void SetSelectedItem(u8 item)
{
    u16 *map = (u16 *)0x06006464;

    gGameState.unk1DE = item;
    DrawItemIcon(item, map);
    map += 2;
    DrawItemCount(item, map);
}

// Put item in menu slot (5 per row, 3 tiles apart; count below the icon).
void SetSlotItem(u8 slot, u8 item)
{
    struct ItemMenu *menu = (struct ItemMenu *)gGameState.items;
    u8 col = slot % 5;
    u8 row = slot / 5;
    u16 *map = (u16 *)(0x06007000 + ((row * 40 + 8) >> 3) * 64 + ((col * 24 + 8) >> 2));

    menu->slotItems[slot] = item;
    DrawItemIcon(item, map);
    map += 0x40;
    DrawItemCount(item, map);
}

// The dungeon chest counter (MainMode1 starts it at state 1): state 2 blinks the new chest
// bits until unk1FA is clear, state 3 waits 300 frames or a key.
void ChestCounterMain(void)
{
    if (gGameState.unk1E0 == 3)
    {
        if (--gGameState.unk1CA == 0 || gMain.newKeys != 0)
        {
            DrawChestRow();
            SoundPlay(0x2A - SOUND_ID_SHIFT, 0);
            gGameState.unk1CC = 0;
            gGameState.unk13C = 0;
            gGameState.unk1E0 = 0;
            gGameState.unk104 = 1;
            gGameState.unk1CA = 0;
        }
    }
    else if (gGameState.unk1E0 == 2)
    {
        if (++gGameState.unk1CA == 0x20)
            SoundPlay(0x12 - SOUND_ID_SHIFT, 0);
        if (gGameState.unk1CA & 0x20)
        {
            if (!(gGameState.unk1CA & 0x1F))
                SoundPlay(0x25 - SOUND_ID_SHIFT, 0);
            gGameState.flags.dungeonChests |= gGameState.unk1E1;
        }
        else
        {
            gGameState.flags.dungeonChests &= ~gGameState.unk1E1;
        }
        DrawChestRow();
        if (gGameState.unk1FA == 0 && (gGameState.unk1CA & 0x20))
        {
            gGameState.unk1CA = 300;
            gGameState.unk1E0++;
        }
    }
}

#ifdef VERSION_GZLE01_ud
// This version adds a sound test (L toggles it; see docs/layout.md).
INCLUDE_ASM(ItemMenuUpdate);
#else
// Item menu: move the cursor over the 5 x 3 slots with the d-pad (with key repeat); B picks
// the item under it, and the cursor sprite slides there.
void ItemMenuUpdate(void)
{
    s32 col = (u8)(gGameState.menuCursor % 5);
    s32 row = (u8)(gGameState.menuCursor / 5);
    u16 keys = gMain.heldKeys;
    u8 item;
    struct ItemSlot *entry;

#ifdef VERSION_GZLJ01
    if (gGameState.unk1E0 != 0)
#else
    if (gGameState.unk1E0 > 1)
#endif
    {
        ChestCounterMain();
        return;
    }
    if (gGameState.unk1E5_0)
    {
        gGameState.menuCursorX += gGameState.menuCursorDX;
        gGameState.menuCursorY += gGameState.menuCursorDY;
        if (gGameState.menuCursorX == 0x9000)
        {
            gGameState.unk1E5_0 = 0;
            SetSelectedItem(gGameState.slotItems[gGameState.menuCursor]);
            SoundPlay(0x15 - SOUND_ID_SHIFT, 0);
        }
        DrawItemSprite(gGameState.slotItems[gGameState.menuCursor], gGameState.menuCursorX >> 8,
                      gGameState.menuCursorY >> 8, gMain.mosaic != 0);
        return;
    }
    if (gMain.newKeys & keys & 0xF0)
    {
        gGameState.menuKeyRepeat = 16;
        keys = gMain.newKeys;
        gGameState.unk1DF = 0;
        SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
    }
    else if (Func_02018A90(&gGameState.menuKeyRepeat))
    {
        keys = 0;
    }
    else if (keys & 0xF0)
    {
        gGameState.menuKeyRepeat = 4;
        SoundPlay(0x15 - SOUND_ID_SHIFT, 0);
    }
    if (keys & 0x20)
    {
        if (--col < 0)
            col = 4;
    }
    else if (keys & 0x10)
    {
        if (++col > 4)
            col = 0;
    }
    if (keys & 0x40)
    {
        if (--row < 0)
            row = 2;
    }
    else if (keys & 0x80)
    {
        if (++row > 2)
            row = 0;
    }
    gGameState.menuCursor = row * 5 + col;
    item = gGameState.slotItems[gGameState.menuCursor];
    if (row == 2)
    {
        if ((gGameState.flags.dungeonChests >> col) & 1)
            MsgStartSilent(gChestRowIds[col]);
        else
            gGameState.unk13C = 0;
    }
    else
    {
        if (gMain.newKeys & 2)
        {
            if (item == 0)
            {
                gGameState.unk1DF = 1;
            }
            else if (item != gGameState.unk1DE)
            {
                gGameState.menuCursorX = ((gGameState.menuCursor % 5) * 24 + 20) << 8;
                gGameState.menuCursorY = ((gGameState.menuCursor / 5) * 40 + 20) << 8;
                gGameState.menuCursorDX = (0x9000 - gGameState.menuCursorX) >> 4;
                gGameState.menuCursorDY = (0x8800 - gGameState.menuCursorY) >> 4;
                gGameState.unk1E5_0 = 1;
                SoundPlay(0x14 - SOUND_ID_SHIFT, 0);
            }
            else
            {
                SoundPlay(0x1C - SOUND_ID_SHIFT, 0);
            }
        }
        entry = &gGameState.items[item];
        if (item == 0)
            gGameState.unk13C = 0;
        else
            MsgStartSilent(entry->unk4);
    }
    DrawSlotCursor(gGameState.menuCursor);
}
#endif

// Set up the item menu: reset the item table, adjust prices and the dungeon item, and draw
// the ten slots.
bool32 ItemMenuInit(void)
{
    s32 i;

    CpuSet(gShopItemsInit, gGameState.items, 0x30);
    if (gGameState.unk138_4)
    {
        gGameState.items[3].price = 20;
        gGameState.items[4].price = 30;
        gGameState.items[5].price = 30;
    }
    if (gGameState.unk138_5)
    {
        gGameState.items[8].price = 10;
        gGameState.items[9].price = 20;
        gGameState.items[10].price = 40;
    }
    if (gGameState.flags.dungeonNo == 0)
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x1000;
        gGameState.items[11].objPalette = 9;
        gGameState.items[11].unk4 = 7;
        gGameState.unk1E5_3 = 1;
    }
    else
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x8000;
        gGameState.items[11].objPalette = 8;
        gGameState.items[11].unk4 = 6;
        gGameState.unk1E5_3 = 0;
    }
    i = 0;
    do
    {
        if (i == 6 && !gGameState.unk138_3)
        {
            SetSlotItem(6, 11);
            i = 7;
        }
        else if (!gGameState.flags.unkB_7 && (i == 4 || i > 7))
        {
            SetSlotItem(i, 0);
            i++;
        }
        else
        {
            SetSlotItem(i, i + 1);
            i++;
        }
    } while (i <= 9);
    if (gGameState.unk1DE == 0)
        gGameState.unk1DE = 2;
    SetSelectedItem(gGameState.unk1DE);
    UpdateHudPanels();
    DrawChestRow();
    gGameState.unk1CC = 0;
    gGameState.menuCursor = 0;
    gGameState.unk1E5_0 = 0;
    return TRUE;
}

// Draw the dungeon chest row of the item menu (a 2x2 tile icon per flags.dungeonChests bit).
// Nearly matches, but the original copies the running tile value to r0 before each store
// (`adds r0, r1, #0; strh r0, ...`) and loads it in both arms of the if (`b` over the pool),
// which neither u16, u32 nor s32 `tile` reproduces:
//     void DrawChestRow(void)
//     {
//         u16 *top = (u16 *)0x06007306;
//         s32 i;
//         u16 *bottom = (u16 *)0x06007346;
//         u32 tile;
//
//         for (i = 0; i <= 4; i++)
//         {
//             if ((gGameState.flags.dungeonChests >> i) & 1)
//                 tile = 0xB0C6;
//             else
//                 tile = 0xC0C2;
//             top[0] = tile;
//             top[1] = tile + 1;
//             bottom[0] = tile + 0x10;
//             bottom[1] = tile + 0x11;
//             bottom += 2;
//             top += 2;
//         }
//     }
INCLUDE_ASM(DrawChestRow);

bool32 SendItemUse(u8 item, u8 arg)
{
    GBACOM_BUF_ITEM[0] = item;
    GBACOM_BUF_ITEM[1] = arg;
    if (GbaComSendDataSet(GBACOM_BUF_ITEM, 2, GBACOM_SLOT_ITEM, *(u32 *)GBACOM_BUF_ITEM))
        return TRUE;
    return FALSE;
}

// Use the item in menu entry `item` (16: from the other button?). Returns FALSE if it can't be
// used right now.
// Nearly matches: the register allocation (item in r4, the entry in r5 and &items in r6)
// only comes out right when the marker test is written relative to gGameState.items, e.g.
// `((u8 *)gGameState.items)[FindMarker(1) * 8 - 0xF6]`, and even then items - 0xF6 is
// computed before adding the index instead of after (`adds r0, r6, r0; subs r0, #0xf6`).
//     bool32 UseItem(u8 item)
//     {
//         struct ItemSlot *entry = &gGameState.items[item];
//         u8 price;
//
//         if (item == 0)
//             return FALSE;
//         if (!gMain.linked)
//             return TRUE;
//         if (gGameState.markers[FindMarker(1)].unk6 != 0
//          && ((item >= 3 && item <= 5) || item == 8 || item == 9 || item == 10 || item == 16))
//         {
//             if (item == 16)
//                 GamePlaySound(0x1F, 1);
//             else
//                 GamePlaySound(0xE, 1);
//             return FALSE;
//         }
//         if (item == 6)
//         {
//             GamePlaySound(0xD, 1);
//         }
//         else if (item == 11)
//         {
//             gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0x88;
//             SetSelectedItem(gGameState.unk1DE);
//             Func_0201FCA0();
//         }
//         else if (item == 1)
//         {
//             if (gMain.unk457 == 0)
//             {
//                 if (MarkerDistance(1) <= 24)
//                 {
//                     GamePlaySound(0xC, 1);
//                 }
//                 else if (gGameState.unk058 == 0)
//                 {
//                     u8 i = FindMarker(3);
//
//                     gGameState.unk058 = 3;
//                     gGameState.unk054 = gGameState.markers[i].x;
//                     gGameState.unk056 = gGameState.markers[i].y;
//                     SoundPlay(0xB - SOUND_ID_SHIFT, 0);
//                 }
//                 else
//                 {
//                     GamePlaySound(0x1A6, 1);
//                 }
//             }
//             else
//             {
//                 GamePlaySound(0xE, 1);
//             }
//         }
//         else if (item >= 3 && item <= 4)
//         {
//             if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
//                 goto no_money;
//             SendItemUse(item == 3 ? 0x11 : 0x12, 0);
//         }
//         else if (item >= 8 && item <= 10)
//         {
//             if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
//             {
//                 GamePlaySound(0xF, 1);
//                 SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
//             }
//             else
//             {
//                 gGameState.effect[0] = item;
//                 Func_0201F878();
//             }
//         }
//         else
//         {
//             if (item == 5 && gGameState.rupees - gGameState.rupeeDelta < entry->price)
//             {
//             no_money:
//                 GamePlaySound(0xF, 1);
//                 SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
//                 return FALSE;
//             }
//             if (item == 16 || item == 5)
//                 price = 0;
//             else
//                 price = entry->price;
//             SendItemUse(item, price);
//         }
//         return TRUE;
//     }
INCLUDE_ASM(UseItem);

void Func_0201E584(void)
{
    if (gGameState.unk21E == 0)
    {
        if (gGameState.unk0FD == 0)
        {
            SetSelectedItem(gGameState.unk1DE);
        }
        else
        {
            u16 tile = 0xB080;
            u16 *map = (u16 *)0x06006464;
            s32 i;

            for (i = 0; i < 4; i++)
            {
                map[0] = tile;
                map[0x20] = tile + 0x10;
                tile++;
                map++;
            }
        }
    }
}

// Slide the two HUD panels at the top of the screen in and out (unk21F/unk21D: the left one,
// unk220/unk21E: the right one with the held item and its price) depending on the game's
// state, and draw them.
// Nearly matches. The conditions of the second `else if` (unk220) come out with one jump
// threaded differently (the original re-tests dungeonNo and unk224 after the unk224 test
// fails), and in cases 0 and 1 the original keeps the digit doubled ((u8)x * 2, in r3, the
// same register as the tile in cases 2 and 3); writing it that way shifts the register
// allocation of the whole function.
//     void UpdateHudPanels(void)
//     {
//         u16 *map;
//         u16 tile;
//         s32 i;
//
//         if (gGameState.unk224 != 0)
//             gGameState.unk0FC = 2;
//         else if (gGameState.unk11A != 0)
//             gGameState.unk0FC = 0;
//         else
//             gGameState.unk0FC = 1;
//
//         if (gGameState.unk21F == 0)
//         {
//             if (gMain.modeState != 2 || !gGameState.unk1E5_1
//              || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.unk104 == 0)
//              || gGameState.unk1CC != 0
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy != 0
//                  && gGameState.unk104 == 0))
//                 gGameState.unk21F = 1;
//         }
//         else if (gGameState.unk1E5_1 && gGameState.unk1CC == 0 && gMain.modeState == 2)
//         {
//             if (((gGameState.flags.dungeonNo != 0 || gGameState.unk224 == 0) && gGameState.flags.busy == 0
//                  && gGameState.unk104 != 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 != 0))
//                 gGameState.unk21F = 0;
//         }
//
//         if (gGameState.unk220 == 0)
//         {
//             if (gGameState.unk1CC == 0)
//             {
//                 if (gMain.modeState != 2 || (!gGameState.unk1E5_1 && gGameState.unk1DE != 11)
//                  || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.unk104 == 0)
//                  || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy != 0
//                      && gGameState.unk104 == 0)
//                  || gGameState.unk121 != 0)
//                     gGameState.unk220 = 1;
//             }
//         }
//         else if (gGameState.unk1E5_1 && gMain.modeState == 2)
//         {
//             if ((gGameState.flags.dungeonNo != 0 && gGameState.flags.busy == 0 && gGameState.unk104 != 0
//                  && gGameState.unk121 == 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy == 0
//                  && gGameState.unk104 != 0 && gGameState.unk121 == 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 != 0)
//              || gGameState.unk1CC != 0)
//                 gGameState.unk220 = 0;
//         }
//
//         if (gGameState.unk21F != 0)
//         {
//             if (gGameState.unk21D <= 3)
//             {
//                 gGameState.unk21D++;
//                 map = (u16 *)0x06006440 + (0x1C - gGameState.unk21D);
//                 tile = 0xE01B;
//                 for (i = 0; i < gGameState.unk21D; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 gGameState.unk21D = 4;
//             }
//         }
//         else
//         {
//             tile = gHudPanelTiles[gGameState.unk0FC];
//             map = (u16 *)0x06006470;
//             if (gGameState.unk21D != 0)
//             {
//                 gGameState.unk21D--;
//                 for (i = 4; i != gGameState.unk21D; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//                 tile = 0xE01B;
//                 for (; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 for (i = 0; i < 4; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//             }
//         }
//
//         if (gGameState.unk220 != 0)
//         {
//             if (gGameState.unk21E <= 3)
//             {
//                 gGameState.unk21E++;
//                 map = (u16 *)0x06006440 + (0x16 - gGameState.unk21E);
//                 tile = 0xE01B;
//                 for (i = 0; i < gGameState.unk21E; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 gGameState.unk21E = 4;
//             }
//         }
//         else if (gGameState.unk21E != 0)
//         {
//             gGameState.unk21E--;
//             if (gGameState.unk0FD == 0)
//             {
//                 u8 n = gGameState.unk21E;
//                 u8 num;
//
//                 map = (u16 *)0x06006440 + (0x15 - n);
//                 switch (n)
//                 {
//                 case 0:
//                     num = gGameState.items[gGameState.unk1DE].price % 10;
//                     map[0] = num * 2 + 0x4030;
//                     map[0x20] = num * 2 + 0x4031;
//                     break;
//                 case 1:
//                     num = gGameState.items[gGameState.unk1DE].price / 10;
//                     if (num != 0)
//                     {
//                         map[0] = num * 2 + 0x4030;
//                         map[0x20] = num * 2 + 0x4031;
//                     }
//                     else
//                     {
//                         map[0] = 0xD09F;
//                         map[0x20] = 0xD09F;
//                     }
//                     break;
//                 case 2:
//                     tile = gGameState.items[gGameState.unk1DE].tile;
//                     map[0] = tile + 1;
//                     map[0x20] = tile + 1 + 0x10;
//                     break;
//                 case 3:
//                     tile = gGameState.items[gGameState.unk1DE].tile;
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     break;
//                 }
//                 map++;
//                 tile = 0xE01B;
//                 for (i = gGameState.unk21E; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 map = (u16 *)0x06006464;
//                 tile = 0xB080;
//                 for (i = 4; i != gGameState.unk21E; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//                 tile = 0xE01B;
//                 for (; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//         }
//     }
INCLUDE_ASM(UpdateHudPanels);

void Func_0201EBAC(void)
{
    gGameState.unk1E5_1 = 1;
    gGameState.unk1E5_2 = 0;
    gGameState.unk120 = 0;
}

void Func_0201EBD4(void)
{
    gGameState.unk1C8 = 180;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
    ObjSpawn(0);
}

void Func_0201EC10(void)
{
    u8 mosaic = gMain.mosaic != 0;
    u8 i;
    u8 unk;

    if (gGameState.flags.busy & 1)
        gGameState.unk1C8++;
    if (gGameState.unk1C8 == 0)
    {
        Func_0201EBAC();
        return;
    }
    i = FindMarker(3);
    gGameState.shake = gGameState.unk1C8;
    unk = gGameState.unk1EE;
    if (unk == 0)
        Func_020190A8(gSprite_0202CC60, gGameState.markers[i].x - gMain.bgOfs[3][0],
                      gGameState.markers[i].y - gMain.bgOfs[3][1], mosaic, unk);
}

void Func_0201ECC4(void)
{
    gGameState.unk12D = 0;
    gGameState.unk120 = 1;
    gGameState.unk034 = gGameState.unk032 = 0x44;
    gGameState.unk12C = gSprite_0202C950[0].duration;
    gGameState.unk12B = 1;
}

void Func_0201ED08(void)
{
    u32 mosaic = gMain.mosaic != 0;
    const struct SpriteAnimFrame *anim;

    if (gGameState.unk12C == 0)
    {
        if (gGameState.unk12D < 22)
        {
            gGameState.unk12D++;
        }
        else if (gGameState.unk12B < 2)
        {
            gGameState.unk12B = 2;
            gGameState.unk132 = 10;
            gGameState.unk13E = 300;
        }
        gGameState.unk12C = gSprite_0202C950[gGameState.unk12D].duration;
        anim = gSprite_0202C950;
    }
    else
    {
        gGameState.unk12C--;
        anim = gSprite_0202C950;
    }
    DrawSpriteAt(anim, gGameState.unk12D, gGameState.unk032, gGameState.unk034, mosaic, 0, 1);
}

void Func_0201EDC4(void)
{
    Func_0201ECC4();
    gGameState.unk12E = gSprite_0202C928[0].duration;
    gGameState.unk12F = 0;
    GamePlaySound(0x23, 1);
}

// Draw the item effect sprite (kind 0: gSprite_0202C928, else gSprite_0202C940) at a map
// position. Nearly matches, but the original sign-extends x and y again before each
// subtraction (agbcc drops that, since only the low half reaches DrawSpriteAt), and y and
// the mosaic flag swap r7/r8:
//     void Func_0201EDF8(s16 x, s16 y, u8 kind)
//     {
//         u32 mosaic = gMain.mosaic != 0;
//         const struct SpriteAnimFrame *anim;
//         u8 flip;
//
//         if (kind == 0)
//         {
//             if (gGameState.unk12E == 0)
//             {
//                 gGameState.unk12F ^= 1;
//                 gGameState.unk12E = gSprite_0202C928[gGameState.unk12F].duration;
//                 anim = gSprite_0202C928;
//             }
//             else
//             {
//                 gGameState.unk12E--;
//                 anim = gSprite_0202C928;
//             }
//             flip = 0;
//             if (gGameState.effect[0] == 12)
//                 flip = 16;
//             DrawSpriteAt(anim, gGameState.unk12F, x - gMain.bgOfs[3][0] + 3,
//                          y - gMain.bgOfs[3][1] - 24, mosaic, flip, 1);
//         }
//         else
//         {
//             if (gGameState.unk130 == 0)
//             {
//                 gGameState.unk131 ^= 1;
//                 gGameState.unk130 = gSprite_0202C940[gGameState.unk131].duration;
//                 anim = gSprite_0202C940;
//             }
//             else
//             {
//                 gGameState.unk130--;
//                 anim = gSprite_0202C940;
//             }
//             flip = 0;
//             if (gGameState.effect[0] == 12)
//                 flip = 8;
//             if (gGameState.unk131 == 0)
//                 DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] - 4,
//                              y - gMain.bgOfs[3][1], mosaic, flip, 1);
//             else
//                 DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] + 4,
//                              y - gMain.bgOfs[3][1], mosaic, flip, 1);
//         }
//     }
INCLUDE_ASM(Func_0201EDF8);

// Draw the flags.timer count (two digits) at x, y; a sound plays when it changes.
void DrawTimerCount(s16 x, s16 y)
{
    u32 mosaic = gMain.mosaic != 0;
    u8 digit;

    if (gGameState.flags.timer != gGameState.timer)
        SoundPlay(0xC - SOUND_ID_SHIFT, 0);
    digit = gGameState.flags.timer / 10;
    if (digit != 0)
        DrawSprite(gSpriteTable[10], 0, x, y, 0, mosaic, 0, 0, 0, digit + 1, 0);
    digit = gGameState.flags.timer % 10;
    DrawSprite(gSpriteTable[10], 0, x + 8, y, 0, mosaic, 0, 0, 0, digit + 1, 0);
}

void Func_0201F068(void)
{
    struct Main *main = &gMain;
    u8 notType3 = gGameState.effect[0] != 3;
    u8 price;
    u16 x;
    u16 y;

    if (gGameState.unk12B != 6)
        Func_0201ED08();
    if (gGameState.unk12B == 2)
    {
        if (gGameState.unk1FA == 0)
        {
            if (gGameState.unk132 == 0)
            {
                if (gGameState.effect[0] == 13)
                    price = gGameState.items[5].price;
                else
                    price = gGameState.items[gGameState.effect[0]].price;
                SendItemUse(gGameState.effect[0], price);
                gGameState.unk12B = 5;
            }
            else
            {
                gGameState.unk132--;
            }
        }
    }
    else if (gGameState.unk12B == 5)
    {
        if (gGameState.flags.timer != 0)
        {
            gGameState.unk120 = 0;
            gGameState.unk12B = 6;
        }
    }
    else if (gGameState.unk12B == 6)
    {
        u8 i = FindMarker(1);

        Func_0201EDF8(gGameState.markers[i].x, gGameState.markers[i].y, notType3);
        if (gGameState.flags.timer == 0)
        {
            gGameState.unk120 = 1;
            gGameState.unk1C8 = 120;
            gGameState.unk12B = 7;
            gGameState.unk032 = gGameState.markers[i].x - main->bgOfs[3][0];
            gGameState.unk034 = gGameState.markers[i].y - main->bgOfs[3][1];
            if (gGameState.unk1DE == 5)
                GamePlaySound(0x30, 1);
            else if (!notType3)
                GamePlaySound(0x2E, 1);
            else
                GamePlaySound(0x2F, 1);
        }
        else
        {
            if (!notType3)
            {
                x = gGameState.markers[i].x - main->bgOfs[3][0] + 4;
                y = gGameState.markers[i].y - main->bgOfs[3][1] - 36;
            }
            else
            {
                x = gGameState.markers[i].x - main->bgOfs[3][0] + 4;
                y = gGameState.markers[i].y - main->bgOfs[3][1] - 16;
            }
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 7)
    {
        if (gGameState.unk1C8 == 0)
        {
            gGameState.unk120 = 0;
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
    }
}

void Func_0201F240(void)
{
    Func_0201ECC4();
    gGameState.unk130 = gSprite_0202C940[0].duration;
    gGameState.unk131 = 0;
    GamePlaySound(0x25, 1);
}

void Func_0201F274(void)
{
    gGameState.unk1C8 = 120;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

void Func_0201F2A8(void)
{
    if (gGameState.unk1C8 == 0)
        Func_0201EBAC();
}

void Func_0201F2C4(void)
{
    Func_0201ECC4();
    if (gGameState.effect[1] == 13)
    {
        gGameState.effect[0] = gGameState.effect[1];
        GamePlaySound(0x29, 1);
    }
    else if (gGameState.effect[1] == 12)
    {
        GamePlaySound(0x2A, 1);
    }
    else
    {
        GamePlaySound(0x2B, 1);
    }
}

// Item effect 5.
void Effect5(void)
{
    Func_0201ED08();
    if (gGameState.unk12B == 2)
    {
        if (gGameState.unk1FA == 0)
        {
            if (gGameState.unk132 == 0)
            {
                if (gGameState.effect[1] == 10)
                {
                    GamePlaySound(0x2C, 1);
                    gGameState.effect[0] = gGameState.effect[1];
                    gGameState.unk120 = 0;
                    gGameState.unk132 = 8;
                    gGameState.unk12B = 3;
                }
                else if (gGameState.effect[1] == 0x13)
                {
                    SendItemUse(0x13, gGameState.items[5].price);
                    gGameState.unk12B = 5;
                }
                else
                {
                    gGameState.effect[0] = gGameState.effect[1];
                    Func_0201FEEC();
                    gGameState.unk12B = 3;
                }
            }
            else
            {
                gGameState.unk132--;
            }
        }
    }
    else if (gGameState.unk12B == 5)
    {
    }
    else if (gGameState.unk12B == 6)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            gGameState.unk120 = 0;
            GamePlaySound(0x30, 1);
            Func_0201EBAC();
        }
    }
}

void Func_0201F420(void)
{
    if (gMain.random & 1)
        GamePlaySound(0x3A, 14);
    else
        GamePlaySound(0x3B, 14);
    gGameState.unk1E2 = 0;
}

// Item effect 7, the shop: gGameState.unk1E2 steps through choosing an item, sending the purchase request
// (slot 12) and waiting for the GC's answer (slot 13).
void Effect7(void)
{
    u16 item;
    u8 what, result;

    if (gGameState.flags.unk3_2)
    {
        Func_0201EBAC();
        return;
    }
    if (gGameState.unk1E2 == 0)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.unk13C = 0;
            if ((*(u32 *)gGameState.effect & 0xFFFFFF00) == 0)
            {
                GamePlaySound(0x3C, 14);
                gGameState.unk1E2 = 4;
            }
            else if (gGameState.effect[1])
            {
                GamePlaySound(0x40, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[2])
            {
                GamePlaySound(0x3F, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[3])
            {
                GamePlaySound(0x3E, 14);
                gGameState.unk1E2 = 1;
            }
            else
            {
                GamePlaySound(0x3C, 14);
                gGameState.unk1E2 = 4;
            }
        }
    }
    else if (gGameState.unk1E2 == 1)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk142 == 0x3E)
            {
                if (gGameState.unk14F == 1)
                    gGameState.unk1E2 = 5;
                else
                    gGameState.unk1E2 = 2;
            }
            else if (gGameState.unk142 == 0x3F || gGameState.unk142 == 0x40)
            {
                if (gGameState.unk14F == 2)
                    gGameState.unk1E2 = 5;
                else
                    gGameState.unk1E2 = 2;
            }
        }
    }
    else if (gGameState.unk1E2 == 2)
    {
        item = gGameState.unk142;
        if (item >= 0x41 && item <= 0x45)
        {
            if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
            {
                if (gGameState.unk14F == 0)
                {
                    if (gGameState.unk142 == 0x41)
                        what = 0;
                    else if (gGameState.unk142 == 0x43)
                        what = 1;
                    else
                        what = 2;
                    GBACOM_BUF_SHOP[0] = gShopRequests[what * 3];
                    GBACOM_BUF_SHOP[1] = gShopRequests[what * 3 + 1];
                    GBACOM_BUF_SHOP[2] = gShopRequests[what * 3 + 2];
                    GbaComSendDataSet(GBACOM_BUF_SHOP, 4, GBACOM_SLOT_SHOP, *(u32 *)GBACOM_BUF_SHOP);
                    gGameState.unk1E2 = 3;
                    gMain.newKeys ^= 1;
                }
                else
                {
                    gGameState.unk13C = 0;
                    GamePlaySound(0x3D, 1);
                    gGameState.unk1E2 = 5;
                }
            }
        }
    }
    else if (gGameState.unk1E2 == 3)
    {
        if (GbaComRecvStatusCheck(GBACOM_SLOT_ITEM_BUY))
        {
            what = GBACOM_BUF_ITEM_BUY[0];
            result = GBACOM_BUF_ITEM_BUY[1];
            GbaComDataStatusReset(GBACOM_SLOT_ITEM_BUY);
            if (result == 0)
            {
                gGameState.unk13C = 0;
                GamePlaySound(0x48, 14);
                gGameState.unk1E2 = 4;
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
            }
            else if (result == 1)
            {
                gMain.newKeys |= 1;
                gGameState.unk1E2 = 4;
            }
            else if (result == 2)
            {
                gGameState.unk13C = 0;
                gGameState.unk1E2 = 4;
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                if (what == 0)
                    GamePlaySound(0x44, 14);
                else if (what == 1)
                    GamePlaySound(0x42, 14);
                else
                    GamePlaySound(0x49, 14);
            }
            else
            {
                gGameState.unk13C = 0;
                GamePlaySound(0x46, 14);
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                gGameState.unk1E2 = 4;
            }
        }
        else
        {
            gMain.newKeys &= 0xFF3E;
        }
    }
    else if (gGameState.unk1E2 == 4)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.unk13C = 0;
            GamePlaySound(0x3D, 1);
            gGameState.unk1E2 = 5;
        }
    }
    else
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
            Func_0201EBAC();
    }
}

void Func_0201F878(void)
{
    Func_0201ECC4();
    gGameState.unk1E5_1 = 0;
    gGameState.unk1E5_2 = 1;
    gGameState.unk12E = gSprite_0202C928[0].duration;
    gGameState.unk12F = 0;
    GamePlaySound(0x27, 1);
}

// Draws the title sprite at (x, y) on BG3's map, bobbing; kind 1 adds gSprite_0202C490 with a
// palette per effect, kind 2 flips it and adds gSprite_0202C928 at (unk036, unk038).
// Doesn't match: the original keeps x, y and bob in registers (spilling copies of kind and
// mosaic instead) and loads BG3's scroll as a folded gMain+0x432 constant in the first and
// last calls; this keeps the address in a register:
//     s16 bob = gBobOffsets_020244DA[(gMain.frameCount & 0x30) >> 4];
//     u8 mosaic = gMain.mosaic != 0;
//     u8 flags = 0;
//     u8 palette;
//
//     if (kind == 2)
//         flags = 8;
//     DrawSpriteAt(gSprite_0202C938, 0, (s16)x - gMain.bgOfs[3][0] + bob,
//                  (s16)y - gMain.bgOfs[3][1], mosaic, flags, 1);
//     if (kind == 1)
//     {
//         if (gGameState.effect[0] == 8)
//             palette = 8;
//         else if (gGameState.effect[0] == 9)
//             palette = 15;
//         else
//             palette = 4;
//         DrawSprite(gSprite_0202C490, 0, (s16)x - gMain.bgOfs[3][0] + (bob + 11),
//                    (s16)y - gMain.bgOfs[3][1] + 26, 0xC0, mosaic, 0, 0, 0, 0, palette);
//     }
//     else if (kind == 2)
//     {
//         DrawSpriteAt(gSprite_0202C928, 0, gGameState.unk036 - gMain.bgOfs[3][0] + bob,
//                      gGameState.unk038 - gMain.bgOfs[3][1], mosaic, 16, 1);
//     }
INCLUDE_ASM(Func_0201F8C4);

// Item effects 8-10: the title sprite drops in over the type 1 marker and sends the item use.
// Doesn't match by one instruction: in state 2 the original stores a fresh 0 to unk120 where this
// reuses the register holding unk132 (known to be 0):
//     void Effect8(void)
//     {
//         struct Main *main = &gMain;
//         s32 m = FindMarker(1);
//         u8 idx = gGameState.effect[0] - 8;
//         u16 x, y;
//         u8 arg;
//
//         if (gGameState.unk12B <= 3)
//             Func_0201ED08();
//         if (gGameState.unk12B == 2)
//         {
//             if (gGameState.unk1FA == 0)
//             {
//                 if (gGameState.unk132 == 0)
//                 {
//                     gGameState.unk120 = 0;
//                     gGameState.unk132 = 8;
//                     gGameState.unk12B = 3;
//                     gGameState.flags.isFree = 0;
//                     gGameState.unk1E4 = gGameState.effect[0];
//                     SendItemUse(0x14, 0);
//                 }
//                 else
//                 {
//                     gGameState.unk132--;
//                 }
//             }
//         }
//         else if (gGameState.unk12B == 3)
//         {
//             if (gGameState.unk132 == 0)
//             {
//                 if (gGameState.unk134 == 0)
//                 {
//                     gGameState.unk034 = main->bgOfs[3][1] - 0x30;
//                     gGameState.unk12B = 4;
//                 }
//             }
//             else
//             {
//                 gGameState.unk132--;
//                 gGameState.flags.isFree = 0;
//             }
//         }
//         else if (gGameState.unk12B == 4)
//         {
//             if (main->frameCount & 1)
//                 gGameState.unk034++;
//             if (gGameState.markers[m].y - gGameState.unk034 <= 0x18)
//             {
//                 gGameState.unk132 = (idx + 2) * 60;
//                 gGameState.unk034 = gGameState.markers[m].y - 0x18;
//                 gGameState.unk12B = 6;
//                 GamePlaySound(0x28, 1);
//             }
//             Func_0201F8C4(gGameState.markers[m].x - 0x18, gGameState.unk034, 1);
//         }
//         else if (gGameState.unk12B == 6)
//         {
//             x = gGameState.markers[m].x - 0x18;
//             y = gGameState.markers[m].y - 0x18;
//             Func_0201F8C4(x, y, 1);
//             if (gGameState.unk132 == 0)
//             {
//                 if (gGameState.unk1DE == 5)
//                     arg = gGameState.items[5].price;
//                 else
//                     arg = gGameState.items[idx + 8].price;
//                 SendItemUse(idx + 8, arg);
//                 gGameState.unk034 = y;
//                 gGameState.unk12B = 5;
//             }
//             else
//             {
//                 SoundPlayLoop(1, 1, 0);
//                 if (--gGameState.unk132 == 10)
//                     gGameState.unk13E = 300;
//             }
//         }
//         else if (gGameState.unk12B == 5)
//         {
//             x = gGameState.markers[m].x - 0x18;
//             y = gGameState.markers[m].y - 0x18;
//             Func_0201F8C4(x, y, 1);
//         }
//         else if (gGameState.unk12B == 7)
//         {
//             gGameState.unk034--;
//             Func_0201F8C4(gGameState.markers[m].x - 0x18, gGameState.unk034, 0);
//             if (gGameState.unk034 < (s16)main->bgOfs[3][1] - 0x30)
//             {
//                 gGameState.unk12B = 0;
//                 if (gGameState.unk1DE == 5)
//                     GamePlaySound(0x30, 1);
//                 else
//                     GamePlaySound(gGameState.effect[0] + 0x10, 1);
//                 Func_0201EBAC();
//             }
//         }
//     }
//
INCLUDE_ASM(Effect8);

void Func_0201FCA0(void)
{
    if (!gGameState.unk1E5_3)
        GamePlaySound(0xD2, 1);
    else
        GamePlaySound(0xDB, 1);
    gGameState.effect[0] = 11;
    gGameState.unk1E5_2 = 1;
    gGameState.unk1E5_1 = 0;
    gGameState.unk1E2 = 0;
}

// Item effect 11.
void Effect11(void)
{
    if (gGameState.unk1E2 == 0)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk14F == 0)
                gGameState.unk1E2 = 2;
            else if (gGameState.unk14F == 1)
                gGameState.unk1E2 = 1;
            else
                gGameState.unk1E2 = 4;
        }
    }
    else if (gGameState.unk1E2 == 1)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
            gGameState.unk1E2 = 2;
    }
    else if (gGameState.unk1E2 == 2)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            GamePlaySound(0xD8, 1);
            gGameState.unk1E2 = 3;
        }
    }
    else if (gGameState.unk1E2 == 3)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk14F == 1)
            {
                gMain.newKeys ^= 1;
                if (!gGameState.unk1E5_3)
                    GamePlaySound(0xD4, 1);
                else
                    GamePlaySound(0xDD, 1);
                gGameState.unk1E2 = 1;
            }
            else
            {
                gGameState.unk1E2 = 4;
            }
        }
    }
    else
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0xAE;
            SetSelectedItem(gGameState.unk1DE);
            Func_0201EBAC();
        }
    }
}

void Func_0201FEEC(void)
{
    gGameState.unk132 = 8;
    gGameState.unk120 = 0;
    gGameState.unk12B = 4;
}

// Item effect 12.
void Effect12(void)
{
    struct Main *main = &gMain;
    s32 m = FindMarker(1);
    u16 x, y;

    if (gGameState.unk12B == 3)
    {
        if (gGameState.unk132 == 0)
        {
            if (gGameState.unk134 == 0)
            {
                gGameState.unk038 = gGameState.unk034 = main->bgOfs[3][1] - 0x30;
                gGameState.unk12B = 4;
            }
        }
        else
        {
            gGameState.unk132--;
        }
    }
    else if (gGameState.unk12B == 4)
    {
        if (main->frameCount & 1)
        {
            gGameState.unk034++;
            gGameState.unk038++;
        }
        if (gGameState.markers[m].y - gGameState.unk034 <= 0x18)
        {
            gGameState.unk034 = gGameState.markers[m].y - 0x18;
            gGameState.unk038 = gGameState.markers[m].y - 0x18;
            SendItemUse(gGameState.effect[0], gGameState.items[5].price);
            gGameState.unk12B = 5;
        }
        x = gGameState.markers[m].x - 0x15;
        gGameState.unk036 = gGameState.markers[m].x + 0x11;
        Func_0201F8C4(x, gGameState.unk034, 2);
    }
    else if (gGameState.unk12B == 5)
    {
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (gGameState.flags.timer)
        {
            gGameState.unk1C8 = 120;
            gGameState.unk12B = 6;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 6)
    {
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (!gGameState.flags.timer)
        {
            gGameState.unk032 = gGameState.markers[m].x - 0x15;
            gGameState.unk036 = gGameState.markers[m].x + 0x11;
            gGameState.unk034 = gGameState.markers[m].y - 0x18;
            gGameState.unk038 = gGameState.markers[m].y - 0x18;
            gGameState.unk12B = 7;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 7)
    {
        gGameState.unk034--;
        gGameState.unk038--;
        gGameState.unk032 = gGameState.markers[m].x - 0x15;
        gGameState.unk036 = gGameState.markers[m].x + 0x11;
        Func_0201F8C4(gGameState.unk032, gGameState.unk034, 2);
        if (gGameState.unk034 < (s16)main->bgOfs[3][1] - 0x30)
        {
            GamePlaySound(0x30, 1);
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
        if (gGameState.unk038 < (s16)main->bgOfs[3][1] - 0x30)
            gGameState.unk038 = main->bgOfs[3][1] - 0x30;
    }
}

void Func_02020150(void)
{
    GamePlaySound(0x50, 1);
    gGameState.unk1C8 = 300;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

// Item effect 14: a countdown (unk1C8) that ticks faster near the end, drawn at the type 3
// marker; at 0 it switches to effect 2.
void Effect14(void)
{
    u8 mosaic = gMain.mosaic != 0;
    s32 m;

    if (gGameState.flags.busy & 1)
        gGameState.unk1C8++;
    if (gGameState.unk1EE == 0)
    {
        gGameState.unk13E = 0;
        if (gGameState.unk1C8 > 179)
        {
            if ((u16)(gGameState.unk1C8 % 30) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.unk1C8 > 59)
        {
            if ((u16)(gGameState.unk1C8 % 20) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.unk1C8 == 0)
        {
            gGameState.unk13E = 300;
            gGameState.effect[0] = 2;
            Func_0201EBD4();
        }
        else if ((u16)(gGameState.unk1C8 % 10) == 0)
        {
            SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        m = FindMarker(3);
        Func_020190A8(gSprite_0202CC50, gGameState.markers[m].x - gMain.bgOfs[3][0],
                      gGameState.markers[m].y - gMain.bgOfs[3][1], mosaic, 0);
    }
}

// Run the current item effect (effect[0], from slot 7) while one is active.
void UpdateItemEffect(void)
{
    if (gGameState.unk1E5_2)
    {
        if (!gGameState.unk1E5_1)
        {
            if (gGameState.unk1C8 != 0)
                gGameState.unk1C8--;
            gItemEffectFuncs[gGameState.effect[0]]();
        }
    }
}

// Link lost: load the title graphics with the "link lost" BG map and fade in (state 1).
void Mode2State0(void)
{
    u8 blend;

    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x1100;  // BG0 and OBJ
    gMain.bgcnt[0] = 0x4C08;
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    LZ77UnCompVram(gMode2BgMap, (void *)0x06006000);
#ifndef VERSION_GZLJ01
    Func_0201BAD8();
    gGameState.msgPtr = MSG_DATA + *(u16 *)(MSG_DATA + *(u16 *)MSG_DATA + 0x3E);
    gGameState.unk14A = 6;
    gGameState.unk14B = 0;
    gGameState.unk157 = 6;
    gGameState.unk15A = 0;
    DrawString(gMode2UsString, 0, 0, 8);
#endif
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.hblankScroll = 0;
    gMode0Work.blend = blend = 0;
    gMain.bldcnt = 0x2071;  // BG0 over BG0-3, OBJ and backdrop
    gMain.bldalpha = ((16 - blend) << 8) | blend;
    gGameState.flagsReceived = 0;
    gGameState.unk23C = 1;
    gMode0Work.timer = 1;
    gMode0Work.fadingOut = 0;
    gMain.modeState++;
}

// Pulse the blend (between 6 and 16, holding 3 s at full) until the link is back, then
// restart mode 1.
void Mode2State1(void)
{
    SoundPlayLoop(2, 2, 0);
    if (--gMode0Work.timer == 0)
    {
        gMode0Work.timer = 2;
        if (gMode0Work.fadingOut == 0)
        {
            gMode0Work.blend++;
            if (gMode0Work.blend == 16)
            {
                gMode0Work.timer = 180;
                gMode0Work.fadingOut ^= 1;
            }
        }
        else
        {
            gMode0Work.blend--;
            if (gMode0Work.blend == 6)
                gMode0Work.fadingOut ^= 1;
        }
        gMain.bldalpha = ((16 - gMode0Work.blend) << 8) | gMode0Work.blend;
    }
    if (gMain.linked)
    {
        gMain.mode = 1;
        gMain.modeState = 0;
    }
    DrawSpriteAt(gSprite_0202C848, 0, 0xD0, 0x80, 0, 0, 0);
}

void MainMode2(void)
{
    gMode2StateFuncs[gMain.modeState]();
}

// Slow vx toward 0 by decel (it can overshoot) and move x.
void ObjDecelX(struct Obj *obj, s16 decel)
{
    if (obj->vx >= 0)
        decel = -decel;
    if (obj->vx != 0)
        obj->vx += decel;
    obj->x += obj->vx;
}

// Slow vy toward 0 by decel (it can overshoot) and move y.
void ObjDecelY(struct Obj *obj, s16 decel)
{
    if (obj->vy >= 0)
        decel = -decel;
    if (obj->vy != 0)
        obj->vy += decel;
    obj->y += obj->vy;
}

// While in the air (z < 0 or moving), accelerate vz by gravity and land at z = 0.
void ObjFall(struct Obj *obj, s16 gravity)
{
    if (obj->vz | obj->z)
    {
        obj->vz += gravity;
        obj->z += obj->vz;
        if (obj->z >= 0)
        {
            obj->vz = 0;
            obj->z = 0;
        }
    }
}

void ObjReset(struct Obj *obj)
{
    obj->x = 0;
    obj->y = 0;
    obj->z = 0;
    obj->vx = 0;
    obj->vy = 0;
    obj->vz = 0;
    obj->unk12 = 0;
    obj->unk14 = 0;
    obj->unk16 = 0;
    obj->unk18 = 0;
    obj->unk1A = 0;
    obj->unk1C = 0;
    obj->unk1E = 0;
    obj->active = 0;
    obj->type = 0;
}

// Run every active object's type function.
void ObjUpdateAll(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            gObjFuncs[gObjs[i].type](&gObjs[i]);
    }
}

// Index of the first free object, or -1.
s32 ObjFindFree(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (!gObjs[i].active)
            return i;
    }
    return -1;
}

// Start a free object of the given type (its function sees active = 1 and sets it up).
bool32 ObjSpawn(u8 type)
{
    s32 i = ObjFindFree();

    if (i != -1)
    {
        gObjs[i].type = type;
        gObjs[i].active = 1;
        gObjFuncs[gObjs[i].type](&gObjs[i]);
        return TRUE;
    }
    return FALSE;
}

void ObjResetActive(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            ObjReset(&gObjs[i]);
    }
}

// Place this piece and up to four more at the first type 3 marker, flying out in a fan.
void ObjBurstInit(struct Obj *obj)
{
    s16 ofsX = gObjBurstOfsX[obj->unk1C];
    s16 ofsY = gObjBurstOfsY[obj->unk1C];
    s32 i = 0;
    s32 idx;
    s32 j;

    do
    {
        if (gGameState.markers[i].type == 3)
            break;
        i++;
    } while (i < gGameState.markerCount);

    obj->x = (gGameState.markers[i].x << 8) + ofsX;
    obj->y = (gGameState.markers[i].y << 8) + ofsY;
    obj->unk1A = obj->unk18 = 0;
    obj->vx = gObjBurstVelX[obj->unk1C];
    obj->vy = gObjBurstVelY[obj->unk1C];
    obj->vz = -0x100;
    obj->unk1E = 0;
    obj->unk12 = 2;
    obj->unk16 = 0x180;
    obj->active++;

    for (j = 1; j <= 4; j++)
    {
        idx = ObjFindFree();
        if (idx != -1)
        {
            // BUG: indexes from obj, not gObjs (the same when obj is gObjs[0])
            obj[idx].unk1C = j;
            ofsX = gObjBurstOfsX[obj[idx].unk1C];
            ofsY = gObjBurstOfsY[obj[idx].unk1C];
            obj[idx].type = 0;
            obj[idx].active = 1;
            obj[idx].x = (gGameState.markers[i].x << 8) + ofsX;
            obj[idx].y = (gGameState.markers[i].y << 8) + ofsY;
            obj[idx].unk1A = obj[idx].unk18 = 0;
            obj[idx].vx = gObjBurstVelX[obj[idx].unk1C];
            obj[idx].vy = gObjBurstVelY[obj[idx].unk1C];
            obj[idx].vz = -0x100;
            obj[idx].unk1E = 0;
            obj[idx].unk12 = 2;
            obj[idx].unk16 = 0x180;
            obj[idx].active = 2;
        }
    }
}

// Fly, spinning, and scale with the vertical speed; vanish on landing.
void ObjBurstFly(struct Obj *obj)
{
    ObjDecelX(obj, 0);
    ObjDecelY(obj, 0);
    ObjFall(obj, 5);
    if (obj->z == 0)
        ObjReset(obj);
    obj->unk1E -= 16;
    if (obj->vz < 0)
        obj->unk16 = 0x80 - obj->vz;
    else
        obj->unk16 = obj->vz + 0x80;
}

// Object type 0: pieces flying out of the type 3 marker (spawned by Func_0201EBD4).
void ObjBurst(struct Obj *obj)
{
    u8 mosaic = (gMain.bgcnt[0] >> 6) & 1;
    s32 x, y;

    if (!(gGameState.flags.busy & 1))
        gObjBurstStateFuncs[obj->active - 1](obj);
    if (obj->active)
    {
        x = (obj->x >> 8) - gMain.bgOfs[3][0];
        y = ((obj->y + obj->z) >> 8) - gMain.bgOfs[3][1];
        DrawSprite(gSprite_0202C150, 0, x, y, obj->unk1E, mosaic, obj->unk16, obj->unk18,
                   obj->unk1A, 0, 0);
    }
}

// Fill a per-line BG0HOFS table: BG0's scroll plus a sine wave of the given amplitude (8.8),
// advancing the phase by 4 per line.
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude)
{
    s32 i;
    const s16 *sin;
    u16 hofs;

    i = 160;
    sin = gSinTable;
    hofs = *gMain.bgOfs[0];
    for (; i != 0; i--)
    {
        *table = hofs + ((sin[phase] * amplitude) >> 8);
        phase += 4;
        table++;
    }
}

void SelectWindowWipe(u8 open)
{
    s16 step;

    if (open)
        step = 16;
    else
        step = -16;
    gMain.winin = 0x1F;
    gGameState.unk1E8 = gGameState.unk1E8 + step;
    if (gGameState.unk1E8 >= 0xE8 && gGameState.unk1E8 <= 0xF8)
    {
        gMain.win0h = 0xEF;
    }
    else if (gGameState.unk1E8 + 8 == 0)
    {
        gMain.win0h = 0;
        gMain.winin = 0;
    }
    else
    {
        gMain.win0h = gGameState.unk1E8 + 8;
    }
    gMain.win0v = 0x9F;
    gMain.winout = 0;
    gMain.dispcnt |= 0x2000;
}

void SelectWindowDrawEdge(void)
{
    DrawSprite(gSprite_0202BD78, 0, gGameState.unk1E8, gGameState.unk1EA, 0, 0, 0, 0, 0, 0, 0);
}

// Select (in mode 1 state 2, when nothing else is up) wipes a window across the screen
// (SelectWindowWipe), flips BG0 to its other half behind it and wipes it back; Select again (or
// leaving state 2) reverses it. gGameState.unk1EE is the step.
// Doesn't match: in states 3 and 4 the registers for three offset constants differ.
//     void UpdateSelectWindow(void)
//     {
//         switch (gGameState.unk1EE)
//         {
//         case 0:
//             if (gMain.modeState == 2 && gGameState.unk1FA == 0 && gGameState.unk1CC == 0
//              && !gGameState.unk1E5_4 && (gMain.newKeys & 4))
//             {
//                 gGameState.unk1FB = 1;
//                 SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
//                 gGameState.unk1EE++;
//             }
//             break;
//         case 1:
//             SelectWindowWipe(0);
//             if (gGameState.unk1E8 == 0xFFF8)
//             {
//                 gGameState.unk1EE++;
//                 gMain.bgOfs[0][0] = 0x100;
//             }
//             SelectWindowDrawEdge();
//             break;
//         case 2:
//             SelectWindowWipe(1);
//             if (gGameState.unk1E8 == 0xF8)
//             {
//                 gGameState.unk1EE++;
//                 gMain.dispcnt ^= 0x2000;
//             }
//             SelectWindowDrawEdge();
//             break;
//         case 3:
//             if (gMain.modeState != 2 || (gMain.newKeys & 4))
//             {
//                 SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
//                 gGameState.unk1EE++;
//             }
//             break;
//         case 4:
//             SelectWindowWipe(0);
//             if (gGameState.unk1E8 == 0xFFF8)
//             {
//                 gGameState.unk1EE++;
//                 gMain.bgOfs[0][0] = 0;
//             }
//             SelectWindowDrawEdge();
//             break;
//         case 5:
//             SelectWindowWipe(1);
//             if (gGameState.unk1E8 == 0xF8)
//             {
//                 gGameState.unk1EE = 0;
//                 gGameState.unk1FB = 0;
//                 gGameState.unk104 = 1;
//                 gMain.dispcnt &= 0xDFFF;
//             }
//             SelectWindowDrawEdge();
//             break;
//         }
//     }
INCLUDE_ASM(UpdateSelectWindow);

void ResetSelectWindow(void)
{
    gGameState.unk1E8 = 0xF8;
    gGameState.unk1EA = 0x50;
    gGameState.unk1EE = 0;
}

// The status icon at (232, 128) and the blinking frame around it; gGameState.unk1F4 picks the
// icon.
void DrawStatusIcon(void)
{
    u8 blink = (gMain.frameCount >> 3) % 2;
    s32 m = FindMarker(1);
    u8 type;

    if (gGameState.unk1FB | gGameState.unk1FA | gGameState.unk224 | gGameState.unk1CC)
        return;
    if (gGameState.unk121 != 0 || gGameState.markers[m].unk6 == 4)
    {
        gGameState.unk1F4 = 0;
    }
    else if (gGameState.flags.unk8_7)
    {
        if (gGameState.flags.isFree)
            gGameState.unk1F4 = 1;
        else
            gGameState.unk1F4 = 0;
    }
    else if (gGameState.flags.isFree)
    {
        if (gGameState.unk1F5)
        {
            SoundPlayLoop(3, 3, 0);
            gGameState.unk1F4 = 2;
        }
        else
        {
            gGameState.unk1F4 = 1;
        }
    }
    else if (gGameState.flags.followTarget)
    {
        SoundPlayLoop(4, 4, 0);
        gGameState.unk1F4 = 1;
    }
    else if (gGameState.unk1F5)
    {
        SoundPlayLoop(3, 3, 0);
        gGameState.unk1F4 = 2;
    }
    else
    {
        gGameState.unk1F4 = 0;
    }

    if (gGameState.unk1F4 == 1)
    {
        type = gGameState.markers[FindMarker(1)].unk6;
        if (type)
            gGameState.unk1F4 = type + 2;
    }

    if (gGameState.unk1F4 == 0)
        return;
    if (gGameState.unk1F4 == 1)
        DrawSprite(gSprite_0202C420, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.unk1F4 == 2)
        DrawSprite(gSprite_0202C428, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.unk1F4 == 3)
        DrawSpriteAt(gSprite_0202C440, 0, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 4)
        DrawSpriteAt(gSprite_0202C448, gGameState.unk126, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 5)
        DrawSpriteAt(gSprite_0202C4A8, gGameState.unk12A, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 6)
        DrawSpriteAt(gSprite_0202C498, gGameState.unk128, 0xE8, 0x80, 0, 0, 0);
    DrawSprite(gSprite_0202C430, 0, 0xE0, blink + 0x80, 0, 0, 0, 0, 0, 0, 0);
}
