#include "global.h"
#include "gba/syscall.h"
#include "game.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

// Game code object 3 (GZLJ01 0201BA2C-0201DA78): the text renderer, the choice menu and the
// messages. The US .rodata puts the choice menu's tables in the object with MsgStep's
// pointer table; the renderer is only called from here. See docs/layout.md.

const u32 gTextColorMasks[7] =
{
    0x33333333, 0x55555555, 0x77777777, 0x99999999,
    0xBBBBBBBB, 0xDDDDDDDD, 0xFFFFFFFF
};

const u16 gTwoChoiceIds[5] =
{
    0x3E, 0x41, 0x43, 0x45, 0x1A6
};

const u16 gTwoChoiceMsgIds[10] =
{
    0x41, 0x3D,
    0x47, 0x3D,
    0x47, 0x3D,
    0x47, 0x3D,
    0xFFFF, 0x1A7
};

const u16 gThreeChoiceIds[7] =
{
    0x3F, 0x40, 0xD2, 0xD4, 0xD8, 0xDB, 0xDD
};

const u16 gThreeChoiceMsgIds[21] =
{
    0x41, 0x43, 0x3D,
    0x41, 0x45, 0x3D,
    0xD3, 0xD4, 0xDA,
    0xD5, 0xD6, 0xD7,
    0xDA, 0xD4, 0xD9,
    0xDC, 0xDD, 0xDA,
    0xDE, 0xDF, 0xE0
};

// Sound per message sound code (Func_0201C484); codes 0x20-0x22 are SoundPlayLoop IDs.
#define SE(id) ((id) < 7 ? (id) : (id) - SOUND_ID_SHIFT)
const u16 gSoundIds_0202430E[] =
{
    SE(0x1F), SE(0x1A), SE(0x1B), SE(0x2B), SE(0x2A), SE(0x29), SE(0x1C), SE(0x1E),
    SE(0x1D), SE(0x23), 0, 0, SE(0x13), SE(0x13), SE(0x13), SE(0x13),
    SE(0x12), SE(0x11), SE(0x10), SE(0x08), SE(0x09), 3, 3, 1,
    4, SE(0x24), 5,
#ifdef VERSION_GZLJ01
    6,  // voice clip 6, not in the US sound bank
#endif
    SE(0x21), SE(0x22), SE(0x20), SE(0x26),
    2, 1, 5
};
#undef SE

const u16 gMsgTingleAnims[17] =
{
    2, 10, 12, 13, 0, 4, 7, 8, 9,
    11, 14, 15, 16, 17, 18, 19, 20
};

#ifdef VERSION_GZLJ01
const u16 gGlyphRunStart[8] =
{
    0x86, 0xD1, 0xD2, 0xCE, 0xCB, 0xD3, 0xD5, 0xD7
};
#else
const u16 gGlyphRunStart[8] =
{
    0x4F, 0x71, 0x72, 0x6E, 0x6B, 0x73, 0x75, 0x77
};
#endif

const u8 gGlyphRunLength[8] =
{
    1, 1, 1, 3, 3, 2, 2, 1
};

#ifdef VERSION_GZLJ01
const u8 gGlyphTable_0202438E[6] =
{
    0x50, 0x51, 0x52, 0x1C, 0x53, 0x39
};

const u8 gDayNameChars[7 * 2] =
{
    0x15, 0x10,
    0xC0, 0x11,
    0x5, 0x5,
    0xC, 0x1,
    0x22, 0x7,
    0x6, 0x2D,
    0xCB, 0xCB
};

#else
// Accented letters (DrawChar): per character code, the base glyph and how far down the
// accent (a small glyph, one per table) goes.
const u8 gAccentGlyphs_0202AFF4[2][2] =
{
    {0x13, 0}, {0x2D, 2}
};

const u8 gAccentGlyphs_0202AFF8[2][2] =
{
    {0x01, 0}, {0x1B, 2}
};

const u8 gAccentGlyphs_0202AFFC[12][2] =
{
    {0x01, 0}, {0x05, 0}, {0x26, 1}, {0x0F, 0}, {0x15, 0}, {0x19, 0},
    {0x1B, 2}, {0x1F, 2}, {0x8C, 2}, {0x29, 2}, {0x2F, 2}, {0x33, 2}
};

const u8 gAccentGlyphs_0202B014[6][2] =
{
    {0x01, 0}, {0x0E, 0}, {0x0F, 0}, {0x1B, 2}, {0x28, 2}, {0x29, 2}
};

const u8 gAccentGlyphs_0202B020[10][2] =
{
    {0x01, 0}, {0x05, 0}, {0x26, 1}, {0x0F, 0}, {0x15, 0},
    {0x1B, 2}, {0x1F, 2}, {0x8C, 2}, {0x29, 2}, {0x2F, 2}
};

const u8 gAccentGlyphs_0202B034[12][2] =
{
    {0x01, 0}, {0x05, 0}, {0x26, 1}, {0x0F, 0}, {0x15, 0}, {0x19, 0},
    {0x1B, 2}, {0x1F, 2}, {0x8C, 2}, {0x29, 2}, {0x2F, 2}, {0x33, 2}
};

const u8 gAccentGlyphs_0202B04C[10][2] =
{
    {0x01, 0}, {0x05, 0}, {0x26, 1}, {0x0F, 0}, {0x15, 0},
    {0x1B, 2}, {0x1F, 2}, {0x8C, 2}, {0x29, 2}, {0x2F, 2}
};

#endif

void (*const gMsgCmdFuncs[19])(void) =
{
    MsgCmd00,
    MsgCmd01,
    DrawWeekday,
    DrawHour,
    DrawMinute,
    MsgCmd05,
    MsgCmd06,
    MsgCmd07,
    MsgCmd08,
    MsgCmd09,
    MsgCmd0A,
    MsgCmd0B,
    MsgCmd0C,
    MsgCmd0D,
    MsgCmd0E,
    MsgCmd0F,
    MsgCmd10,
    MsgCmd11,
    MsgCmd12
};

const u8 *const gTingleFrames_020243F0[1] =
{
    gSprite_0202C740
};

const u8 *const gTingleFrames_020243F4[2] =
{
    gSprite_0202C650, gSprite_0202C740
};

const u8 *const gTingleFrames_020243FC[1] =
{
    gSprite_0202C740
};

const u8 *const gTingleFrames_02024400[2] =
{
    gSprite_0202C650, gSprite_0202C740
};

const u8 *const gTingleFrames_02024408[1] =
{
    gSprite_0202C650
};

const u8 *const gTingleFrames_0202440C[1] =
{
    gSprite_0202C788
};

const u8 *const gTingleFrames_02024410[1] =
{
    gSprite_0202C7C8
};

const u8 *const gTingleFrames_02024414[1] =
{
    gSprite_0202C848
};

const u8 *const gTingleFrames_02024418[2] =
{
    gSprite_0202C850, gSprite_0202C650
};

const u8 *const gTingleFrames_02024420[1] =
{
    gSprite_0202CAE0
};

const u8 *const *const gTingleAnims[15] =
{
    gTingleFrames_020243F0,
    gTingleFrames_020243F0,
    gTingleFrames_020243F4,
    gTingleFrames_020243FC,
    gTingleFrames_02024400,
    gTingleFrames_02024408,
    gTingleFrames_020243FC,
    gTingleFrames_0202440C,
    gTingleFrames_02024410,
    gTingleFrames_02024414,
    gTingleFrames_02024418,
    gTingleFrames_02024420,
    gTingleFrames_020243F0,
    gTingleFrames_020243F0,
    gTingleFrames_020243F0
};

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

void TextClearCanvas(void)
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
// The u8 x and y (the header's types) matter: their zero-extensions, folded into `& 7` by
// combine, still count in src's live length, which leaves src in its stack slot.
#ifdef VERSION_GZLJ01
void TextBlitGlyph(u8 x, u8 y, u8 color, u8 rows, const u32 *src, u32 *dst)
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
    for (; n > 0; dst++, n--)
    {
        prev = dst;
        w = *src++ & mask & mask2;
        if (shift)
        {
            *dst |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
            prev[8] = w;
        }
        else
        {
            *dst = w;
        }
    }
    dst += 0x58;
    for (; m > 0; dst++, m--)
    {
        prev = dst;
        w = *src++ & mask;
        if (shift)
        {
            *dst |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
            prev[8] = w;
        }
        else
        {
            *dst = w;
        }
    }
}
#else
INCLUDE_ASM(TextBlitGlyph);
#endif

// Scale a glyph (4 bytes = 8 pixels a row, count bytes) up 2x into TEXT_GLYPH_BUF: the left
// half of each row goes to the first 8x8 4bpp tile, the right half to the second. The US
// glyphs are 6 pixels wide: each tile gets three (doubled) and a column of colour 1.
void TextScaleGlyph(s32 count, const u8 *src)
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

// Draw an 8x8 glyph (from 0x0600A000) at pixel (x, y) of the text canvas; with unk15A set,
// scaled 2x into four tiles. TextDrawSmallGlyph is the same for the 4-row glyphs at 0x0600A010.
#ifdef VERSION_GZLJ01
void TextDrawGlyph(u8 x, u8 y, u8 color, u16 ch)
{
    u32 *glyph = (u32 *)0x0600A000 + ch * 8;
    u32 *dst;
    u32 row;
    u32 col;
    u8 y2;

    if (gGameState.unk15A)
    {
        TextScaleGlyph(32, (u8 *)glyph);
        y += 4;
        row = (y >> 3) * 0x180 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
        TextBlitGlyph(x + gGameState.unk157, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst + 8);
        y2 = y + 8;
        TextBlitGlyph(x, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 16, dst + 0x60);
        TextBlitGlyph(x + gGameState.unk157, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 24, dst + 0x68);
    }
    else
    {
        row = (y >> 3) * 0x180 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, glyph, dst);
    }
}
#else
// US near-miss: the right halves go where x + unk157 falls (as in TextDrawSmallGlyph), and the
// link-lost screen (mode 2) has a 30-tile-wide canvas. Same code shape, but the original
// spills gGameState's address to the stack and allocates the other registers differently.
/*
void TextDrawGlyph(u8 x, u8 y, u8 color, u16 ch)
{
    u32 *glyph = (u32 *)0x0600A000 + ch * 8;
    u32 *dst;
    u32 row;
    u32 col;
    u8 y2;
    u32 col2;
    s32 x2;

    if (gGameState.unk15A)
    {
        TextScaleGlyph(32, (u8 *)glyph);
        y += 4;
        row = (y >> 3) * 0x180 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
        y2 = y + 8;
        TextBlitGlyph(x, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 16, dst + 0x60);
        x2 = x + gGameState.unk157;
        col2 = (x2 >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col2);
        TextBlitGlyph(x2, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst);
        TextBlitGlyph(x + gGameState.unk157, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 24, dst + 0x60);
    }
    else
    {
        if (gMain.mode != 2)
            row = (y >> 3) * 0x180 + (y & 7) * 4;
        else
            row = (y >> 3) * 0x3C0 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, glyph, dst);
    }
}
*/
INCLUDE_ASM(TextDrawGlyph);
#endif

void TextDrawSmallGlyph(u8 x, u8 y, u8 color, u16 ch)
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
        TextScaleGlyph(16, (u8 *)glyph);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
#ifdef VERSION_GZLJ01
        TextBlitGlyph(x + gGameState.unk157, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst + 8);
#else
        // The right half goes where x + unk157 falls, not simply the next tile.
        x2 = x + gGameState.unk157;
        col2 = (x2 >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col2);
        TextBlitGlyph(x2, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst);
#endif
    }
    else
    {
        TextBlitGlyph(x, y, color, 4, glyph, dst);
    }
}

// Add the text box frame's sprites to gOamBuffer: the corners (tiles 0x24, 0x26) and 12
// edge pieces (0x25), each once at y = row * 8 + 48 (a line higher while the screen shakes)
// and once flipped at y = 40, all in palette 8.
// Near-miss: identical but for one instruction, the edge loop's x (0x88) is set up after the
// hoisted constants 0x25 and ~0xC instead of before them.
/*
#define TEXT_BOX_PIECE(p, Y, X, TILE, VFLIP)     p->y = Y;                                    p->mosaic = 0;                               p->x = X;                                    VFLIP                                        p->size = 0;                                 p->tileNum = TILE;                           p->priority = 0;                             p->paletteNum = 8;                           p++;                                         gMain.unk450++;

void Func_0201BEA0(s8 row)
{
    struct OamData16 *p = (struct OamData16 *)&gOamBuffer[gMain.unk450];
    s32 y;
    s32 shake;
    s32 t;
    s32 i;

    t = row * 8;
    shake = (gGameState.shake >> 1) & 1;
    y = t + 48;
    TEXT_BOX_PIECE(p, y - shake, 0x80, 0x24, )
    TEXT_BOX_PIECE(p, 40, 0x80, 0x24, p->vFlip = 1;)
    TEXT_BOX_PIECE(p, y - shake, 0xE8, 0x26, )
    TEXT_BOX_PIECE(p, 40, 0xE8, 0x26, p->vFlip = 1;)
    for (i = 0; i < 12; i++)
    {
        TEXT_BOX_PIECE(p, y - shake, i * 8 + 0x88, 0x25, )
        TEXT_BOX_PIECE(p, 40, i * 8 + 0x88, 0x25, p->vFlip = 1;)
    }
}
*/
INCLUDE_ASM(Func_0201BEA0);

void Func_0201C0E8(u8 playSound)
{
    u32 params[2];
    u32 mask;
    s32 r;

    TextClearCanvas();
    gGameState.textX = 0;
    gGameState.textY = 8;
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

void DrawChoiceCursor(void)
{
    u32 mosaic;
    const u8 *sprite;
    u8 y;
    u8 timer;

    mosaic = (gMain.bgcnt[0] >> 6) & 1;
    timer = --gGameState.unk152;
    sprite = (const u8 *)gSprite_0202C130;
    if (timer == 0)
    {
        gGameState.unk153 ^= 1;
        gGameState.unk152 = (sprite + gGameState.unk153 * 8)[4];
    }
    y = gGameState.unk150 + gGameState.unk14F * 12;
    y += 0x34;
    DrawSprite(sprite, gGameState.unk153, 0x8C, y, 0, mosaic, 0, 0, 0, 0, 0);
}

// The choice menu after a question (unk14F: cursor; two choices when unk151 is 1, else
// three). Up/Down move the cursor, A picks: the sound for (unk142, choice) from the tables
// is played, or 0xFFFF closes the menu. Returns 1 while waiting, 0 after a pick, 3 for 0xFFFF.
// The result is a u8 here; callers see s32 (the prototype in game.h; MsgStep returns it as is).
u8 ChoiceMenuUpdate_(void) asm("ChoiceMenuUpdate");
u8 ChoiceMenuUpdate_(void)
{
    s32 i;

    if (gGameState.unk151 == 1)
    {
        if (gMain.newKeys & 0xC0)
        {
            gGameState.unk14F ^= 1;
            SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
            gGameState.unk153 = 0;
            gGameState.unk152 = gSprite_0202C130[0].duration;
        }
    }
    else if (gMain.newKeys & 0xC0)
    {
        SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
        if (gMain.newKeys & 0x80)
        {
            if (++gGameState.unk14F > 2)
                gGameState.unk14F = 0;
        }
        else
        {
            if (--gGameState.unk14F > 2)
                gGameState.unk14F = 2;
        }
        gGameState.unk153 = 0;
        gGameState.unk152 = gSprite_0202C130[0].duration;
    }
    DrawChoiceCursor();
    if (gMain.newKeys & 1)
    {
        i = 0;
        if (gGameState.unk151 == 1)
        {
            while (gTwoChoiceIds[i] != gGameState.unk142)
                i++;
            if (gTwoChoiceMsgIds[i * 2 + gGameState.unk14F] == 0xFFFF)
            {
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                gGameState.unk13E = 300;
                return 3;
            }
            if (gTwoChoiceMsgIds[i * 2 + gGameState.unk14F] == 0x1A7)
                gGameState.unk11E = 1;
            ShowMessage(gTwoChoiceMsgIds[i * 2 + gGameState.unk14F], gGameState.unk226);
        }
        else
        {
            while (gThreeChoiceIds[i] != gGameState.unk142)
                i++;
            if (gThreeChoiceMsgIds[i * 3 + gGameState.unk14F] == 0xFFFF)
            {
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                return 3;
            }
            ShowMessage(gThreeChoiceMsgIds[i * 3 + gGameState.unk14F], gGameState.unk226);
        }
        gGameState.unk151 = 0;
        gGameState.unk150 = 0;
        gGameState.unk14F = 0xFF;
        gGameState.unk14E = 0;
        gMain.newKeys ^= 1;
        SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        return 0;
    }
    return 1;
}

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
        SoundPlayLoop(gGameState.unk140, gGameState.unk140, 0);
        if (gGameState.msgPtr[1] == 0xF9)
        {
            gGameState.msgPtr++;
            gGameState.unk14D = *++gGameState.msgPtr;
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
    entry = gTingleAnims + i;
    gGameState.unk202 = (**entry)[4];
}

bool32 Func_0201C60C(void)
{
    u8 step;

    if (gGameState.textX < 0x60)
        return TRUE;
    gGameState.textX = 0;
    if (gGameState.unk15A)
        step = 24;
    else
        step = 12;
    gGameState.textY += step;
    step -= 12;
    if (gGameState.textY < 0x50 - step)
        return TRUE;
    return FALSE;
}

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
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, ch);
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
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0xC4);
        if (ch <= 0xEA)
            ch += 0x33;
        else
            ch = gGlyphTable_0202438E[ch - 0xEB];
    }
    else if (ch > 0xBC)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0xC2);
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
    TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, ch);
    if (gGameState.unk15A)
        gGameState.textX += gGameState.unk157 * 2;
    else
        gGameState.textX += gGameState.unk157;
    if ((u16)gGameState.unk140 == 0xFFFF)
        SoundPlay(0x28, 0);
    return TRUE;
}
#else
// The US font has accented letters: the base glyph, then a small accent glyph over it (the
// accent only outside the link-lost screen; 0x9E/0x9F draw the small glyph first).
// Near-miss: identical but for the accent's table address in the 7 accent branches, which the
// original forms as (index * 2 + 1) + table and this as index * 2 + (table + 1).
/*
#define DRAW_ACCENTED(table, first, accent)                                                   \
    {                                                                                         \
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A,                  \
                      table[ch - first][0]);                                                  \
        if (gMain.mode != 2)                                                                  \
            TextDrawSmallGlyph(gGameState.textX, gGameState.textY + table[ch - first][1],     \
                               gGameState.unk14A, accent);                                    \
    }

bool32 DrawChar(u8 ch)
{
    if (ch == 0x9F)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0x8A);
        if (gMain.mode != 2)
            TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0);
    }
    else if (ch == 0x9E)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0x89);
        if (gMain.mode != 2)
            TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, 0);
    }
    else if (ch > 0x9B)
        DRAW_ACCENTED(gAccentGlyphs_0202AFF4, 0x9C, 0x84)
    else if (ch > 0x99)
        DRAW_ACCENTED(gAccentGlyphs_0202AFF8, 0x9A, 0x8B)
    else if (ch > 0x8D)
        DRAW_ACCENTED(gAccentGlyphs_0202AFFC, 0x8E, 0x88)
    else if (ch > 0x87)
        DRAW_ACCENTED(gAccentGlyphs_0202B014, 0x88, 0x83)
    else if (ch > 0x7D)
        DRAW_ACCENTED(gAccentGlyphs_0202B020, 0x7E, 0x85)
    else if (ch > 0x71)
        DRAW_ACCENTED(gAccentGlyphs_0202B034, 0x72, 0x86)
    else if (ch > 0x67)
        DRAW_ACCENTED(gAccentGlyphs_0202B04C, 0x68, 0x87)
    else
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.unk14A, ch);
    if (gGameState.unk15A)
        gGameState.textX += gGameState.unk157 * 2;
    else
        gGameState.textX += gGameState.unk157;
    if ((u16)gGameState.unk140 == 0xFFFF)
        SoundPlay(0x28 - SOUND_ID_SHIFT, 0);
    return TRUE;
}
#undef DRAW_ACCENTED
*/
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
            return ChoiceMenuUpdate();
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
                    gGameState.unk152 = gSprite_0202C130[0].duration;
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
                    gGameState.unk152 = gSprite_0202C130[0].duration;
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
        StepTowardS8(&gGameState.msgBoxPos, target, 2);
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

// Show message id (0-0x437) with Tingle's voice. Callers see an s16 parameter (the prototype
// in game.h); it is narrowed to u16 here.
bool32 ShowMessage_(u16 id, u8 arg) asm("ShowMessage");
bool32 ShowMessage_(u16 id, u8 arg)
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
