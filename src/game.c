#include "global.h"
#include "main.h"
#include "game.h"
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

INCLUDE_ASM(Func_0201B3A4);

// Play the sound for what the cursor is on: unk135 if set, else the selected marker's.
// Near-miss: the original computes the (s16) ids in the shifted domain, ((v << 16) +
// (c << 16)) >> 16, which agbcc only emits for an explicit shift pair (the ID macro below);
// written that way the code shape matches but the registers of the entry block differ
// (the original keeps gGameState in r4 and loads it first).
/*
#define ID(v, c) ((s32)(((v) << 16) + ((c) << 16)) >> 16)

void Func_0201B7D4(void)
{
    u8 idx = gGameState.unk119;

    if (gGameState.unk135 != 0)
    {
        if (gGameState.unk135 == 0x21 && gGameState.flags.unk5_6)
            GamePlaySound(0x13E, 1);
        else
            GamePlaySound(gGameState.unk135 + 0x14D, 1);
    }
    else if (gGameState.markers[idx].type == 0x13)
        GamePlaySound(gSounds_02024278[gGameState.markers[idx].unk6 - 1], 1);
    else if (gGameState.markers[idx].type == 0x14)
        GamePlaySound(0x5D, 1);
    else if (gGameState.markers[idx].unk7 <= 0x23)
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x63), 1);
    else if (gGameState.markers[idx].unk7 <= 0x27)
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x7C), 1);
    else if (gGameState.markers[idx].unk7 <= 0x2C)
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x82), 1);
    else if (gGameState.markers[idx].unk7 <= 0x38)
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x87), 1);
    else if (gGameState.markers[idx].unk7 <= 0x40)
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x8F), 1);
    else
        GamePlaySound(ID(gGameState.markers[idx].unk7, 0x6F), 1);
}
*/
INCLUDE_ASM(Func_0201B7D4);

// Act on the current selection (unk119: a marker, or unk135): 0: send the switch record,
// 1: a random reaction sound, 2: play the selection's sound via Func_0201B7D4.
// Near-miss: same code shape, but the original keeps result in r1 (reusing its 1 as the
// mask in the type 16 test) where agbcc puts it in r4, and orders the dispatch compares
// as == 2, == 0.
/*
void Func_0201B8C8(void)
{
    u8 result = 0;

    if (gGameState.unk119 != 0)
    {
        if (gGameState.markers[gGameState.unk119].type == 12)
        {
            result = 1;
            if (gGameState.unk11C)
                result = 2;
        }
        else if (gGameState.markers[gGameState.unk119].type == 13)
        {
            result = 1;
            if (gGameState.unk11D)
                result = 2;
        }
        else if (gGameState.markers[gGameState.unk119].type == 5)
        {
            if (gGameState.unk010 & (1 << gGameState.markers[gGameState.unk119].unk6))
                return;
            result = 1;
        }
        else if (gGameState.markers[gGameState.unk119].type == 16)
        {
            result = 1;
            if ((gGameState.unk026 >> gGameState.markers[gGameState.unk119].unk6) & 1)
                result = 2;
        }
        else
        {
            result = 2;
            if (gGameState.markers[gGameState.unk119].type == 7)
                result = 1;
        }
    }
    else if (gGameState.unk135)
    {
        result = 2;
    }
    switch (result)
    {
    case 2:
        Func_0201B7D4();
        SoundPlay(2, 0);
        break;
    case 0:
        if (gGameState.switchType == 15)
        {
            gGameState.unk1F8 = gGameState.switchSound;
            gGameState.switchSound = 0x302;
        }
        if (gGameState.switchType != 0 && gGameState.switchType != 0xFF)
        {
            Mode1SendSwitch(3);
            SoundPlay(0x21 - SOUND_ID_SHIFT, 0);
        }
        break;
    default:
        if ((gMain.random & 0xFF) > 0xCB)
            GamePlaySound(0x17, 1);
        else if ((gMain.random & 0xFF) > 0x65)
            GamePlaySound(0x16, 1);
        else
            GamePlaySound(0x15, 1);
        gGameState.unk156 = gMain.random % 60 + 30;
        if (gGameState.markers[gGameState.unk119].type != 7)
            gGameState.unk11A = 2;
        break;
    }
}
*/
INCLUDE_ASM(Func_0201B8C8);

void Func_0201BA2C(s8 row)
{
    u16 *map = (u16 *)0x06006120 + row * 32;
    u16 tile = row * 12 + 0xF370;
    s32 i;

    map[0] = map[0x20] = 0x9023;
    map++;
    for (i = 0; i < 12; i++)
    {
        map[0x20] = tile + 12;
        *map = tile++;
        map++;
    }
    map[0] = map[0x20] = 0xD024;
}

void Func_0201BA9C(s8 row)
{
    u16 *src = (u16 *)0x06007800;
    s32 off = row * 32;

    src += off;
    CpuSet(src, (u16 *)0x060061A0 + off, 14);
    src += 0x20;
    CpuSet(src, (u16 *)0x060061E0 + off, 14);
}

void Func_0201BAD8(void)
{
    u32 *p = (u32 *)0x0600F100;
    u32 fill = 0x11111111;
    s32 i;

    for (i = 0x3C0; i != 0; i--)
        *p++ = fill;
}

// Blit a glyph's 4bpp rows (src, one word per 8-pixel row) into the text canvas at dst,
// x pixels into the tile: n rows into this tile row, the rest (rows - n) into the next one
// (0x180 bytes on, 12 tiles per row). Pixels that spill past the tile go to the next tile.
// Near-miss: same code shape, but the original keeps src in its stack slot and mask2 in r9
// (and dst's incremented copy in r0 across the inner loop); agbcc gives src a register here.
/*
void Func_0201BAF8(u32 x, u32 y, u8 color, u8 rows, const u32 *src, u32 *dst)
{
    s32 shift, n, m, i;
    u32 mask, mask2, w, *prev;

    x &= 7;
    shift = 8 - x;
    shift &= 7;
    y &= 7;
    n = 8 - y;
    m = rows - n;
    mask = gTextColorMasks[color];
    if (gGameState.unk157 == 8)
        mask2 = -1;
    else
        mask2 = 0x11FFFFFF;
    for (; n > 0; n--)
    {
        prev = dst;
        w = *src++ & mask & mask2;
        if (shift)
        {
            *dst++ |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
            prev[8] = w;
        }
        else
        {
            *dst++ = w;
        }
    }
    dst += 0x58;
    for (; m > 0; m--)
    {
        prev = dst;
        w = *src++ & mask;
        if (shift)
        {
            *dst++ |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
            prev[8] = w;
        }
        else
        {
            *dst++ = w;
        }
    }
}
*/
INCLUDE_ASM(Func_0201BAF8);

// Scale a glyph (4 bytes = 8 pixels a row, count bytes) up 2x into TEXT_GLYPH_BUF: the left
// half of each row goes to the first 8x8 4bpp tile, the right half to the second. The US
// glyphs are 6 pixels wide: each tile gets three (doubled) and a column of colour 1.
void Func_0201BC0C(s32 count, const u8 *src)
{
    s32 i;
    u16 *dst;
    u8 b;
#ifndef VERSION_GZLJ01
    u32 b1;
    u32 b2;
#endif

    for (i = 0; i < count; i += 4)
    {
        dst = (u16 *)((u8 *)TEXT_GLYPH_BUF + (i & 0x10) * 4 + (i & 0xC) * 2);
#ifdef VERSION_GZLJ01
        b = *src;
        dst[2] = dst[0] = EXPAND_NIBBLES(b);
        src++;
        b = *src;
        dst[3] = dst[1] = EXPAND_NIBBLES(b);
        src++;
        b = *src;
        dst[0x12] = dst[0x10] = EXPAND_NIBBLES(b);
        src++;
        b = *src;
        dst[0x13] = dst[0x11] = EXPAND_NIBBLES(b);
        src++;
#else
        b = *src;
        dst[2] = dst[0] = EXPAND_NIBBLES(b);
        src++;
        b1 = *src;
        dst[3] = dst[1] = ((b1 & 0x0F) + ((b1 & 0x0F) << 4)) + 0x1100;
        src++;
        b2 = *src;
        b = (b1 >> 4) | (b2 << 4);
        dst[0x12] = dst[0x10] = EXPAND_NIBBLES(b);
        dst[0x13] = dst[0x11] = (((b2 >> 4) & 0x0F) + (((b2 >> 4) & 0x0F) << 4)) + 0x1100;
        src += 2;
#endif
    }
}

INCLUDE_ASM(Func_0201BCC0);

void Func_0201BDE4(u8 x, u8 y, u8 color, u16 ch)
{
    u32 *glyph = (u32 *)0x0600A010 + ch * 8;
    u32 *dst;
    u32 row;
    u32 col;
#ifndef VERSION_GZLJ01
    u32 col2;
    s32 x2;
#endif

    y -= 4;
    row = (y >> 3) * 0x180 + (y & 7) * 4;
    col = (x >> 3) * 32 + 0x0600F100;
    dst = (u32 *)(row + col);
    if (gGameState.unk15A)
    {
        Func_0201BC0C(16, (u8 *)glyph);
        Func_0201BAF8(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
#ifdef VERSION_GZLJ01
        Func_0201BAF8(x + gGameState.unk157, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst + 8);
#else
        // The right half goes where x + unk157 falls, not simply the next tile.
        x2 = x + gGameState.unk157;
        col2 = (x2 >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col2);
        Func_0201BAF8(x2, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst);
#endif
    }
    else
    {
        Func_0201BAF8(x, y, color, 4, glyph, dst);
    }
}

INCLUDE_ASM(Func_0201BEA0);

void Func_0201C0E8(u8 playSound)
{
    u32 params[2];
    u32 mask;
    s32 r;

    Func_0201BAD8();
    gGameState.unk148 = 0;
    gGameState.unk149 = 8;
    if (playSound)
    {
        mask = 0xFFFF0000;
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
}

void Func_0201C164(void)
{
    u32 mosaic;
    const u8 *sprite;
    u8 y;
    u8 timer;

    mosaic = (gMain.bgcnt[0] >> 6) & 1;
    timer = --gGameState.unk152;
    sprite = gSprite_0202C130;
    if (timer == 0)
    {
        gGameState.unk153 ^= 1;
        gGameState.unk152 = (sprite + gGameState.unk153 * 8)[4];
    }
    y = gGameState.unk150 + gGameState.unk14F * 12;
    y += 0x34;
    DrawSprite(sprite, gGameState.unk153, 0x8C, y, 0, mosaic, 0, 0, 0, 0, 0);
}

INCLUDE_ASM(Func_0201C204);

void Func_0201C484(u8 id)
{
    u32 params[2];
    u32 mask;

    if ((u8)(id - 12) <= 3)
    {
        id = gMain.random % 3 + 12;
        mask = 0xFFFF0000;
        params[0] = 0;
        if (id == 12)
            params[1] = params[1] & mask;
        else if (id == 13)
            params[1] = (params[1] & mask) | 1;
        else
            params[1] = (params[1] & mask) | 2;
        SoundPlay(gSoundIds_0202430E[id], params);
    }
    else if ((u8)(id - 0x20) <= 2)
    {
        gGameState.unk140 = gSoundIds_0202430E[id];
        Func_02021284(gGameState.unk140, gGameState.unk140, 0);
        if (gGameState.unk144[1] == 0xF9)
        {
            gGameState.unk144++;
            gGameState.unk14D = *++gGameState.unk144;
        }
    }
    else if (id != 0x16)
    {
        SoundPlay(gSoundIds_0202430E[id], 0);
    }
}

void Func_0201C568(void)
{
    u8 i;
    const u8 *const *const *entry;

    gGameState.unk22A = 0;
    if (gGameState.unk226 == 2)
    {
        gGameState.unk206 = 0x300;
    }
    else if (gGameState.unk226 == 3)
    {
        gGameState.unk206 = 0x100;
    }
    else if (gGameState.unk226 == 11)
    {
        gGameState.unk206 = 0;
        gGameState.unk22C = 0;
        gGameState.unk208 = 0x100;
    }
    else
    {
        gGameState.unk206 = 0;
    }
    gGameState.unk228 = 0;
    gGameState.unk227 = 0;
    i = gGameState.unk226;
    entry = gTable_02024424 + i;
    gGameState.unk202 = (**entry)[4];
}

bool32 Func_0201C60C(void)
{
    u8 step;

    if (gGameState.unk148 < 0x60)
        return TRUE;
    gGameState.unk148 = 0;
    if (gGameState.unk15A)
        step = 24;
    else
        step = 12;
    gGameState.unk149 += step;
    step -= 12;
    if (gGameState.unk149 < 0x50 - step)
        return TRUE;
    return FALSE;
}

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
