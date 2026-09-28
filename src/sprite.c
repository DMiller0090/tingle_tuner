#include "global.h"
#include "gba/syscall.h"
#include "game.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

// Game code object 1 (GZLJ01 02018884-0201920C): the easing/step helpers and DrawSprite.
// Its .rodata is gSinTable and gSpriteTable. Nothing in the binary separates the helpers
// from the sprite code, so they share the object. See docs/layout.md.

const s16 gSinTable[320] =
{
    0, 6, 12, 18, 25, 31, 37, 43, 49, 56, 62, 68, 74, 80, 86, 92,
    97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
    181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234,
    236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255,
    256, 255, 255, 255, 254, 254, 253, 252, 251, 249, 248, 246, 244, 243, 241, 238,
    236, 234, 231, 228, 225, 222, 219, 216, 212, 209, 205, 201, 197, 193, 189, 185,
    181, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103,
    97, 92, 86, 80, 74, 68, 62, 56, 49, 43, 37, 31, 25, 18, 12, 6,
    0, -6, -12, -18, -25, -31, -37, -43, -49, -56, -62, -68, -74, -80, -86, -92,
    -97, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176,
    -181, -185, -189, -193, -197, -201, -205, -209, -212, -216, -219, -222, -225, -228, -231, -234,
    -236, -238, -241, -243, -244, -246, -248, -249, -251, -252, -253, -254, -254, -255, -255, -255,
    -256, -255, -255, -255, -254, -254, -253, -252, -251, -249, -248, -246, -244, -243, -241, -238,
    -236, -234, -231, -228, -225, -222, -219, -216, -212, -209, -205, -201, -197, -193, -189, -185,
    -181, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103,
    -97, -92, -86, -80, -74, -68, -62, -56, -49, -43, -37, -31, -25, -18, -12, -6,
    0, 6, 12, 18, 25, 31, 37, 43, 49, 56, 62, 68, 74, 80, 86, 92,
    97, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176,
    181, 185, 189, 193, 197, 201, 205, 209, 212, 216, 219, 222, 225, 228, 231, 234,
    236, 238, 241, 243, 244, 246, 248, 249, 251, 252, 253, 254, 254, 255, 255, 255
};

const u8 *const gSpriteTable[22] =
{
    0x0,
    gSprite_0202BD88,
    gSprite_0202BDA8,
    gSprite_0202BD98,
    gSprite_0202BE30,
    gSprite_0202C148,
    gSprite_0202C1A0,
    gSprite_0202C188,
    gSprite_0202C140,
    gSprite_0202C190,
    gSprite_0202C128,
    gSprite_0202BE38,
    gSprite_0202C1A8,
    gSprite_0202C1B0,
    gSprite_0202C1F0,
    gSprite_0202C1E8,
    gSprite_0202C3E8,
    gSprite_0202BDE8,
    gSprite_0202BE00,
    gSprite_0202BDD8,
    gSprite_0202C4C0,
    gSprite_0202C1E0
};

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
// mosaic is a u8 here; callers see a u32 (the prototype in game.h).
void DrawSprite_(const void *sprite, u8 frame, s16 x, s16 y, u8 angle, u8 mosaic, u16 scale, s16 ofsX,
                 s16 ofsY, u8 tileOfs, u8 palette) asm("DrawSprite");
void DrawSprite_(const void *sprite, u8 frame, s16 x, s16 y, u8 angle, u8 mosaic, u16 scale, s16 ofsX,
                 s16 ofsY, u8 tileOfs, u8 palette)
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
void DrawSpriteParts(const void *parts, s16 x, s16 y, u8 mosaic, u8 priority)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];
    const struct SpritePart *part = parts;
    // FAKEMATCH: the no-op % 2 lengthens x's live range so global-alloc ranks it after ~0x10
    s32 shake = ((gGameState.shake >> 1) % 2) & 1;

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
