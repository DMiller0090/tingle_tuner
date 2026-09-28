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
    gFollowIconSprite,
    gFollowIconSprite,
    gMapCursorIconSprite,
    (const u8 *)gTargetSprite
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

const u16 gTowerChartTiles[2] =
{
    0xCC, 0xE0
};

const s16 gFriendMsgIds[18] =
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

// Draw the map markers: step the markers' animation timers (while gGameState.animateMarkers is
// set), then draw every marker of a known type (1-21) that is on screen with its type's sprite
// (gSpriteTable[type], or a special one), rotated by MapMarker.angle, plus a highlight ring when
// MapMarker.highlight is set; last the target (icons[2]) when the chart is closed. The player
// (type 1) is drawn at half size over the icon of the character he controls (MapMarker.param);
// the cursor (type 3) is hidden while items are disabled (an item effect runs) unless it is held
// or the effect is 14 (the bomb countdown), and unexamined markers of types 5, 12, 13 and 16
// are drawn as gSpriteTable[7].
// Matching notes: every local is declared at the top in this order, which gives the original's
// spill slots (i, dx, scale, mosaic, then the dy << 16 temp). The original compares the type
// twice (`type == 3 ... type == 3`); cse deletes the second test for any C form that yields the
// same value, so `type` is a hard-register variable (FAKEMATCH): cse records no equivalences
// for non-fixed hard registers. It is copied from `t` after the continue checks so that
// local-alloc ties the 0x3F constant to `t` (`movs r2, #0x3f; ands r2, r0`). The icon at the end
// reuses dx/dy, which settles the global-alloc ties (JP: dy before the highlight call's zero
// constant, so dy gets r2 and dx's copy r6; US: mosaic, not dx, gets sl). US passes 12 for type
// 1's param == 3 sprite and frame 0 for the icon; its pal is set but never gets a register.
#define SPRITE_FRAMES(p) ((const struct SpriteFrame *)(p))

void DrawMapMarkers(void)
{
    struct MapMarker *marker = gGameState.markers;
    s32 i;
    const struct SpriteFrame *frames;
    u8 frame;
    s16 dx;
    u16 scale;
    s16 dy;
    u8 pal2;
    u8 pal;
    u8 mosaic = gMain.mosaic != 0;

    ObjUpdateAll();
    if (gGameState.animateMarkers)
    {
        if (gGameState.playerBlinkTimer > 19)
        {
            gGameState.playerBlinkTimer = 0;
            gGameState.playerFrame ^= 1;
        }
        else
        {
            gGameState.playerBlinkTimer++;
        }
        if (gGameState.char2Timer == 0)
        {
            if (++gGameState.char2Frame > 8)
                gGameState.char2Frame = 0;
            gGameState.char2Timer = gCharIcon2[gGameState.char2Frame].duration;
        }
        else
        {
            gGameState.char2Timer--;
        }
        if (gGameState.char3Timer == 0)
        {
            gGameState.char3Frame ^= 1;
            gGameState.char3Timer = gCharIcon4[gGameState.char3Frame].duration;
        }
        else
        {
            gGameState.char3Timer--;
        }
        if (gGameState.char4Timer == 0)
        {
            gGameState.char4Frame ^= 1;
            gGameState.char4Timer = gCharIcon4[gGameState.char4Frame].duration;
        }
        else
        {
            gGameState.char4Timer--;
        }
        if (gGameState.enemyBlinkTimer > 9)
        {
            gGameState.enemyBlinkTimer = 0;
            gGameState.enemyFrame ^= 1;
        }
        else
        {
            gGameState.enemyBlinkTimer++;
        }
        if (gGameState.holdState != 0)
        {
            if (gGameState.flags.cursorHeld)
            {
                if (gGameState.holdTimer == 0)
                {
                    gGameState.holdFrame++;
                    if (gGameState.holdState == 1)
                    {
                        if (gGameState.holdFrame > 6)
                            gGameState.holdFrame = 0;
                        frames = &SPRITE_FRAMES(gCursorHeldSprite)[gGameState.holdFrame];
                        gGameState.holdTimer = frames->duration;
                    }
                    else if (gGameState.holdFrame > 18)
                    {
                        gGameState.holdState = 0;
                        Mode1SendSwitch(3);
                    }
                    else
                    {
                        frames = &SPRITE_FRAMES(gCursorReleaseSprite)[gGameState.holdFrame];
                        gGameState.holdTimer = frames->duration;
                    }
                }
                else
                {
                    gGameState.holdTimer--;
                }
            }
            else if (gGameState.holdState == 1)
            {
                gGameState.holdState = 0;
            }
        }
        else if (gGameState.itemsEnabled && gGameState.flags.busy == 0)
        {
            if (gGameState.cursorBlinkTimer > 29)
            {
                gGameState.cursorBlinkTimer = 0;
                gGameState.cursorFrame ^= 1;
            }
            else
            {
                gGameState.cursorBlinkTimer++;
            }
        }
        if (gGameState.type9Timer != 0)
        {
            gGameState.type9Timer--;
        }
        else
        {
            gGameState.type9Frame ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[9])[gGameState.type9Frame];
            gGameState.type9Timer = frames->duration;
        }
        if (gGameState.type13Timer != 0)
        {
            gGameState.type13Timer--;
        }
        else
        {
            if (++gGameState.type13Frame > 5)
                gGameState.type13Frame = 0;
            frames = &SPRITE_FRAMES(gSpriteTable[13])[gGameState.type13Frame];
            gGameState.type13Timer = frames->duration;
        }
        if (gGameState.type16Timer != 0)
        {
            gGameState.type16Timer--;
        }
        else
        {
            gGameState.type16Frame ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[16])[gGameState.type16Frame];
            gGameState.type16Timer = frames->duration;
        }
        if (gGameState.friendTimer != 0)
        {
            gGameState.friendTimer--;
        }
        else
        {
            gGameState.friendFrame ^= 1;
            frames = &SPRITE_FRAMES(gSpriteTable[19])[gGameState.friendFrame];
            gGameState.friendTimer = frames->duration;
        }
    }

    gGameState.enemyHighlighted = 0;
    for (i = 0; i < gGameState.markerCount; marker++, i++)
    {
        register u32 type asm("r2");  // FAKEMATCH
        u32 t = 0x3F & ((u8 *)marker)[5];

        if (t == 0 || marker->type > 21)
            continue;
        type = t;
        scale = 0;
        if (!gGameState.itemsEnabled && type == 3 && gGameState.holdState == 0 && type == 3)
        {
            if (!gGameState.effectActive)
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
            if (marker->param == 0)
                DrawSpriteAt(gCharIcon0, 0, dx, dy, mosaic, pal, 1);
            else if (marker->param == 1)
                DrawSpriteAt(gCharIcon1, 0, dx, dy, mosaic, pal, 1);
            else if (marker->param == 2)
                DrawSpriteAt(gCharIcon2, gGameState.char2Frame, dx, dy, mosaic, pal, 1);
            else if (marker->param == 3)
#ifdef VERSION_GZLJ01
                DrawSpriteAt(gCharIcon3, gGameState.char3Frame, dx, dy, mosaic, pal, 1);
#else
                DrawSpriteAt(gCharIcon3, gGameState.char3Frame, dx, dy, mosaic, 12, 1);
#endif
            else if (marker->param == 4)
                DrawSpriteAt(gCharIcon4, gGameState.char4Frame, dx, dy, mosaic, pal, 1);
            scale = 0x80;
            frame = gGameState.playerFrame;
        }
        else if (marker->type == 3)
        {
            if (gGameState.holdState != 0)
            {
                frames = gCursorReleaseSprite;
                if (gGameState.holdState == 1)
                    frames = gCursorHeldSprite;
                frame = gGameState.holdFrame;
            }
            else if (gGameState.selection != 0)
            {
                frames = gCursorSelectSprite;
                frame = 0;
            }
            else if (gGameState.flags.busy == 0 && (gGameState.inMessage != 0 || gGameState.active != 0))
            {
                frame = gGameState.cursorFrame;
            }
            else
            {
                frames = gChoiceCursorSprite;
                frame = 1;
            }
        }
        else if (marker->type == 2)
        {
            if (marker->gbaName == 0x21)
            {
                frames = gSprite_0202BD80;
                frame = 0;
            }
            else
            {
                frame = gGameState.enemyFrame;
            }
        }
        else if (marker->type == 6)
        {
            if (marker->gbaName == 0x31)
                frames = gSprite_0202BD80;
            frame = 0;
        }
        else if (marker->type == 9)
        {
            if (gGameState.type9Frame != 0)
            {
                dx += gMarkerDirOfsX[marker->angle >> 5];
                dy += gMarkerDirOfsY[marker->angle >> 5];
            }
            frame = 0;
        }
        else if (marker->type == 13)
        {
            if (gGameState.revealedType13 == 0)
            {
                frames = SPRITE_FRAMES(gSpriteTable[7]);
                frame = 0;
            }
            else
            {
                frame = gGameState.type13Frame;
            }
        }
        else if (marker->type == 16)
        {
            if (!((gGameState.revealedType16 >> marker->param) & 1))
            {
                if (gGameState.selectedType == 16 && gGameState.selectedMarker == i)
                {
                    frames = SPRITE_FRAMES(gSpriteTable[7]);
                    frame = 0;
                }
                else
                {
                    frames = gChoiceCursorSprite;
                    frame = 1;
                }
            }
            else
            {
                frame = gGameState.type16Frame;
            }
        }
        else if (marker->type == 5)
        {
            if (marker->param != 0xFF && !(gGameState.revealedType5 & (1 << marker->param)))
                frames = SPRITE_FRAMES(gSpriteTable[7]);
            frame = 0;
        }
        else if (marker->type == 12)
        {
            if (gGameState.revealedType12 == 0)
                frames = SPRITE_FRAMES(gSpriteTable[7]);
            frame = 0;
        }
        else if (marker->type == 19)
        {
            if (marker->param == 0)
            {
                frames = gFriendIcon0;
                frame = 0;
            }
            else if (marker->param == 1)
            {
                frames = gCharIcon1;
                frame = 0;
            }
            else if (marker->param == 2)
            {
                frames = gCharIcon2;
                frame = gGameState.char2Frame;
            }
            else if (marker->param == 3)
            {
                frames = gCharIcon3;
                frame = gGameState.char3Frame;
            }
            else
            {
                frame = gGameState.friendFrame;
            }
        }
        else
        {
            frame = 0;
        }
        if (marker->highlight)
        {
            gGameState.enemyHighlighted = marker->type == 2 ? 1 : 0;
            if (marker->highlight == 1)
                pal2 = 3;
            else
                pal2 = 2;
            DrawSprite(gMarkerHighlightSprite, 0, dx, dy - 8, marker->angle + 0x80, mosaic, 0, 0, 0, pal, pal2 + 1);
        }
        DrawSprite(frames, frame, dx, dy, marker->angle + 0x80, mosaic, scale, 0, 0, pal, 0);
    }

    if (gMain.mapImageMode == 0 && gGameState.chartOpen == 0 && gGameState.icons[2].type != 0)
    {
        dx = gGameState.icons[2].x - gMain.bgOfs[3][0];
        dy = gGameState.icons[2].y - gMain.bgOfs[3][1];

        if ((u16)(dx + 0x40) <= 0xFF && (u16)(dy + 0x40) <= 0xFF)
#ifdef VERSION_GZLJ01
            DrawSpriteAt(gTargetSprite, gGameState.icons[2].frame, dx, dy, mosaic, 0, 1);
#else
            DrawSpriteAt(gTargetSprite, 0, dx, dy, mosaic, 0, 1);
#endif
    }
}

// Highlight the rooms on the four BG map blocks at 0x0600C000 (one room each, gGameState.seaSquare):
// clear the palette bits of the block showing marker 1's room, then set palette bit 1 (|= 0x2000)
// on a 2x2 or 4x4 tile square around every gChartPoints entry in each block's room. Room 0x19's
// entries only show once flags.towerRaised is set.
#define CLEAR_PAL(p) (*(p) &= 0x1FFF, (p)++)
#define CLEAR_PAL_4(p) CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p)
#define CLEAR_PAL_32(p) CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); \
    CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p)

void HighlightMapRooms(void)
{
    u8 m = FindMarker(1);
    u16 x, y;
    s32 n, i, j, k;
    u16 *map;

    // x/y are reused: the room (x + y * 7), the block index and a tile position; n is also the
    // row offset.
    x = gGameState.markers[m].x >> 8;
    y = (gGameState.markers[m].y >> 8) * 7;
    x += y;
    for (n = 0; n <= 3; n++)
    {
        if (gGameState.seaSquare[n] == x)
        {
            x = (gGameState.markers[m].x >> 8) & 1;
            y = ((gGameState.markers[m].y >> 8) & 1) << 1;
            x |= y;
            map = (u16 *)(0x0600C000 + (x << 11));
            for (i = 0; i < 32; i++)
            {
                CLEAR_PAL_32(map);
            }
            break;
        }
    }
    for (i = 0; i <= 3; i++)
    {
        if (gGameState.seaSquare[i] == 0x19 && !gGameState.flags.towerRaised)
            continue;
        for (j = 0; j <= 0x30; j++)
        {
            struct SeaChartEntry *e = &gChartPoints[j];

            if (gGameState.seaSquare[i] == e->square)
            {
                s32 size;

                if (e->big)
                {
                    x = (e->x >> 3) - 2;
                    y = (e->y >> 3) - 2;
                    size = 4;
                }
                else
                {
                    x = (e->x >> 3) - 1;
                    y = (e->y >> 3) - 1;
                    size = 2;
                }
                map = (u16 *)(0x0600C000 + (i << 11)) + (y * 32 + x);
                for (n = 0; n < size * 32; n += 32)
                {
                    for (k = 0; k < size; k++)
                        map[n + k] |= 0x2000;
                }
            }
        }
    }
}

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
    if (gGameState.markers[i].x == gGameState.followLastX && gGameState.markers[i].y == gGameState.followLastY)
        gGameState.followStillCount--;
    else
        gGameState.followStillCount = 2;
    if ((s8)gGameState.followStillCount <= 0)
    {
        gGameState.followStill = 1;
        gGameState.followStillCount = 2;
    }
    gGameState.followLastX = gGameState.markers[i].x;
    gGameState.followLastY = gGameState.markers[i].y;
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

// Per-frame map update. Sea chart mode (gMain.mapImageMode == 0): find the four sea chart squares
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
    gGameState.followStill = 0;
    if (gMain.mapImageMode == 0)
    {
        s32 j;

        DarkenUnvisitedSquares();
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
            if (gGameState.viewLocked == 0)
                FollowMarkerStreamedMap();
        }
        else if (gGameState.itemsEnabled)
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
            sprite = gMapScrollCursorSprite;
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
    if (gGameState.chartOpen == 0)
        // DrawMapMarkers takes no parameters, but this passes two.
        ((void (*)())DrawMapMarkers)(&gGameState, gMain.frameCount);
}

// Follow the player's marker without redrawing the map (the fixed sea view or the streamed
// map image).
void UpdateMapView(void)
{
    if (gMain.mapImageMode == 0)
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
    if (gMain.mapImageMode != 0)
    {
        if (gGameState.mapLoaded == 0)
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
// from the player, pulse it faster the closer the player gets, show the near and arrival
// messages, and (once Link has the Wind Waker, flags.hasTact) name its direction (message
// 0x19C + dir) when the wind (flags.windDir) blows more than 9/256 of a turn away from it.
// FAKEMATCH: dx/dy and the sin x/y are block locals (local-alloc gives them r2/r1 and r2/r3).
// Each sin coordinate is narrowed in place before the shift: the `(s16)x` temporaries mention
// x before its last set, so combine marks x invalid and keeps the s16 extensions of x + 0xB8
// and y + 0x58 (with a plain `x >>= 3` it proves them redundant and drops them). The extra
// narrowings and the (s16) ArcTan2 operands (the BIOS call takes s16) compile to nothing, but
// their insns still count in the live length of the chartOpen value (the zero kept in r5):
// global-alloc must rank level (14 refs) above that zero (15 refs) so level takes r4, the
// zero r5 and gGameState r6, which needs the zero's live length >= 15/14 of level's. The
// separate timer decrement/increment statements shorten level's range.
void UpdateTargetCompass(void)
{
    u8 angle;
    s32 level;

    if (gGameState.inMessage != 0)
        return;
    if (gGameState.chartOpen != 0)
        return;
    if (gGameState.icons[2].type != 3)
        return;
    {
        u8 i = FindMarker(1);
        s32 dx, dy;

        dx = gGameState.icons[2].x - gGameState.markers[i].x;
        dy = gGameState.icons[2].y - gGameState.markers[i].y;
        angle = ((u16)~ArcTan2((s16)(dx >> 4), (s16)(dy >> 4)) + 0x4001) >> 8;
    }
    {
        s32 x, y;

        x = gSinTable[angle];
        x = (s16)x;
        x = (s16)x;
        x = (s16)x >> 3;
        y = (gSinTable + 64)[angle];
        y = (s16)y;
        y = (s16)y;
        y = (s16)y >> 3;
        DrawSprite(gMapIconSprites[gGameState.icons[2].type], 0, (u16)(x + 0xB8), (u16)(y + 0x58), 0, 0, 0, 0, 0, 0, 0);
    }
    level = MarkerDistance(0);
    if ((u32)level <= 0x18)
    {
        if (gGameState.chartOpen == 0)
        {
            ShowMessage(0x1A5, 1);
            gGameState.icons[2].type = 0;
            gGameState.icons[1].type = 0;
            gGameState.placingTarget = 0;
        }
        level = 0;
        gGameState.targetNear = 0;
    }
    else if ((u32)level <= 0x7F)
    {
        if (gGameState.chartOpen == 0 && gGameState.targetNear == 0)
        {
            ShowMessage(0x1A4, 1);
            gGameState.targetNear = 1;
        }
        level = 1;
    }
    else if ((u32)level <= 0x1FF)
    {
        level = 2;
        gGameState.targetNear = 0;
    }
    else if ((u32)level <= 0x3FF)
    {
        level = 3;
        gGameState.targetNear = 0;
    }
    else
    {
        level = 4;
        gGameState.targetNear = 0;
    }
    if (gGameState.targetBlinkOn != 0)
    {
        gGameState.targetBlinkTimer--;
        if (gGameState.targetBlinkTimer == 0)
            gGameState.targetBlinkOn = 0;
    }
    else
    {
        gGameState.targetBlinkTimer++;
        if (gGameState.targetBlinkTimer >= gTargetBlinkPeriods[level])
        {
            gGameState.targetBlinkTimer = gTargetBlinkPeriods[level];
            gGameState.targetBlinkOn = 1;
        }
    }
    gGameState.targetScale = (gGameState.targetBlinkTimer << 7) / gTargetBlinkPeriods[level] + 0x100;
    DrawSprite(gCompassArrowSprite, 0, 200, 72, angle + 0x80, 0, gGameState.targetScale, 0, 16, 0, 0);
    if (gGameState.chartOpen == 0 && gGameState.flags.hasTact && level > 1
     && (u8)(gGameState.flags.windDir - angle + 9) > 18)
    {
        angle = ~angle + 1;
        if (((angle + 8) & 0x1F) <= 0x10)
        {
            u8 dir = (u8)(angle + 0x88) >> 5;

            if (gGameState.targetDir != dir && gGameState.inMessage == 0)
            {
                ShowMessage(0x19C + dir, 1);
                gGameState.targetDir = dir;
            }
        }
        else
        {
            gGameState.targetDir = 0xFF;
        }
    }
}

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

// Icon type 2: the cursor. While placing a target (placingTarget == 1), move it a sea chart square
// (16 pixels) at a time with the D-pad and place the target (icons[2]) with A. Then name the
// square under the cursor if its visitedSquares bit is set.
void MapIconCursor(struct MapIcon *icon)
{
    u16 keys;
    u16 pos;
    u8 square;
    s32 i;

    if (gGameState.placingTarget == 1)
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
    if ((gGameState.visitedSquares[square >> 3] >> (square & 7)) & 1)
    {
        for (i = 0; i < 49; i++)
        {
            if (gChartPoints[i].square == square)
            {
                if (square == 25 && !gGameState.flags.towerRaised)
                {
                    gGameState.msgOpen = 0;
                }
                else
                {
                    if (square == 32 && gGameState.flags.hasCabanaDeed)
                        ShowMessage(0x13E, 1);
                    else
                        ShowMessage(0x14E + gChartPoints[i].square, 1);
#ifndef VERSION_GZLJ01
                    gGameState.msgWaitTimer = 0;
#endif
                }
            }
        }
    }
    else
    {
        gGameState.msgOpen = 0;
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
                DrawSprite(gTargetSprite, 0, gPearlMarkerPos[i * 2] + 12,
                           gPearlMarkerPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
    else if (gGameState.flags.pearlsToPlace)
    {
        for (i = 0; i < 3; i++)
            if ((gGameState.flags.pearlsToPlace >> i) & 1)
                DrawSprite(gTargetSprite, 0, gTriangleIslandPos[i * 2] + 12,
                           gTriangleIslandPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
    else if (gGameState.flags.bossesToBeat)
    {
        for (i = 0; i < 2; i++)
            if ((gGameState.flags.bossesToBeat >> i) & 1)
                DrawSprite(gTargetSprite, 0, gTempleMarkerPos[i * 2] + 12,
                           gTempleMarkerPos[i * 2 + 1] + 12, 0, mosaic, 0, 0, 0, 0, 8);
    }
}

// B closes and L toggles the sea chart overlay (chartOpen) while itemsEnabled allows it; while it
// is open, A (once) calls InitCursorIcon and the quest markers are drawn.
void UpdateChartOverlay(void)
{
    u16 tile;
    u16 *map;

    if (gGameState.itemsEnabled && (gMain.newKeys & 0x202))
    {
        gGameState.placingTarget = 0;
        gGameState.icons[1].type = 0;
        if (gMain.newKeys & 2)
        {
            if (gGameState.chartOpen)
            {
                gGameState.chartOpen = 0;
                gGameState.msgOpen = 0;
                SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
            }
        }
        else
        {
            gGameState.chartOpen ^= 1;
#ifndef VERSION_GZLJ01
            if (gGameState.chartOpen == 0)
                MsgClose();
#endif
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        }
        if (gGameState.chartOpen)
        {
            gMain.bgOfs[1][1] = 0x7C;
            gGameState.aButtonAction = 2;
            gGameState.hudChartLabel = 1;
            InitMarkerIcon();
            if (gGameState.icons[2].type)
                InitTargetIcon();
            map = (u16 *)0x060075D2;
            tile = gTowerChartTiles[gGameState.flags.towerRaised];
            if (!(gGameState.visitedSquares[3] & 2))
                tile |= 0x6000;
            if (gGameState.flags.towerRaised)
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
            gGameState.aButtonAction = 0;
            gGameState.hudChartLabel = 0;
        }
        RedrawHudItem();
    }
    else if (gGameState.chartOpen)
    {
        gMain.bgOfs[2][0] = -4;
        gMain.bgOfs[2][1] = 4;
        UpdateMapIcons();
        if (gGameState.placingTarget == 0 && (gMain.newKeys & 1))
        {
            InitCursorIcon();
            gGameState.placingTarget = 1;
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        }
        DrawQuestMarkers();
    }
    UpdateTargetCompass();
}

// Darken the sea chart's 7x7 squares (2x2 tiles each, from map row 17 column 1 of the BG
// map at 0x06007000) whose bit in gGameState.visitedSquares is clear: palette 6 instead of 0.
void DarkenUnvisitedSquares(void)
{
    s32 i;
    u32 off;
    u16 *p;

    for (i = 0; i < 49; i++)
    {
        off = (((i / 7) * 2 + 17) * 32 + ((i % 7) * 2 + 1)) * 2;
        p = (u16 *)(0x06007000 + off);
        if ((gGameState.visitedSquares[i >> 3] >> (i & 7)) & 1)
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

// What the cursor's marker (type 3) is on: returns the index of a marker within 8 pixels (and
// sets selectedType to its type), else on the sea looks the position up in the slot 14 table
// of sea chart spots (setting selectedSquare to the square's id + 1). selection becomes 1 when
// something is selectable (voice clip 1 when it was 0; mostly a 2 is kept) and 0 when not.
// Only the first type test uses `type`; the rest re-read the field, which cse turns into a
// copy of type's register (the `adds r0, r6, #0` after the first compare). `r` is s16: the
// comparisons then use a widened copy of it.
s32 UpdateSelection(void)
{
    u8 cursor;
    s32 i;
    u8 type;
    u8 square;
    u16 ex;
    u16 ey;
    s16 r;

    if (gGameState.holdState != 0)
    {
        gGameState.selectedType = 0;
        gGameState.selection = 0;
        return 0;
    }
    gGameState.selectedSquare = 0;
    cursor = FindMarker(3);
    i = FindMarker(0x14);
    if (i != 0xFF && gGameState.awayFromType20 == 0)
    {
        if ((u16)(gGameState.markers[i].x - gGameState.markers[cursor].x + 8) > 16 || (u16)(gGameState.markers[i].y - gGameState.markers[cursor].y + 8) > 16)
            gGameState.awayFromType20 = 1;
    }
    if ((u8)(gGameState.switchType - 1) <= 1 || gGameState.switchType == 4
     || (gGameState.switchType == 13 && gGameState.flags.isFree))
    {
        if (gGameState.selection == 0)
        {
            SoundPlay(1, 0);
            gGameState.selection = 1;
        }
        return 0;
    }
    if (gGameState.switchType == 0xFF)
    {
        gGameState.selection = 1;
        return 0;
    }
    if (cursor == 0xFF)
        goto none;
    for (i = 0; i < gGameState.markerCount; i++)
    {
        type = gGameState.markers[i].type;
        if (type == 1 || gGameState.markers[i].type == 3 || gGameState.markers[i].type == 4
         || gGameState.markers[i].type == 0x11 || gGameState.markers[i].type == 0x12)
            continue;
        if ((u16)(gGameState.markers[i].x - gGameState.markers[cursor].x + 8) > 16)
            continue;
        if ((u16)(gGameState.markers[i].y - gGameState.markers[cursor].y + 8) > 16)
            continue;
        gGameState.selectedType = type;
        if (gGameState.markers[i].gbaName != 0)
        {
            if (gGameState.markers[i].type == 2 || gGameState.markers[i].type == 6 || gGameState.markers[i].type == 14 || gGameState.markers[i].type == 15)
            {
                gGameState.selection = 1;
                return i;
            }
            if (gGameState.markers[i].type == 5)
            {
                if (gGameState.markers[i].param == 0xFF)
                    continue;
                if (gGameState.revealedType5 & (1 << gGameState.markers[i].param))
                    continue;
                if (gGameState.selection == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.selection = 1;
                }
                return i;
            }
            if (gGameState.markers[i].type == 12)
            {
                if (gGameState.revealedType12)
                    continue;
                if (gGameState.selection == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.selection = 1;
                }
                return i;
            }
            if (gGameState.selection == 0)
            {
                if (gGameState.flags.isFree)
                    SoundPlay(1, 0);
                gGameState.selection = 1;
            }
            return i;
        }
        if (gGameState.markers[i].type == 7)
        {
            if (gGameState.switchType == 4 || gGameState.switchType == 10)
            {
                if (gGameState.selection == 0)
                {
                    SoundPlay(1, 0);
                    gGameState.selection = 1;
                }
                return i;
            }
            gGameState.selection = 0;
            return 0;
        }
        if ((u8)(gGameState.markers[i].type - 8) <= 3 || gGameState.markers[i].type == 0x15)
        {
            if (gGameState.switchType == 3 || gGameState.switchType == 15)
                gGameState.selection = 1;
            else
                gGameState.selection = 0;
            return 0;
        }
        if (gGameState.markers[i].type == 0x13)
        {
            if ((u8)(gGameState.markers[i].param - 1) <= 2)
            {
                gGameState.selection = 1;
                return i;
            }
            return 0;
        }
        if (gGameState.markers[i].type == 0x14)
        {
            if (gGameState.awayFromType20)
            {
                gGameState.selection = 1;
                return i;
            }
            gGameState.selectedType = 0;
            return 0;
        }
        if (gGameState.selection == 0)
        {
            if (gGameState.flags.isFree)
                SoundPlay(1, 0);
            gGameState.selection = 1;
        }
        return 0;
    }
    if (gMain.mapImageMode == 0)
    {
        square = (gGameState.markers[cursor].x >> 8) + (gGameState.markers[cursor].y >> 8) * 7;
        for (i = 0; i < 49; i++)
        {
            if (gChartPoints[i].square != square)
                continue;
            if (square == 25 && !gGameState.flags.towerRaised)
                return 0;
            if (gChartPoints[i].big)
            {
                ex = (gChartPoints[i].x - 16) & 0xF8;
                ey = (gChartPoints[i].y - 16) & 0xF8;
                r = 32;
            }
            else
            {
                ex = gChartPoints[i].x - 8;
                ey = gChartPoints[i].y - 8;
                r = 8;
            }
            if ((u8)(gGameState.markers[cursor].x - ex) > r)
                continue;
            if ((u8)(gGameState.markers[cursor].y - ey) > r)
                continue;
            gGameState.selectedSquare = gChartPoints[i].square + 1;
            gGameState.selection = 1;
            return 0;
        }
    }
none:
    gGameState.selectedType = 0;
    gGameState.selection = 0;
    return 0;
}

// Show the message for what the cursor is on: the square's name (0x14E + square; square 32,
// the Private Oasis, has 0x13E once Link has the Cabana Deed), a friend's (gFriendMsgIds), or
// the marker's by its gbaName.
// The ids are computed in the shifted domain, ((v << 16) + (c << 16)) >> 16, which agbcc only
// emits for an explicit shift pair (ID below).
#define ID(v, c) ((s32)(((v) << 16) + ((c) << 16)) >> 16)

void ShowSelectionMsg(void)
{
    u8 idx = gGameState.selectedMarker;
    // FAKEMATCH: 0x21 in a variable (found with decomp-permuter) moves the selectedSquare value
    // from r1 to r0 and idx to r2, as in the original; a literal gives idx the r0 preference.
    s32 c = 0x21;

    if (gGameState.selectedSquare != 0)
    {
        if (gGameState.selectedSquare == c && gGameState.flags.hasCabanaDeed)
            ShowMessage(0x13E, 1);
        else
            ShowMessage(gGameState.selectedSquare + 0x14D, 1);
    }
    else if (gGameState.markers[idx].type == 0x13)
        ShowMessage(gFriendMsgIds[gGameState.markers[idx].param - 1], 1);
    else if (gGameState.markers[idx].type == 0x14)
        ShowMessage(0x5D, 1);
    else if (gGameState.markers[idx].gbaName <= 0x23)
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x63), 1);
    else if (gGameState.markers[idx].gbaName <= 0x27)
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x7C), 1);
    else if (gGameState.markers[idx].gbaName <= 0x2C)
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x82), 1);
    else if (gGameState.markers[idx].gbaName <= 0x38)
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x87), 1);
    else if (gGameState.markers[idx].gbaName <= 0x40)
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x8F), 1);
    else
        ShowMessage(ID(gGameState.markers[idx].gbaName, 0x6F), 1);
}

// A on the selection (selectedMarker: a marker, or selectedSquare): 0 (nothing selected): send
// the pending switch record; 1 (not examined yet): a random thinking message (0x15-0x17), and
// Mode1State2 shows the selection's message after reactionTimer; 2: ShowSelectionMsg now.
void ExamineSelection(void)
{
    u8 result = 0;

    if (gGameState.selectedMarker != 0)
    {
        if (gGameState.markers[gGameState.selectedMarker].type == 12)
        {
            if (gGameState.revealedType12)
                result = 2;
            else
                result = 1;
        }
        else if (gGameState.markers[gGameState.selectedMarker].type == 13)
        {
            if (gGameState.revealedType13)
                result = 2;
            else
                result = 1;
        }
        else if (gGameState.markers[gGameState.selectedMarker].type == 5)
        {
            if (gGameState.revealedType5 & (1 << gGameState.markers[gGameState.selectedMarker].param))
                return;
            result = 1;
        }
        else if (gGameState.markers[gGameState.selectedMarker].type == 16)
        {
            if ((gGameState.revealedType16 >> gGameState.markers[gGameState.selectedMarker].param) & 1)
                result = 2;
            else
                result = 1;
        }
        else
        {
            if (gGameState.markers[gGameState.selectedMarker].type == 7)
                result = 1;
            else
                result = 2;
        }
    }
    else if (gGameState.selectedSquare)
    {
        result = 2;
    }
    if (result != 2)
    {
        if (result == 0)
        {
            if (gGameState.switchType == 15)
            {
                gGameState.nextMsg = gGameState.switchMsg;
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
            gGameState.reactionTimer = gMain.random % 60 + 30;
            if (gGameState.markers[gGameState.selectedMarker].type != 7)
                gGameState.selection = 2;
        }
    }
    else
    {
        ShowSelectionMsg();
        SoundPlay(2, 0);
    }
}

// The original pads the end of this object with zeros (agbcc would pad with nops).
asm(".align 2, 0");
