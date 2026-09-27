#include "global.h"
#include "gba/syscall.h"
#include "main.h"
#include "game.h"
#include "sound.h"
#include "gba_com.h"

// Game code after heap.c, up to the sound driver (GZLJ01 02018884-02020E28). Not split into
// objects yet: see docs/layout.md for the groups the .rodata order implies.

s16 EaseTowardMinMax(s16 *pos, s16 target, s16 div, s16 maxStep, s16 minStep)
{
    s16 diff = target - *pos;
    s16 step;

    if (*pos != target)
    {
        step = diff / div;
        if (step > minStep || step < -minStep)
        {
            if (step > maxStep)
                step = maxStep;
            if (step < -maxStep)
                step = -maxStep;
            *pos += step;
        }
        else if (diff >= 0)
        {
            *pos += minStep;
            if ((s16)(target - *pos) <= 0)
                *pos = target;
        }
        else
        {
            *pos -= minStep;
            if ((s16)(target - *pos) >= 0)
                *pos = target;
        }
    }
    return target - *pos;
}

void EaseTowardMax(s16 *pos, s16 target, s16 div, s16 maxStep)
{
    s16 step = (s16)(target - *pos) / div;

    if (step > maxStep)
        *pos += maxStep;
    else if (step < -maxStep)
        *pos -= maxStep;
    else
        *pos += step;
}

bool32 StepTowardS8(s8 *pos, s8 target, s8 speed)
{
    if (speed != 0)
    {
        if (*pos > target)
            speed = -speed;
        *pos += speed;
        if ((*pos - target) * speed >= 0)
        {
            *pos = target;
            return TRUE;
        }
    }
    else if (*pos == target)
    {
        return TRUE;
    }
    return FALSE;
}

bool32 StepToward(s16 *pos, s16 target, s16 speed)
{
    if (speed != 0)
    {
        if (*pos > target)
            speed = -speed;
        *pos += speed;
        if ((*pos - target) * speed >= 0)
        {
            *pos = target;
            return TRUE;
        }
    }
    else if (*pos == target)
    {
        return TRUE;
    }
    return FALSE;
}

bool32 StepTowardAngle(s16 *pos, s16 target, s16 speed)
{
    if (speed != 0)
    {
        if ((s16)(*pos - target) > 0)
            speed = -speed;
        *pos += speed;
        if ((s16)(*pos - target) * speed >= 0)
        {
            *pos = target;
            return TRUE;
        }
    }
    else if (*pos == target)
    {
        return TRUE;
    }
    return FALSE;
}

u8 CountDownU8(u8 *timer)
{
    if (*timer != 0)
        (*timer)--;
    return *timer;
}

u16 CountDownU16(u16 *timer)
{
    if (*timer != 0)
        (*timer)--;
    return *timer;
}

// Add a sprite frame to gOamBuffer at gMain.unk450 (the OAM entries used so far this frame).
// With no angle and scale it draws every part at its offset; otherwise each part gets its own
// double-size affine OBJ and matrix (gMain.unk451 counts those), rotated and scaled about
// (x - ofsX, y - ofsY). The parts' own offsets are ignored then, and ofsX/ofsY flip sign for
// every part, so it only works right for one-part sprites or zero offsets (all callers so far).
void DrawSprite(const void *sprite, u8 frame, s16 x, s16 y, u8 angle, u8 mosaic, u16 scale, s16 ofsX, s16 ofsY,
                u8 tileOfs, u8 palette)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];
    const struct SpriteFrame *frames = sprite;
    const struct SpritePart *part;
    s32 shake;
    s16 cos, sin;
    struct OamData *mtx;

    frames += frame;
    part = frames->parts;
    shake = (gGameState.shake >> 1) & 1;

    if (angle == 0 && scale == 0)
    {
        while (part->end != 0xFFFF)
        {
            oam->y = y + part->y + shake;
            oam->objMode = part->objMode;
            oam->mosaic = mosaic;
            oam->shape = part->shape;
            oam->x = x + part->x;
            oam->hFlip = part->hFlip;
            oam->vFlip = part->vFlip;
            oam->size = part->size;
            oam->tileNum = part->tileNum + tileOfs;
            oam->priority = part->priority;
            if (palette)
                oam->paletteNum = palette - 1;
            else
                oam->paletteNum = part->paletteNum;
            oam++;
            gMain.unk450++;
            part++;
        }
    }
    else
    {
        s16 negSin;

        cos = (gSinTable + 64)[angle];
        sin = gSinTable[angle];
        negSin = -sin;
        mtx = (struct OamData *)gOamBuffer + gMain.unk451 * 4;
        if (scale == 0)
            scale = 0x100;
        while (part->end != 0xFFFF)
        {
            s32 px, py;
            s32 rx32;
            s32 rx;
            s16 ry;
            u16 half;
            s16 w;

            half = 4 << part->size;
            ofsX = -ofsX;
            ofsY = -ofsY;
            px = (ofsX << 8) / scale;
            rx32 = cos * px;
            py = (ofsY << 8) / scale;
            rx = (rx32 + sin * py) << 8;  // (s16)(... >> 8), finished below
            ry = (cos * py - sin * px) >> 8;
            rx >>= 16;
            w = half * 2;
            oam->x = x - (w + ofsX - rx);
            oam->y = y + shake - (w + ofsY - ry);
            oam->objMode = part->objMode;
            oam->mosaic = mosaic;
            oam->shape = part->shape;
            oam->size = part->size;
            oam->tileNum = part->tileNum + tileOfs;
            oam->priority = part->priority;
            if (palette)
                oam->paletteNum = palette - 1;
            else
                oam->paletteNum = part->paletteNum;
            oam->affineMode = 3;
            oam->matrixNum = gMain.unk451;
            oam->hFlip = gMain.unk451 >> 3;
            oam->vFlip = gMain.unk451 >> 4;
            mtx[0].affineParam = scale * cos >> 8;
            mtx[1].affineParam = negSin * scale >> 8;
            mtx[2].affineParam = scale * sin >> 8;
            mtx[3].affineParam = scale * cos >> 8;
            oam++;
            mtx += 4;
            gMain.unk450++;
            gMain.unk451++;
            part++;
        }
    }
}

// DrawSprite without rotation, tile offset or the parts' own priority.
void DrawSpriteAt(const void *sprite, u8 frame, s16 x, s16 y, u8 mosaic, u8 palette, u8 priority)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];
    const struct SpriteFrame *frames = sprite;
    const struct SpritePart *part;
    s32 shake;

    frames += frame;
    part = frames->parts;
    shake = (gGameState.shake >> 1) & 1;

    while (part->end != 0xFFFF)
    {
        oam->y = y + part->y + shake;
        oam->objMode = part->objMode;
        oam->mosaic = mosaic;
        oam->shape = part->shape;
        oam->x = x + part->x;
        oam->hFlip = part->hFlip;
        oam->vFlip = part->vFlip;
        oam->size = part->size;
        oam->tileNum = part->tileNum;
        oam->priority = priority;
        if (palette)
            oam->paletteNum = palette - 1;
        else
            oam->paletteNum = part->paletteNum;
        oam++;
        gMain.unk450++;
        part++;
    }
}

// DrawSpriteAt for a bare part list, keeping the parts' palettes.
// Near-miss: x and the ~0x10 mosaic mask swap registers (sb/r8); everything else matches.
/*
void DrawSpriteParts(const void *parts, s16 x, s16 y, u8 mosaic, u8 priority)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];
    const struct SpritePart *part = parts;
    s32 shake = (gGameState.shake >> 1) & 1;

    while (part->end != 0xFFFF)
    {
        oam->y = y + part->y + shake;
        oam->objMode = part->objMode;
        oam->mosaic = mosaic;
        oam->shape = part->shape;
        oam->x = x + part->x;
        oam->hFlip = part->hFlip;
        oam->vFlip = part->vFlip;
        oam->size = part->size;
        oam->tileNum = part->tileNum;
        oam->priority = priority;
        oam->paletteNum = part->paletteNum;
        oam++;
        gMain.unk450++;
        part++;
    }
}
*/
INCLUDE_ASM(DrawSpriteParts);

void CopyMapColumn(s16 x, s16 y, const u16 *src)
{
    u16 *dest = (u16 *)(((y & 0xF8) << 3) + ((x & 0xF8) >> 2) + 0x06005800);
    s32 total = 16;
    s32 n = 32 - ((y >> 3) & 0x1F);
    bool32 wrapped = FALSE;
    const u16 *end;

    if (n > total)
        n = total;
    end = src + gGameState.unk00C->width * n;
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
        src += gGameState.unk00C->width;
    } while (src < end);
    total = n;
    if (total != 0)
    {
        if (wrapped)
            dest += 0x400;
        else
            dest -= 0x400;
        end = src + gGameState.unk00C->width * total;
        do
        {
            *dest = *src;
            dest += 0x20;
            if (dest > (u16 *)0x06005FFF)
                dest -= 0x800;
            src += gGameState.unk00C->width;
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
    s16 dx = (state->scrollX & ~7) - (ox & ~7);
    s16 dy = (state->scrollY & ~7) - (oy & ~7);
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
        src = &gGameState.unk00C->map[(state->scrollY >> 3) * gGameState.unk00C->width + (ox >> 3)];
        do
        {
            ox += colPix;
            src += colStep;
            CopyMapColumn(ox, state->scrollY, src);
            dx -= colPix;
        } while (dx != 0);
    }
    if (dy != 0)
    {
        if (dy < 0)
        {
            rowStep = -gGameState.unk00C->width;
            rowPix = -8;
        }
        else
        {
            oy += 120;
            rowStep = gGameState.unk00C->width;
            rowPix = 8;
        }
        src = &gGameState.unk00C->map[(oy >> 3) * gGameState.unk00C->width + (state->scrollX >> 3)];
        do
        {
            oy += rowPix;
            src += rowStep;
            CopyMapRow(oy, state->scrollX, src);
            dy -= rowPix;
        } while (dy != 0);
    }
end:
    gMain.bgOfs[3][0] = state->scrollX - 8;
    gMain.bgOfs[3][1] = state->scrollY - 8;
}

INCLUDE_ASM(Func_02019510);

// Highlight the rooms on the four BG map blocks at 0x0600C000 (one room each, gGameState.unk0F0):
// clear the palette bits of the block showing marker 1's room, then set palette bit 1 (|= 0x2000)
// on a 2x2 or 4x4 tile square around every gSlot14Data entry in each block's room. Room 0x19's
// entries only show once flags.unkA_4 is set.
// Near-miss: the original keeps (y >> 8) * 7 narrowed to u16 before adding it to room, and
// allocates registers differently from there on (the unrolled loop's 0x1FFF lands in r2, not r4).
/*
#define CLEAR_PAL(p) (*(p) &= 0x1FFF, (p)++)
#define CLEAR_PAL_4(p) CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p); CLEAR_PAL(p)
#define CLEAR_PAL_32(p) CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); \
    CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p); CLEAR_PAL_4(p)

void HighlightMapRooms(void)
{
    u8 m = FindMarker(1);
    u16 room;
    s32 i, j, k;
    u16 *map;

    room = gGameState.markers[m].x >> 8;
    room += (u16)((gGameState.markers[m].y >> 8) * 7);
    for (i = 0; i <= 3; i++)
    {
        if (gGameState.unk0F0[i] == room)
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
        if (gGameState.unk0F0[i] == 0x19 && !gGameState.flags.unkA_4)
            continue;
        for (j = 0; j <= 0x30; j++)
        {
            u8 *e = &gSlot14Data[j * 4];

            if (gGameState.unk0F0[i] == e[3])
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
    dx = gGameState.markers[i].x - (s16)(gGameState.scrollX + 0x44);
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
    dy = gGameState.markers[i].y - (s16)(gGameState.scrollY + 0x44);
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
    gGameState.scrollX += dx;
    gGameState.scrollY += dy;
    gMain.bgOfs[2][0] = gGameState.scrollX;
    gMain.bgOfs[2][1] = gGameState.scrollY;
    if (((s16)gMain.bgOfs[2][0] + 8) & 0x8000)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][0] = -8;
    }
    if ((s16)gMain.bgOfs[2][0] > 0x680)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][0] = 0x680;
    }
    if (((s16)gMain.bgOfs[2][1] + 8) & 0x8000)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][1] = -8;
    }
    if ((s16)gMain.bgOfs[2][1] > 0x680)
    {
        gGameState.scrolling = FALSE;
        gMain.bgOfs[2][1] = 0x680;
    }
    gMain.bgOfs[3][0] = gMain.bgOfs[2][0];
    gMain.bgOfs[3][1] = gMain.bgOfs[2][1];
}

void FollowMarkerStreamedMap(void)
{
    s16 maxX = gGameState.unk00C->unk00 - 120;
    s16 maxY = gGameState.unk00C->unk02 - 120;
    s16 dx;
    s16 dy = 0;
    u8 i = FindMarker(3);

    if (i == 0xFF)
        i = FindMarker(1);
    gGameState.scrolling = FALSE;
    dx = gGameState.markers[i].x - (s16)(gGameState.scrollX + 0x3C);
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
    dy = gGameState.markers[i].y - (s16)(gGameState.scrollY + 0x3C);
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
    gGameState.scrollX += dx;
    if (gGameState.scrollX < 0)
    {
        gGameState.scrolling = FALSE;
        gGameState.scrollX = 0;
    }
    if (gGameState.scrollX > maxX)
    {
        gGameState.scrolling = FALSE;
        gGameState.scrollX = maxX;
    }
    gGameState.scrollY += dy;
    if (gGameState.scrollY < 0)
    {
        gGameState.scrolling = FALSE;
        gGameState.scrollY = 0;
    }
    if (gGameState.scrollY > maxY)
    {
        gGameState.scrolling = FALSE;
        gGameState.scrollY = maxY;
    }
}

INCLUDE_ASM(Func_0201A224);

INCLUDE_ASM(Func_0201A540);

INCLUDE_ASM(Func_0201A570);

INCLUDE_ASM(Func_0201A69C);

INCLUDE_ASM(Func_0201A6D4);

INCLUDE_ASM(FindMarker);

INCLUDE_ASM(Func_0201A8AC);

INCLUDE_ASM(Func_0201A944);

INCLUDE_ASM(Mode1VBlank);

INCLUDE_ASM(Func_0201AC94);

INCLUDE_ASM(Func_0201ACD0);

INCLUDE_ASM(Func_0201ACF8);

INCLUDE_ASM(Func_0201AD24);

INCLUDE_ASM(Func_0201AD4C);

INCLUDE_ASM(Func_0201AD90);

INCLUDE_ASM(Func_0201AF48);

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

INCLUDE_ASM(Func_0201C660);

INCLUDE_ASM(Func_0201C704);

#ifndef VERSION_GZLJ01
INCLUDE_ASM(UsFunc_02023390);
#endif

INCLUDE_ASM(Func_0201C86C);

INCLUDE_ASM(Func_0201C8D8);

INCLUDE_ASM(Func_0201C948);

INCLUDE_ASM(Func_0201C988);

INCLUDE_ASM(Func_0201C9E4);

INCLUDE_ASM(Func_0201CA24);

INCLUDE_ASM(Func_0201CA48);

INCLUDE_ASM(Func_0201CA6C);

INCLUDE_ASM(Func_0201CA70);

INCLUDE_ASM(Func_0201CA74);

INCLUDE_ASM(Func_0201CA78);

INCLUDE_ASM(Func_0201CA7C);

INCLUDE_ASM(Func_0201CA80);

INCLUDE_ASM(Func_0201CA84);

INCLUDE_ASM(Func_0201CA98);

INCLUDE_ASM(Func_0201CAAC);

INCLUDE_ASM(Func_0201CAC0);

INCLUDE_ASM(Func_0201CAD4);

INCLUDE_ASM(Func_0201CAE8);

INCLUDE_ASM(Func_0201CB10);

INCLUDE_ASM(Func_0201CFE8);

INCLUDE_ASM(Func_0201D8AC);

INCLUDE_ASM(Func_0201D8D8);

INCLUDE_ASM(GamePlaySound);

INCLUDE_ASM(Func_0201DA24);

INCLUDE_ASM(Func_0201DA50);

#ifndef VERSION_GZLJ01
INCLUDE_ASM(DrawString);
#endif

INCLUDE_ASM(Func_0201DA78);

INCLUDE_ASM(Func_0201DB50);

INCLUDE_ASM(DrawItemIcon);

INCLUDE_ASM(DrawItemCount);

INCLUDE_ASM(Func_0201DCEC);

INCLUDE_ASM(Func_0201DD20);

INCLUDE_ASM(Func_0201DDA0);

INCLUDE_ASM(Func_0201DEAC);

INCLUDE_ASM(Func_0201E190);

INCLUDE_ASM(Func_0201E2FC);

INCLUDE_ASM(SendItemUse);

INCLUDE_ASM(Func_0201E384);

INCLUDE_ASM(Func_0201E584);

INCLUDE_ASM(Func_0201E5E0);

INCLUDE_ASM(Func_0201EBAC);

INCLUDE_ASM(Func_0201EBD4);

INCLUDE_ASM(Func_0201EC10);

INCLUDE_ASM(Func_0201ECC4);

INCLUDE_ASM(Func_0201ED08);

INCLUDE_ASM(Func_0201EDC4);

INCLUDE_ASM(Func_0201EDF8);

INCLUDE_ASM(Func_0201EF98);

INCLUDE_ASM(Func_0201F068);

INCLUDE_ASM(Func_0201F240);

INCLUDE_ASM(Func_0201F274);

INCLUDE_ASM(Func_0201F2A8);

INCLUDE_ASM(Func_0201F2C4);

INCLUDE_ASM(Func_0201F308);

INCLUDE_ASM(Func_0201F420);

INCLUDE_ASM(Func_0201F460);

INCLUDE_ASM(Func_0201F878);

INCLUDE_ASM(Func_0201F8C4);

INCLUDE_ASM(Func_0201FA2C);

INCLUDE_ASM(Func_0201FCA0);

INCLUDE_ASM(Func_0201FD04);

INCLUDE_ASM(Func_0201FEEC);

INCLUDE_ASM(Func_0201FF14);

INCLUDE_ASM(Func_02020150);

INCLUDE_ASM(Func_02020190);

INCLUDE_ASM(Func_02020298);

INCLUDE_ASM(Func_020202E8);

INCLUDE_ASM(Func_020203EC);

INCLUDE_ASM(MainMode2);

INCLUDE_ASM(Func_020204D0);

INCLUDE_ASM(Func_02020504);

INCLUDE_ASM(Func_02020538);

INCLUDE_ASM(ObjReset);

INCLUDE_ASM(Func_02020598);

INCLUDE_ASM(Func_020205D4);

INCLUDE_ASM(Func_020205F8);

INCLUDE_ASM(ObjResetActive);

INCLUDE_ASM(Func_02020668);

INCLUDE_ASM(Func_020207D4);

INCLUDE_ASM(Func_0202081C);

INCLUDE_ASM(BuildWaveScrollTable);

INCLUDE_ASM(Func_020208FC);

INCLUDE_ASM(Func_020209A0);

INCLUDE_ASM(Func_020209E0);

INCLUDE_ASM(Func_02020BEC);

INCLUDE_ASM(Func_02020C10);
