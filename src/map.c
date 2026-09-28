#include "global.h"
#include "gba/syscall.h"
#include "game.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

// Game code object 2 (GZLJ01 0201920C-0201BA2C): the map screen, map icons, quest markers
// and the cursor's messages. See docs/layout.md.

const s16 gMarkerDirOfsX[8] =
{
    0, 1, 1, 1, 0, -1, -1, -1
};

const s16 gMarkerDirOfsY[8] =
{
    1, 1, 0, -1, -1, -1, 0, 1
};

const u16 gSeaQuadrantOffsets[4][2] =
{
    0, 0, 1, 0, 0, 1, 1, 1
};

const u8 gSeaSquareTileSets[49] =
{
    0, 2, 2, 1, 1, 1, 2,
    2, 2, 1, 0, 1, 0, 1,
    2, 1, 1, 1, 1, 0, 1,
    1, 2, 2, 2, 2, 1, 1,
    1, 1, 2, 1, 1, 1, 1,
    1, 1, 1, 1, 2, 2, 1,
    1, 2, 1, 1, 1, 1, 1
};

const u8 gSeaAnimFrames[5] =
{
    0, 1, 2, 2, 1
};

const u8 gSeaAnimPeriods[3] =
{
    32, 16, 8
};

const u8 gTargetBlinkPeriods[5] =
{
    0, 5, 10, 25, 40
};

void (*const gMapIconFuncs[4])(struct MapIcon *icon) =
{
    MapIconFollowMarker,
    MapIconFollowMarker,
    MapIconCursor,
    MapIconBlink
};

const u8 *const gMapIconSprites[4] =
{
    gSprite_0202C208,
    gSprite_0202C208,
    gSprite_0202C1F8,
    (const u8 *)gSprite_0202C200
};

const u8 gPearlMarkerPos[6] =
{
    0x58, 0x18, 0x58, 0x58, 0x18, 0x38
};

const u8 gTriangleIslandPos[6] =
{
    0x38, 0x28, 0x58, 0x38, 0x38, 0x48
};

const u8 gTempleMarkerPos[4] =
{
    0x3C, 0x4, 0x2C, 0x6C
};

const u16 gTable_02024274[2] =
{
    0xCC, 0xE0
};

const s16 gMsgIds_02024278[18] =
{
    0x13, 0x38, 0x5E, 0x0, 0x3D, 0x3E, 0x3E, 0x3E, 0x3E,
    0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3E, 0x3F, 0x40, 0x0
};

void CopyMapColumn(s16 x, s16 y, const u16 *src)
{
    u16 *dest = (u16 *)(((y & 0xF8) << 3) + ((x & 0xF8) >> 2) + 0x06005800);
    s32 total = 16;
    s32 n = 32 - ((y >> 3) & 0x1F);
    bool32 wrapped = FALSE;
    const u16 *end;

    if (n > total)
        n = total;
    end = src + gGameState.mapImage->mapWidth * n;
    n = total - n;
    do
    {
        *dest = *src;
        dest += 0x20;
        if (dest > (u16 *)0x06005FFF)
        {
            dest -= 0x800;
            wrapped = TRUE;
        }
        src += gGameState.mapImage->mapWidth;
    } while (src < end);
    total = n;
    if (total != 0)
    {
        if (wrapped)
            dest += 0x400;
        else
            dest -= 0x400;
        end = src + gGameState.mapImage->mapWidth * total;
        do
        {
            *dest = *src;
            dest += 0x20;
            if (dest > (u16 *)0x06005FFF)
                dest -= 0x800;
            src += gGameState.mapImage->mapWidth;
        } while (src < end);
    }
}

void CopyMapRow(s16 y, s16 x, const u16 *src)
{
    u16 *dest = (u16 *)(((y & 0xF8) << 3) + ((x & 0xF8) >> 2) + 0x06005800);
    s32 total = 16;
    s32 n = 32 - ((x >> 3) & 0x1F);
    s32 size;

    if (n > 16)
        n = 16;
    size = n * 2;
    CpuCopy16(src, dest, size);
    total -= n;
    if (total != 0)
    {
        src = (u16 *)((u8 *)src + size);
        dest = (u16 *)((u8 *)dest + (size - 0x40));
        size = total * 2;
        CpuCopy16(src, dest, size);
    }
}

void ScrollMap(struct GameState *state)
{
    s16 ox = gMain.bgOfs[3][0] + 8;
    s16 oy = gMain.bgOfs[3][1] + 8;
    s16 dx = (state->mapX & ~7) - (ox & ~7);
    s16 dy = (state->mapY & ~7) - (oy & ~7);
    s16 colStep;
    s16 rowStep;
    s16 rowPix;
    s16 colPix;
    const u16 *src;

    if (dx == 0 && dy == 0)
        goto end;
    if (dx != 0)
    {
        if (dx < 0)
        {
            colStep = -1;
            colPix = -8;
        }
        else
        {
            ox += 120;
            colStep = 1;
            colPix = 8;
        }
        src = &gGameState.mapImage->map[(state->mapY >> 3) * gGameState.mapImage->mapWidth + (ox >> 3)];
        do
        {
            ox += colPix;
            src += colStep;
            CopyMapColumn(ox, state->mapY, src);
            dx -= colPix;
        } while (dx != 0);
    }
    if (dy != 0)
    {
        if (dy < 0)
        {
            rowStep = -gGameState.mapImage->mapWidth;
            rowPix = -8;
        }
        else
        {
            oy += 120;
            rowStep = gGameState.mapImage->mapWidth;
            rowPix = 8;
        }
        src = &gGameState.mapImage->map[(oy >> 3) * gGameState.mapImage->mapWidth + (state->mapX >> 3)];
        do
        {
            oy += rowPix;
            src += rowStep;
            CopyMapRow(oy, state->mapX, src);
            dy -= rowPix;
        } while (dy != 0);
    }
end:
    gMain.bgOfs[3][0] = state->mapX - 8;
    gMain.bgOfs[3][1] = state->mapY - 8;
}

// Draw the map markers: step the markers' animation timers (while gGameState.unk22B is set),
// then draw every marker of a known type (1-21) that is on screen with its type's sprite
// (gSpriteTable[type], or a special one), rotated by MapMarker.unk4, plus a highlight ring when
// unk5_6 is set; last the extra sprite at gGameState.icons[2].x/056. Type 1 (Tingle?) is drawn at
// half size over a second sprite chosen by unk6; type 3 (Link?) is hidden in some states.
// Near-miss (GZLJ01): the allocation differs from the marker loop on. The original keeps pal in
// sl (a register reloaded with 0 every iteration) and spills dx; this C either folds pal to the
// constant 0 or, as below, keeps it as an int (with a u8 conversion). Also the second
// `marker->type == 3` test is kept in the original but dropped here. The US builds differ:
// no pal variable (0 inline, 12 for type 1's unk6 == 3 sprite) and frame 0 for the last sprite.
// Also: the original's frame is 0x30 (five spill slots at sp+0x1c..0x2c, mosaic at 0x28);
// this C gets 0x2c (four, mosaic at 0x1c), so the spilled locals differ too (not tried further).
/*
#define SPRITE_FRAMES(p) ((const struct SpriteFrame *)(p))

void DrawMapMarkers(void)
{
    struct MapMarker *marker = gGameState.markers;
    u8 mosaic = gMain.mosaic != 0;
    s32 i;
    const struct SpriteFrame *frames;
    s32 pal = 0;

    ObjUpdateAll();
    if (gGameState.unk22B)
    {
        if (gGameState.unk108 > 19)
        {
            gGameState.unk108 = 0;
            gGameState.unk109 ^= 1;
        }
        else
        {
            gGameState.unk108++;
        }
        if (gGameState.unk125 == 0)
        {
            if (++gGameState.unk126 > 8)
                gGameState.unk126 = 0;
            gGameState.unk125 = gSprite_0202C448[gGameState.unk126].duration;
        }
        else
        {
            gGameState.unk125--;
        }
        if (gGameState.unk129 == 0)
        {
            gGameState.unk12A ^= 1;
            gGameState.unk129 = gSprite_0202C498[gGameState.unk12A].duration;
        }
        else
        {
            gGameState.unk129--;
        }
        if (gGameState.unk127 == 0)
        {
            gGameState.unk128 ^= 1;
            gGameState.unk127 = gSprite_0202C498[gGameState.unk128].duration;
        }
        else
        {
            gGameState.unk127--;
        }
        if (gGameState.unk10A > 9)
        {
            gGameState.unk10A = 0;
            gGameState.unk10B ^= 1;
        }
        else
        {
            gGameState.unk10A++;
        }
        if (gGameState.unk121 != 0)
        {
            if (gGameState.flags.unk5_5)
            {
                if (gGameState.unk123 == 0)
                {
                    gGameState.unk124++;
                    if (gGameState.unk121 == 1)
                    {
                        if (gGameState.unk124 > 6)
                            gGameState.unk124 = 0;
                        frames = &SPRITE_FRAMES(gSprite_0202CA08)[gGameState.unk124];
                        gGameState.unk123 = frames->duration;
                    }
                    else if (gGameState.unk124 > 18)
                    {
                        gGameState.unk121 = 0;
                        Mode1SendSwitch(3);
                    }
                    else
                    {
                        frames = &SPRITE_FRAMES(gSprite_0202CA40)[gGameState.unk124];
                        gGameState.unk123 = frames->duration;
                    }
                }
                else
                {
                    gGameState.unk123--;
                }
            }
            else if (gGameState.unk121 == 1)
            {
                gGameState.unk121 = 0;
            }
        }
        else if (gGameState.unk1E5_1 && gGameState.flags.busy == 0)
        {
            if (gGameState.unk10C > 29)
            {
                gGameState.unk10C = 0;
                gGameState.unk10D ^= 1;
            }
            else
            {
                gGameState.unk10C++;
            }
        }
        if (gGameState.unk10F != 0)
        {
            gGameState.unk10F--;
        }
        else
        {
            gGameState.unk110 ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[9])[gGameState.unk110];
            gGameState.unk10F = frames->duration;
        }
        if (gGameState.unk113 != 0)
        {
            gGameState.unk113--;
        }
        else
        {
            if (++gGameState.unk114 > 5)
                gGameState.unk114 = 0;
            frames = &SPRITE_FRAMES(gSpriteTable[13])[gGameState.unk114];
            gGameState.unk113 = frames->duration;
        }
        if (gGameState.unk115 != 0)
        {
            gGameState.unk115--;
        }
        else
        {
            gGameState.unk116 ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[16])[gGameState.unk116];
            gGameState.unk115 = frames->duration;
        }
        if (gGameState.unk117 != 0)
        {
            gGameState.unk117--;
        }
        else
        {
            gGameState.unk118 ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[19])[gGameState.unk118];
            gGameState.unk117 = frames->duration;
        }
    }

    gGameState.unk1F5 = 0;
    for (i = 0; i < gGameState.markerCount; i++, marker++)
    {
        u8 frame;
        u16 scale;
        s16 dx;
        s16 dy;
        u8 pal2;

        if (marker->type == 0 || marker->type > 21)
            continue;
        scale = 0;
        if (!gGameState.unk1E5_1 && marker->type == 3 && gGameState.unk121 == 0 && marker->type == 3)
        {
            if (!gGameState.unk1E5_2)
                continue;
            if (gGameState.effect[0] != 14)
                continue;
        }
        dx = marker->x - gMain.bgOfs[3][0];
        dy = marker->y - gMain.bgOfs[3][1];
        if ((u16)dx > 0x80)
            continue;
        if (dx <= 0 || dx > 0x87)
            continue;
        if (dy <= 0)
            continue;
        if (dy > 0x87)
            continue;
        frames = SPRITE_FRAMES(gSpriteTable[marker->type]);
        pal = 0;
        if (marker->type == 1)
        {
            if (marker->unk6 == 0)
                DrawSpriteAt(gSprite_0202C420, 0, dx, dy, mosaic, pal, 1);
            else if (marker->unk6 == 1)
                DrawSpriteAt(gSprite_0202C440, 0, dx, dy, mosaic, pal, 1);
            else if (marker->unk6 == 2)
                DrawSpriteAt(gSprite_0202C448, gGameState.unk126, dx, dy, mosaic, pal, 1);
            else if (marker->unk6 == 3)
                DrawSpriteAt(gSprite_0202C4A8, gGameState.unk12A, dx, dy, mosaic, pal, 1);
            else if (marker->unk6 == 4)
                DrawSpriteAt(gSprite_0202C498, gGameState.unk128, dx, dy, mosaic, pal, 1);
            scale = 0x80;
            frame = gGameState.unk109;
        }
        else if (marker->type == 3)
        {
            if (gGameState.unk121 != 0)
            {
                frames = gSprite_0202CA40;
                if (gGameState.unk121 == 1)
                    frames = gSprite_0202CA08;
                frame = gGameState.unk124;
            }
            else if (gGameState.unk11A != 0)
            {
                frames = gSprite_0202CAD8;
                frame = 0;
            }
            else if (gGameState.flags.busy == 0 && (gGameState.unk1FA != 0 || gGameState.unk104 != 0))
            {
                frame = gGameState.unk10D;
            }
            else
            {
                frames = gSprite_0202C130;
                frame = 1;
            }
        }
        else if (marker->type == 2)
        {
            if (marker->unk7 == 0x21)
            {
                frames = gSprite_0202BD80;
                frame = 0;
            }
            else
            {
                frame = gGameState.unk10B;
            }
        }
        else if (marker->type == 6)
        {
            if (marker->unk7 == 0x31)
                frames = gSprite_0202BD80;
            frame = 0;
        }
        else if (marker->type == 9)
        {
            if (gGameState.unk110 != 0)
            {
                dx += gMarkerDirOfsX[marker->unk4 >> 5];
                dy += gMarkerDirOfsY[marker->unk4 >> 5];
            }
            frame = 0;
        }
        else if (marker->type == 13)
        {
            if (gGameState.unk11D == 0)
            {
                frames = SPRITE_FRAMES(gSpriteTable[7]);
                frame = 0;
            }
            else
            {
                frame = gGameState.unk114;
            }
        }
        else if (marker->type == 16)
        {
            if ((gGameState.unk026 >> marker->unk6) & 1)
            {
                frame = gGameState.unk116;
            }
            else if (gGameState.unk11B == 16 && gGameState.unk119 == i)
            {
                frames = SPRITE_FRAMES(gSpriteTable[7]);
                frame = 0;
            }
            else
            {
                frames = gSprite_0202C130;
                frame = 1;
            }
        }
        else if (marker->type == 5)
        {
            if (marker->unk6 != 0xFF && !(gGameState.unk010 & (1 << marker->unk6)))
                frames = SPRITE_FRAMES(gSpriteTable[7]);
            frame = 0;
        }
        else if (marker->type == 12)
        {
            if (gGameState.unk11C == 0)
                frames = SPRITE_FRAMES(gSpriteTable[7]);
            frame = 0;
        }
        else if (marker->type == 19)
        {
            if (marker->unk6 == 0)
            {
                frames = gSprite_0202C4B8;
                frame = 0;
            }
            else if (marker->unk6 == 1)
            {
                frames = gSprite_0202C440;
                frame = 0;
            }
            else if (marker->unk6 == 2)
            {
                frames = gSprite_0202C448;
                frame = gGameState.unk126;
            }
            else if (marker->unk6 == 3)
            {
                frames = gSprite_0202C4A8;
                frame = gGameState.unk12A;
            }
            else
            {
                frame = gGameState.unk118;
            }
        }
        else
        {
            frame = 0;
        }
        if (marker->unk5_6)
        {
            gGameState.unk1F5 = marker->type == 2 ? 1 : 0;
            pal2 = 2;
            if (marker->unk5_6 == 1)
                pal2 = 3;
            DrawSprite(gSprite_0202BE28, 0, dx, dy - 8, marker->unk4 + 0x80, mosaic, 0, 0, 0, pal, pal2 + 1);
        }
        DrawSprite(frames, frame, dx, dy, marker->unk4 + 0x80, mosaic, scale, 0, 0, pal, 0);
    }

    if (gMain.unk457 == 0 && gGameState.unk224 == 0 && gGameState.icons[2].type != 0)
    {
        s16 x = gGameState.icons[2].x - gMain.bgOfs[3][0];
        s16 y = gGameState.icons[2].y - gMain.bgOfs[3][1];

        if ((u16)(x + 0x40) <= 0xFF && (u16)(y + 0x40) <= 0xFF)
            DrawSpriteAt(gSprite_0202C200, gGameState.icons[2].frame, x, y, mosaic, 0, 1);
    }
}
*/
INCLUDE_ASM(DrawMapMarkers);

// Highlight the rooms on the four BG map blocks at 0x0600C000 (one room each, gGameState.seaSquare):
// clear the palette bits of the block showing marker 1's room, then set palette bit 1 (|= 0x2000)
// on a 2x2 or 4x4 tile square around every gSlot14Data entry in each block's room. Room 0x19's
// entries only show once flags.unkA_4 is set.
// Near-miss: the original keeps (y >> 8) * 7 narrowed to u16 before adding it to room, and
// allocates registers differently from there on (the unrolled loop's 0x1FFF lands in r2, not r4).
// The u16 row temp b below gives the narrowing, so the entry block matches (430 diff lines,
// was 432). What remains starts at the first loop: the original keeps i in r1, the hoisted
// constant 1 in r2 and gGameState in r4; here 1 is a HImode pseudo (the & 1 on s16 >> 8 is
// shortened) with low priority, so it lands in r4 and gGameState gets copied to r1. Casting
// the shifts to s32/u32, 1u and s32 temps for x >> 8 / y >> 8 did not change it.
/*
#define CLEAR_PAL(p) (*(p) &= 0x1FFF, (p)++)
#define CLEAR_PAL_4(p) CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p)
#define CLEAR_PAL_32(p) CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); \
    CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p)

void HighlightMapRooms(void)
{
    u8 m = FindMarker(1);
    u16 room, b;
    s32 i, j, k;
    u16 *map;

    room = gGameState.markers[m].x >> 8;
    b = (gGameState.markers[m].y >> 8) * 7;
    room += b;
    for (i = 0; i <= 3; i++)
    {
        if (gGameState.seaSquare[i] == room)
        {
            map = (u16 *)(0x0600C000 + ((((gGameState.markers[m].x >> 8) & 1) | (((gGameState.markers[m].y >> 8) & 1) << 1)) << 11));
            for (j = 0; j < 32; j++)
            {
                CLEAR_PAL_32(map);
            }
            break;
        }
    }
    for (i = 0; i <= 3; i++)
    {
        if (gGameState.seaSquare[i] == 0x19 && !gGameState.flags.unkA_4)
            continue;
        for (j = 0; j <= 0x30; j++)
        {
            u8 *e = &gSlot14Data[j * 4];

            if (gGameState.seaSquare[i] == e[3])
            {
                u16 x, y;
                s32 size;
                s32 row;

                if (e[2])
                {
                    x = (e[0] >> 3) - 2;
                    y = (e[1] >> 3) - 2;
                    size = 4;
                }
                else
                {
                    x = (e[0] >> 3) - 1;
                    y = (e[1] >> 3) - 1;
                    size = 2;
                }
                map = (u16 *)(0x0600C000 + (i << 11)) + (y * 32 + x);
                for (row = 0; row < size * 32; row += 32)
                {
                    for (k = 0; k < size; k++)
                        map[row + k] |= 0x2000;
                }
            }
        }
    }
}
*/
INCLUDE_ASM(HighlightMapRooms);

void FollowMarkerFixedMap(void)
{
    s16 dx;
    s16 dy = 0;
    u8 i = FindMarker(3);

    if (i == 0xFF)
        i = FindMarker(1);
    gGameState.scrolling = FALSE;
    dx = gGameState.markers[i].x - (s16)(gGameState.mapX + 0x44);
    if (dx < -16)
    {
        dx = -16;
        gGameState.scrolling = TRUE;
    }
    else if (dx > 16)
    {
        dx = 16;
        gGameState.scrolling = TRUE;
    }
    dy = gGameState.markers[i].y - (s16)(gGameState.mapY + 0x44);
    if (dy < -16)
    {
        dy = -16;
        gGameState.scrolling = TRUE;
    }
    else if (dy > 16)
    {
        dy = 16;
        gGameState.scrolling = TRUE;
    }
    gGameState.mapX += dx;
    gGameState.mapY += dy;
    gMain.bgOfs[2][0] = gGameState.mapX;
    gMain.bgOfs[2][1] = gGameState.mapY;
    if ((gMain.bgOfs[2][0] + 8) & 0x8000)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][0] = -8;
    }
    if (gMain.bgOfs[2][0] > 0x680)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][0] = 0x680;
    }
    if ((gMain.bgOfs[2][1] + 8) & 0x8000)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][1] = -8;
    }
    if (gMain.bgOfs[2][1] > 0x680)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][1] = 0x680;
    }
    gMain.bgOfs[3][0] = gMain.bgOfs[2][0];
    gMain.bgOfs[3][1] = gMain.bgOfs[2][1];
}

void FollowMarkerStreamedMap(void)
{
    s16 maxX = gGameState.mapImage->width - 120;
    s16 maxY = gGameState.mapImage->height - 120;
    s16 dx;
    s16 dy = 0;
    u8 i = FindMarker(3);

    if (i == 0xFF)
        i = FindMarker(1);
    gGameState.scrolling = FALSE;
    dx = gGameState.markers[i].x - (s16)(gGameState.mapX + 0x3C);
    if (dx < -16)
    {
        dx = -16;
        gGameState.scrolling = TRUE;
    }
    else if (dx > 16)
    {
        dx = 16;
        gGameState.scrolling = TRUE;
    }
    dy = gGameState.markers[i].y - (s16)(gGameState.mapY + 0x3C);
    if (dy < -16)
    {
        dy = -16;
        gGameState.scrolling = TRUE;
    }
    else if (dy > 16)
    {
        dy = 16;
        gGameState.scrolling = TRUE;
    }
    if (gGameState.markers[i].x == gGameState.unk01E && gGameState.markers[i].y == gGameState.unk020)
        gGameState.unk019--;
    else
        gGameState.unk019 = 2;
    if ((s8)gGameState.unk019 <= 0)
    {
        gGameState.unk018 = 1;
        gGameState.unk019 = 2;
    }
    gGameState.unk01E = gGameState.markers[i].x;
    gGameState.unk020 = gGameState.markers[i].y;
    gGameState.mapX += dx;
    if (gGameState.mapX < 0)
    {
        gGameState.scrolling = FALSE;
        gGameState.mapX = 0;
    }
    if (gGameState.mapX > maxX)
    {
        gGameState.scrolling = FALSE;
        gGameState.mapX = maxX;
    }
    gGameState.mapY += dy;
    if (gGameState.mapY < 0)
    {
        gGameState.scrolling = FALSE;
        gGameState.mapY = 0;
    }
    if (gGameState.mapY > maxY)
    {
        gGameState.scrolling = FALSE;
        gGameState.mapY = maxY;
    }
}

// Per-frame map update. Sea chart mode (gMain.unk457 == 0): find the four sea chart squares
// around the view, step their tile animations and draw the chart. Map mode: while L is held,
// scroll the map image with the D-pad and draw the scroll cursor.
// The locals dx and dy double as the loop's column and row.
void UpdateMap(void)
{
    u16 maxX = gGameState.mapImage->width - 120;
    u16 maxY = gGameState.mapImage->height - 120;
    s16 dx = 0;
    s16 dy = 0;
    u8 i = FindMarker(3);

    if (i == 0xFF)
        FindMarker(1);
    gGameState.unk018 = 0;
    if (gMain.unk457 == 0)
    {
        s32 j;

        Func_0201B2C0();
        FollowMarkerFixedMap();
        for (j = 0; j < 4; j++)
        {
            s32 k;
            s32 t;
            s32 row;

            dx = ((gMain.bgOfs[2][0] + 8) >> 8) + gSeaQuadrantOffsets[j][0];
            if (dx > 6)
                dx = 7;
            row = (gMain.bgOfs[2][1] + 8) >> 8;
            t = j * 2;  // for seaAnimTimer[j] below, computed here as in the original
            dy = row + gSeaQuadrantOffsets[j][1];
            if (dy > 6)
                dy = 7;
            k = (dy & 1) * 2 + (dx & 1);
            gGameState.seaSquare[k] = dx + dy * 7;
            gGameState.seaTileSet[k] = gSeaSquareTileSets[dy * 7 + dx];
            if (gGameState.seaAnimTimer[k] >= gSeaAnimPeriods[gGameState.seaTileSet[k]])
            {
                if (++gGameState.seaAnimFrame[k] > 4)
                    gGameState.seaAnimFrame[k] = 0;
                gGameState.seaAnimTimer[k] = 0;
            }
            else
            {
                gGameState.seaAnimTimer[t / 2]++;  // [j], not [k]
            }
        }
        CpuCopy16(gSeaAnimTiles, (void *)0x0600C000, 0x2000);
        HighlightMapRooms();
    }
    else
    {
        if (!(gMain.heldKeys & 0x200))
        {
            if (gGameState.mapScrolling == 1)
            {
                SoundPlay(7 - SOUND_ID_SHIFT, 0);
                gGameState.mapScrolling = 0;
            }
            if (gGameState.unk120 == 0)
                FollowMarkerStreamedMap();
        }
        else if (gGameState.unk1E5_1)
        {
            u8 mosaic;
            const u8 *sprite;
            u8 frame;
            bool32 clamped;

            if (gMain.newKeys & 0x200)
            {
                SoundPlay(0x16 - SOUND_ID_SHIFT, 0);
                gGameState.mapScrolling = 1;
            }
            if (gMain.heldKeys & 0x40)
                dy = -4;
            else if (gMain.heldKeys & 0x80)
                dy = 4;
            if (gMain.heldKeys & 0x20)
                dx = -4;
            else if (gMain.heldKeys & 0x10)
                dx = 4;
            mosaic = gMain.mosaic != 0;
            sprite = gSprite_0202BDC8;
            frame = 0;
            if (!(gMain.frameCount & 0xC))
                frame = 1;
            DrawSprite(sprite, frame, 0x44, 0x44, 0, mosaic, 0, 0, 0, 0, 0);
            clamped = FALSE;
            gGameState.mapX += dx;
            if (gGameState.mapX < 0)
            {
                gGameState.mapX = 0;
                clamped = TRUE;
            }
            if (gGameState.mapX > (s16)maxX)
            {
                gGameState.mapX = maxX;
                clamped = TRUE;
            }
            gGameState.mapY += dy;
            if (gGameState.mapY < 0)
            {
                gGameState.mapY = 0;
                clamped = TRUE;
            }
            if (gGameState.mapY > (s16)maxY)
            {
                gGameState.mapY = maxY;
                clamped = TRUE;
            }
            if (!clamped && (gMain.heldKeys & 0xF0))
                SoundPlayLoop(8, 8, NULL);
        }
        ScrollMap(&gGameState);
    }
    if (gGameState.unk224 == 0)
        // DrawMapMarkers takes no parameters, but this passes two.
        ((void (*)())DrawMapMarkers)(&gGameState, gMain.frameCount);
}

void Func_0201A540(void)
{
    if (gMain.unk457 == 0)
    {
        FollowMarkerFixedMap();
    }
    else
    {
        FollowMarkerStreamedMap();
        ScrollMap(&gGameState);
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
                CopyMapRow(y, x, map);
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
// Tried: block-local s32 dx/dy for the ArcTan2 operands (fixes that block, but the sin
// part's x/y then lose their s16 narrowing and the diff grows to 180 lines; sharing x/y is
// what keeps it), s16/u16 x and y, one-expression sin loads, u32 level and a separate u32
// distance. Priorities in the original: level > the unk224 zero > gGameState (r4, r5, r6).
//     void UpdateTargetCompass(void)
//     {
//         u8 angle;
//         s32 x, y;
//         s32 level;
//
//         if (gGameState.unk1FA != 0)
//             return;
//         if (gGameState.unk224 != 0)
//             return;
//         if (gGameState.icons[2].type != 3)
//             return;
//         {
//             u8 i = FindMarker(1);
//
//             x = gGameState.icons[2].x - gGameState.markers[i].x;
//             y = gGameState.icons[2].y - gGameState.markers[i].y;
//             angle = ((u16)~ArcTan2(x >> 4, y >> 4) + 0x4001) >> 8;
//         }
//         x = gSinTable[angle];
//         x >>= 3;
//         y = (gSinTable + 64)[angle];
//         y >>= 3;
//         DrawSprite(gMapIconSprites[gGameState.icons[2].type], 0, (u16)(x + 0xB8), (u16)(y + 0x58), 0, 0, 0, 0, 0, 0, 0);
//         level = MarkerDistance(0);
//         if ((u32)level <= 0x18)
//         {
//             if (gGameState.unk224 == 0)
//             {
//                 ShowMessage(0x1A5, 1);
//                 gGameState.icons[2].type = 0;
//                 gGameState.icons[1].type = 0;
//                 gGameState.unk105 = 0;
//             }
//             level = 0;
//             gGameState.targetNear = 0;
//         }
//         else if ((u32)level <= 0x7F)
//         {
//             if (gGameState.unk224 == 0 && gGameState.targetNear == 0)
//             {
//                 ShowMessage(0x1A4, 1);
//                 gGameState.targetNear = 1;
//             }
//             level = 1;
//         }
//         else if ((u32)level <= 0x1FF)
//         {
//             level = 2;
//             gGameState.targetNear = 0;
//         }
//         else if ((u32)level <= 0x3FF)
//         {
//             level = 3;
//             gGameState.targetNear = 0;
//         }
//         else
//         {
//             level = 4;
//             gGameState.targetNear = 0;
//         }
//         if (gGameState.targetBlinkOn != 0)
//         {
//             if (--gGameState.targetBlinkTimer == 0)
//                 gGameState.targetBlinkOn = 0;
//         }
//         else
//         {
//             if (++gGameState.targetBlinkTimer >= gTargetBlinkPeriods[level])
//             {
//                 gGameState.targetBlinkTimer = gTargetBlinkPeriods[level];
//                 gGameState.targetBlinkOn = 1;
//             }
//         }
//         gGameState.targetScale = (gGameState.targetBlinkTimer << 7) / gTargetBlinkPeriods[level] + 0x100;
//         DrawSprite(gSprite_0202C3F8, 0, 200, 72, angle + 0x80, 0, gGameState.targetScale, 0, 16, 0, 0);
//         if (gGameState.unk224 == 0 && gGameState.flags.hasTact && level > 1
//          && (u8)(gGameState.flags.unk2 - angle + 9) > 18)
//         {
//             angle = ~angle + 1;
//             if (((angle + 8) & 0x1F) <= 0x10)
//             {
//                 u8 dir = (u8)(angle + 0x88) >> 5;
//
//                 if (gGameState.targetDir != dir && gGameState.unk1FA == 0)
//                 {
//                     ShowMessage(0x19C + dir, 1);
//                     gGameState.targetDir = dir;
//                 }
//             }
//             else
//             {
//                 gGameState.targetDir = 0xFF;
//             }
//         }
//     }
INCLUDE_ASM(UpdateTargetCompass);

// Copy each view quadrant's current sea animation frame into the BG tiles, then the map image
// tiles (loaded at 0x0600C000) to 0x06004000.
// A separate tile index per half and a shared src pointer: the index dies once, so local-alloc
// ties it to the tile-set term, and src (set twice) is left to global-alloc, so the final add
// isn't tied into r0 (one shared index, or passing the address straight, adds in place).
void Mode1VBlank(void)
{
    u8 *dest = (u8 *)0x0600EC00;
    s32 i;
    s32 a1, a2, t;
    u8 *src;

    for (i = 0; i < 4; i++)
    {
        a1 = gGameState.seaTileSet[i] * 16;
        t = gSeaAnimFrames[gGameState.seaAnimFrame[i]] * 4 + 0x300;
        a1 += t;
        src = (u8 *)0x06008000 + a1 * 32;
        CpuCopy32(src, dest, 0x80);
        dest += 0x80;
        a2 = gGameState.seaTileSet[i] * 16;
        t = ((gSeaAnimFrames[gGameState.seaAnimFrame[i]] + 2) & 3) * 4 + 0x300;
        a2 += t;
        src = (u8 *)0x06008000 + a2 * 32;
        CpuCopy32(src, dest, 0x80);
        dest += 0x80;
    }
    CpuCopy32((void *)0x0600C000, (void *)0x06004000, 0x2000);
}

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
                        ShowMessage(0x13E, 1);
                    else
                        ShowMessage(0x14E + gSlot14Data[i].square, 1);
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

// Mark the current quest goals on the sea chart: the pearl to find next, else the Triangle
// Islands still missing their pearl, else the temples whose boss is still alive.
void DrawQuestMarkers(void)
{
    u32 mosaic = gMain.mosaic != 0;
    s32 i;

    if (gGameState.flags.pearlToFind)
    {
        for (i = 0; i < 3; i++)
            if ((gGameState.flags.pearlToFind >> i) & 1)
                DrawSprite(gSprite_0202C200, 0, gPearlMarkerPos[i * 2] + 12,
                           gPearlMarkerPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
    else if (gGameState.flags.pearlsToPlace)
    {
        for (i = 0; i < 3; i++)
            if ((gGameState.flags.pearlsToPlace >> i) & 1)
                DrawSprite(gSprite_0202C200, 0, gTriangleIslandPos[i * 2] + 12,
                           gTriangleIslandPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
    else if (gGameState.flags.bossesToBeat)
    {
        for (i = 0; i < 2; i++)
            if ((gGameState.flags.bossesToBeat >> i) & 1)
                DrawSprite(gSprite_0202C200, 0, gTempleMarkerPos[i * 2] + 12,
                           gTempleMarkerPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
}

// B closes and L toggles the sea chart overlay (unk224) while unk1E5_1 allows it; while it
// is open, A (once) calls InitCursorIcon and the quest markers are drawn.
void Func_0201B0F4(void)
{
    u16 tile;
    u16 *map;

    if (gGameState.unk1E5_1 && (gMain.newKeys & 0x202))
    {
        gGameState.unk105 = 0;
        gGameState.icons[1].type = 0;
        if (gMain.newKeys & 2)
        {
            if (gGameState.unk224)
            {
                gGameState.unk224 = 0;
                gGameState.unk13C = 0;
                SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
            }
        }
        else
        {
            gGameState.unk224 ^= 1;
#ifndef VERSION_GZLJ01
            if (gGameState.unk224 == 0)
                MsgClose();
#endif
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        }
        if (gGameState.unk224)
        {
            gMain.bgOfs[1][1] = 0x7C;
            gGameState.unk0FC = 2;
            gGameState.unk0FD = 1;
            InitMarkerIcon();
            if (gGameState.icons[2].type)
                InitTargetIcon();
            map = (u16 *)0x060075D2;
            tile = gTable_02024274[gGameState.flags.unkA_4];
            if (!(gGameState.unk0F4[3] & 2))
                tile |= 0x6000;
            if (gGameState.flags.unkA_4)
            {
                map[0] = tile;
                map[1] = tile | 0x400;
                map[0x20] = tile | 0x800;
                map[0x21] = tile | 0xC00;
            }
            else
            {
                map[0] = tile;
                map[1] = tile;
                map[0x20] = tile;
                map[0x21] = tile;
            }
        }
        else
        {
            gGameState.unk0FC = 0;
            gGameState.unk0FD = 0;
        }
        Func_0201E584();
    }
    else if (gGameState.unk224)
    {
        gMain.bgOfs[2][0] = -4;
        gMain.bgOfs[2][1] = 4;
        UpdateMapIcons();
        if (gGameState.unk105 == 0 && (gMain.newKeys & 1))
        {
            InitCursorIcon();
            gGameState.unk105 = 1;
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        }
        DrawQuestMarkers();
    }
    UpdateTargetCompass();
}

// Darken the sea chart's 7x7 squares (2x2 tiles each, from map row 17 column 1 of the BG
// map at 0x06007000) whose bit in gGameState.unk0F4 is clear: palette 6 instead of 0.
void Func_0201B2C0(void)
{
    s32 i;
    u32 off;
    u16 *p;

    for (i = 0; i < 49; i++)
    {
        off = (((i / 7) * 2 + 17) * 32 + ((i % 7) * 2 + 1)) * 2;
        p = (u16 *)(0x06007000 + off);
        if ((gGameState.unk0F4[i >> 3] >> (i & 7)) & 1)
        {
            p[0] &= 0xFFF;
            p[1] &= 0xFFF;
            p = (u16 *)(0x06007040 + off);
            *p &= 0xFFF;
            p = (u16 *)(0x06007042 + off);
            *p &= 0xFFF;
        }
        else
        {
            p[0] = (p[0] & 0xFFF) | 0x6000;
            p[1] = (p[1] & 0xFFF) | 0x6000;
            p = (u16 *)(0x06007040 + off);
            *p = (*p & 0xFFF) | 0x6000;
            p = (u16 *)(0x06007042 + off);
            *p = (*p & 0xFFF) | 0x6000;
        }
    }
}

// What the player's marker (type 3) is on: returns the index of a marker within 8 pixels
// (and sets unk11B to its type), else looks the position up in the slot 14 table of sea
// chart squares (setting unk135 to the square's id + 1). unk11A: something is selected.
// Only the first type test uses `type`; the rest re-read the field, which cse turns into a
// copy of type's register (the `adds r0, r6, #0` after the first compare). `r` is s16: the
// comparisons then use a widened copy of it.
s32 Func_0201B3A4(void)
{
    u8 player;
    s32 i;
    u8 type;
    u8 square;
    u16 ex;
    u16 ey;
    s16 r;

    if (gGameState.unk121 != 0)
    {
        gGameState.unk11B = 0;
        gGameState.unk11A = 0;
        return 0;
    }
    gGameState.unk135 = 0;
    player = FindMarker(3);
    i = FindMarker(0x14);
    if (i != 0xFF && gGameState.unk136 == 0)
    {
        if ((u16)(gGameState.markers[i].x - gGameState.markers[player].x + 8) > 16 || (u16)(gGameState.markers[i].y - gGameState.markers[player].y + 8) > 16)
            gGameState.unk136 = 1;
    }
    if ((u8)(gGameState.switchType - 1) <= 1 || gGameState.switchType == 4
     || (gGameState.switchType == 13 && gGameState.flags.isFree))
    {
        if (gGameState.unk11A == 0)
        {
            SoundPlay(1, 0);
            gGameState.unk11A = 1;
        }
        return 0;
    }
    if (gGameState.switchType == 0xFF)
    {
        gGameState.unk11A = 1;
        return 0;
    }
    if (player == 0xFF)
        goto none;
    for (i = 0; i < gGameState.markerCount; i++)
    {
        type = gGameState.markers[i].type;
        if (type == 1 || gGameState.markers[i].type == 3 || gGameState.markers[i].type == 4
         || gGameState.markers[i].type == 0x11 || gGameState.markers[i].type == 0x12)
            continue;
        if ((u16)(gGameState.markers[i].x - gGameState.markers[player].x + 8) > 16)
            continue;
        if ((u16)(gGameState.markers[i].y - gGameState.markers[player].y + 8) > 16)
            continue;
        gGameState.unk11B = type;
        if (gGameState.markers[i].unk7 != 0)
        {
            if (gGameState.markers[i].type == 2 || gGameState.markers[i].type == 6 || gGameState.markers[i].type == 14 || gGameState.markers[i].type == 15)
            {
                gGameState.unk11A = 1;
                return i;
            }
            if (gGameState.markers[i].type == 5)
            {
                if (gGameState.markers[i].unk6 == 0xFF)
                    continue;
                if (gGameState.unk010 & (1 << gGameState.markers[i].unk6))
                    continue;
                if (gGameState.unk11A == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.unk11A = 1;
                }
                return i;
            }
            if (gGameState.markers[i].type == 12)
            {
                if (gGameState.unk11C)
                    continue;
                if (gGameState.unk11A == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.unk11A = 1;
                }
                return i;
            }
            if (gGameState.unk11A == 0)
            {
                if (gGameState.flags.isFree)
                    SoundPlay(1, 0);
                gGameState.unk11A = 1;
            }
            return i;
        }
        if (gGameState.markers[i].type == 7)
        {
            if (gGameState.switchType == 4 || gGameState.switchType == 10)
            {
                if (gGameState.unk11A == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.unk11A = 1;
                }
                return i;
            }
            gGameState.unk11A = 0;
            return 0;
        }
        if ((u8)(gGameState.markers[i].type - 8) <= 3 || gGameState.markers[i].type == 0x15)
        {
            if (gGameState.switchType == 3 || gGameState.switchType == 15)
                gGameState.unk11A = 1;
            else
                gGameState.unk11A = 0;
            return 0;
        }
        if (gGameState.markers[i].type == 0x13)
        {
            if ((u8)(gGameState.markers[i].unk6 - 1) <= 2)
            {
                gGameState.unk11A = 1;
                return i;
            }
            return 0;
        }
        if (gGameState.markers[i].type == 0x14)
        {
            if (gGameState.unk136)
            {
                gGameState.unk11A = 1;
                return i;
            }
            gGameState.unk11B = 0;
            return 0;
        }
        if (gGameState.unk11A == 0)
        {
            if (gGameState.flags.isFree)
                SoundPlay(1, 0);
            gGameState.unk11A = 1;
        }
        return 0;
    }
    if (gMain.unk457 == 0)
    {
        square = (gGameState.markers[player].x >> 8) + (gGameState.markers[player].y >> 8) * 7;
        for (i = 0; i < 49; i++)
        {
            if (gSlot14Data[i].square != square)
                continue;
            if (square == 25 && !gGameState.flags.unkA_4)
                return 0;
            if (gSlot14Data[i].big)
            {
                ex = (gSlot14Data[i].x - 16) & 0xF8;
                ey = (gSlot14Data[i].y - 16) & 0xF8;
                r = 32;
            }
            else
            {
                ex = gSlot14Data[i].x - 8;
                ey = gSlot14Data[i].y - 8;
                r = 8;
            }
            if ((u8)(gGameState.markers[player].x - ex) > r)
                continue;
            if ((u8)(gGameState.markers[player].y - ey) > r)
                continue;
            gGameState.unk135 = gSlot14Data[i].square + 1;
            gGameState.unk11A = 1;
            return 0;
        }
    }
none:
    gGameState.unk11B = 0;
    gGameState.unk11A = 0;
    return 0;
}

// Play the sound for what the cursor is on: unk135 if set, else the selected marker's.
// Near-miss: the original computes the (s16) ids in the shifted domain, ((v << 16) +
// (c << 16)) >> 16, which agbcc only emits for an explicit shift pair (the ID macro below);
// written that way the code shape matches but the registers of the entry block differ
// (the original keeps gGameState in r4 and loads it first).
// Best so far (30 diff lines, was 62 with `u8 idx = gGameState.unk119;` at the top): idx
// assigned in the first marker test. What remains is the entry block: the original loads
// unk119 into r2 before the unk135 test and copies gGameState to r4. With idx loaded first,
// global-alloc puts idx in r0 and the unk135 value in r1 (someone prefers r0); idx types,
// a `sel` local, a GameState pointer and nested/early-return forms of the unk135 case
// didn't change that.
// Diagnosis (2026-09-28, prio.py + global.c): with `u8 idx = gGameState.unk119;` at the top
// the code shape is right (the gGameState copy that the original keeps in r4 appears) but
// global-alloc goes unk135 -> r1, base -> r2, idx -> r0 (62 lines; original: unk135 r0,
// base r1, idx r2, &unk135 r3, copy r4). unk135 skips r0 because idx (lower priority,
// conflicting) prefers r0 via `idx << 3` (local-alloc'd to r0), and find_reg's first pass
// avoids regs in regs_someone_prefers. &unk119/&unk135 are global pseudos (REG_EQUIV
// gGameState+off). Tried at the top: idx u8/u16/u32/s32, split decl, a u8/u32/s32 sel local,
// switch on unk135, `!= 0x21 ||` and nested/early-return forms, a GameState pointer (drops
// the r4 copy), a marker pointer, `u8 *p = &unk119`: all 56-76 lines.
/*
#define ID(v, c) ((s32)(((v) << 16) + ((c) << 16)) >> 16)

void Func_0201B7D4(void)
{
    u8 idx;

    if (gGameState.unk135 != 0)
    {
        if (gGameState.unk135 == 0x21 && gGameState.flags.unk5_6)
            ShowMessage(0x13E, 1);
        else
            ShowMessage(gGameState.unk135 + 0x14D, 1);
    }
    else if (idx = gGameState.unk119, gGameState.markers[idx].type == 0x13)
        ShowMessage(gMsgIds_02024278[gGameState.markers[idx].unk6 - 1], 1);
    else if (gGameState.markers[idx].type == 0x14)
        ShowMessage(0x5D, 1);
    else if (gGameState.markers[idx].unk7 <= 0x23)
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x63), 1);
    else if (gGameState.markers[idx].unk7 <= 0x27)
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x7C), 1);
    else if (gGameState.markers[idx].unk7 <= 0x2C)
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x82), 1);
    else if (gGameState.markers[idx].unk7 <= 0x38)
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x87), 1);
    else if (gGameState.markers[idx].unk7 <= 0x40)
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x8F), 1);
    else
        ShowMessage(ID(gGameState.markers[idx].unk7, 0x6F), 1);
}
*/
INCLUDE_ASM(Func_0201B7D4);

// Act on the current selection (unk119: a marker, or unk135): 0: send the switch record,
// 1: a random reaction sound, 2: play the selection's sound via Func_0201B7D4.
void Func_0201B8C8(void)
{
    u8 result = 0;

    if (gGameState.unk119 != 0)
    {
        if (gGameState.markers[gGameState.unk119].type == 12)
        {
            if (gGameState.unk11C)
                result = 2;
            else
                result = 1;
        }
        else if (gGameState.markers[gGameState.unk119].type == 13)
        {
            if (gGameState.unk11D)
                result = 2;
            else
                result = 1;
        }
        else if (gGameState.markers[gGameState.unk119].type == 5)
        {
            if (gGameState.unk010 & (1 << gGameState.markers[gGameState.unk119].unk6))
                return;
            result = 1;
        }
        else if (gGameState.markers[gGameState.unk119].type == 16)
        {
            if ((gGameState.unk026 >> gGameState.markers[gGameState.unk119].unk6) & 1)
                result = 2;
            else
                result = 1;
        }
        else
        {
            if (gGameState.markers[gGameState.unk119].type == 7)
                result = 1;
            else
                result = 2;
        }
    }
    else if (gGameState.unk135)
    {
        result = 2;
    }
    if (result != 2)
    {
        if (result == 0)
        {
            if (gGameState.switchType == 15)
            {
                gGameState.unk1F8 = gGameState.switchMsg;
                gGameState.switchMsg = 0x302;
            }
            if (gGameState.switchType != 0 && gGameState.switchType != 0xFF)
            {
                Mode1SendSwitch(3);
                SoundPlay(0x21 - SOUND_ID_SHIFT, 0);
            }
        }
        else
        {
            if ((gMain.random & 0xFF) > 0xCB)
                ShowMessage(0x17, 1);
            else if ((gMain.random & 0xFF) > 0x65)
                ShowMessage(0x16, 1);
            else
                ShowMessage(0x15, 1);
            gGameState.unk156 = gMain.random % 60 + 30;
            if (gGameState.markers[gGameState.unk119].type != 7)
                gGameState.unk11A = 2;
        }
    }
    else
    {
        Func_0201B7D4();
        SoundPlay(2, 0);
    }
}

// The original pads the end of this object with zeros (agbcc would pad with nops).
asm(".align 2, 0");
