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

// Sound per message sound code (PlayMsgSound); codes 0x20-0x22 are SoundPlayLoop IDs.
#define SE(id) ((id) < 7 ? (id) : (id) - SOUND_ID_SHIFT)
const u16 gMsgSoundIds[] =
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
const u8 gVoicedKanaGlyphs[6] =
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

const u8 *const gTingleAnim0[1] =
{
    gTingleSprite
};

const u8 *const gTingleAnim2[2] =
{
    gSprite_0202C650, gTingleSprite
};

const u8 *const gTingleAnim3[1] =
{
    gTingleSprite
};

const u8 *const gTingleAnim4[2] =
{
    gSprite_0202C650, gTingleSprite
};

const u8 *const gTingleAnim5[1] =
{
    gSprite_0202C650
};

const u8 *const gTingleAnim7[1] =
{
    gSprite_0202C788
};

const u8 *const gTingleAnim8[1] =
{
    gSprite_0202C7C8
};

const u8 *const gTingleAnim9[1] =
{
    gSprite_0202C848
};

const u8 *const gTingleAnim10[2] =
{
    gSprite_0202C850, gSprite_0202C650
};

const u8 *const gTingleAnim11[1] =
{
    gSelectionIconSprite
};

const u8 *const *const gTingleAnims[15] =
{
    gTingleAnim0,
    gTingleAnim0,
    gTingleAnim2,
    gTingleAnim3,
    gTingleAnim4,
    gTingleAnim5,
    gTingleAnim3,
    gTingleAnim7,
    gTingleAnim8,
    gTingleAnim9,
    gTingleAnim10,
    gTingleAnim11,
    gTingleAnim0,
    gTingleAnim0,
    gTingleAnim0
};

// Draw BG map row `row` of the message box: its border tiles and 12 text canvas tiles.
void DrawMsgBoxRow(s8 row)
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

// Put back the two BG map rows under the message box row (from the copy at 0x06007800).
void RestoreMsgBoxRow(s8 row)
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
    if (gGameState.glyphWidth == 8)
        mask2 = -1;
    else
        mask2 = 0x11FFFFFF;
#ifdef VERSION_GZLJ01
    for (; n > 0; dst++, n--)
#else
    // The US stops after `rows` rows (the JP only uses it for m).
    for (; n > 0 && rows != 0; dst++, n--, rows--)
#endif
    {
        prev = dst;
        w = *src++ & mask & mask2;
        if (shift)
        {
            *dst |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
#ifndef VERSION_GZLJ01
            if (w != 0x11111111)  // skip an all-background word
#endif
                prev[8] = w;
        }
        else
        {
            *dst = w;
        }
    }
    dst += 0x58;
#ifdef VERSION_GZLJ01
    for (; m > 0; dst++, m--)
#else
    for (; m > 0 && rows != 0; dst++, m--, rows--)
#endif
    {
        prev = dst;
        w = *src++ & mask;
        if (shift)
        {
            *dst |= w << ((8 - shift) * 4);
            for (i = shift; i > 0; i--)
                w = (w >> 4) | 0x10000000;
#ifndef VERSION_GZLJ01
            if (w != 0x11111111)  // skip an all-background word
#endif
                prev[8] = w;
        }
        else
        {
            *dst = w;
        }
    }
}

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

// Draw an 8x8 glyph (from 0x0600A000) at pixel (x, y) of the text canvas; with textBig set,
// scaled 2x into four tiles. TextDrawSmallGlyph is the same for the 4-row glyphs at 0x0600A010.
#ifdef VERSION_GZLJ01
void TextDrawGlyph(u8 x, u8 y, u8 color, u16 ch)
{
    u32 *glyph = (u32 *)0x0600A000 + ch * 8;
    u32 *dst;
    u32 row;
    u32 col;
    u8 y2;

    if (gGameState.textBig)
    {
        TextScaleGlyph(32, (u8 *)glyph);
        y += 4;
        row = (y >> 3) * 0x180 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
        TextBlitGlyph(x + gGameState.glyphWidth, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst + 8);
        y2 = y + 8;
        TextBlitGlyph(x, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 16, dst + 0x60);
        TextBlitGlyph(x + gGameState.glyphWidth, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 24, dst + 0x68);
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
// US: the arms keep their own row/col (one shared row makes gGameState's address, kept across
// the blits, take a register instead of the original's stack slot).
void TextDrawGlyph(u8 x, u8 y, u8 color, u16 ch)
{
    u32 *glyph = (u32 *)0x0600A000 + ch * 8;
    u32 *dst;

    if (gGameState.textBig)
    {
        u32 row;
        u32 col;
        u8 y2;
        u32 col2;
        s32 x2;

        TextScaleGlyph(32, (u8 *)glyph);
        y += 4;
        row = (y >> 3) * 0x180 + (y & 7) * 4;
        col = (x >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
        y2 = y + 8;
        TextBlitGlyph(x, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 16, dst + 0x60);
        // The right halves go where x + glyphWidth falls, not simply the next tile.
        x2 = x + gGameState.glyphWidth;
        col2 = (x2 >> 3) * 32 + 0x0600F100;
        dst = (u32 *)(row + col2);
        TextBlitGlyph(x2, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst);
        TextBlitGlyph(x + gGameState.glyphWidth, y2, color, 8, (u32 *)TEXT_GLYPH_BUF + 24, dst + 0x60);
    }
    else
    {
        u32 row;
        u32 col;

        // The link-lost screen (mode 2) has a 30-tile-wide canvas.
        if (gMain.mode != 2)
        {
            row = (y >> 3) * 0x180 + (y & 7) * 4;
            col = (x >> 3) * 32 + 0x0600F100;
        }
        else
        {
            row = (y >> 3) * 0x3C0 + (y & 7) * 4;
            col = (x >> 3) * 32 + 0x0600F100;
        }
        dst = (u32 *)(row + col);
        TextBlitGlyph(x, y, color, 8, glyph, dst);
    }
}
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
    if (gGameState.textBig)
    {
        TextScaleGlyph(16, (u8 *)glyph);
        TextBlitGlyph(x, y, color, 8, (u32 *)TEXT_GLYPH_BUF, dst);
#ifdef VERSION_GZLJ01
        TextBlitGlyph(x + gGameState.glyphWidth, y, color, 8, (u32 *)TEXT_GLYPH_BUF + 8, dst + 8);
#else
        // The right half goes where x + glyphWidth falls, not simply the next tile.
        x2 = x + gGameState.glyphWidth;
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
// The mosaic local (not a literal 0) makes loop.c hoist the edge loop's x (0x88) before the
// constants 0x25 and ~0xC (found with decomp-permuter).
#define TEXT_BOX_PIECE(p, Y, X, TILE, VFLIP) \
    p->y = Y;                                \
    p->mosaic = mosaic;                      \
    p->x = X;                                \
    VFLIP                                    \
    p->size = 0;                             \
    p->tileNum = TILE;                       \
    p->priority = 0;                         \
    p->paletteNum = 8;                       \
    p++;                                     \
    gMain.oamCount++;

void DrawMsgBoxFrame(s8 row)
{
    struct OamData16 *p = (struct OamData16 *)&gOamBuffer[gMain.oamCount];
    s32 y;
    s32 shake;
    s32 t;
    s32 i;
    u8 mosaic = 0;

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

// Clear the text canvas and home the text cursor; with playSound, a random voice blip.
void MsgNewPage(u8 playSound)
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
    timer = --gGameState.choiceCursorTimer;
    sprite = (const u8 *)gChoiceCursorSprite;
    if (timer == 0)
    {
        gGameState.choiceCursorFrame ^= 1;
        gGameState.choiceCursorTimer = (sprite + gGameState.choiceCursorFrame * 8)[4];
    }
    y = gGameState.choiceY + gGameState.choiceCursor * 12;
    y += 0x34;
    DrawSprite(sprite, gGameState.choiceCursorFrame, 0x8C, y, 0, mosaic, 0, 0, 0, 0, 0);
}

// The choice menu after a question (choiceCursor: cursor; two choices when choiceMenu is 1, else
// three). Up/Down move the cursor, A picks: the sound for (msgId, choice) from the tables
// is played, or 0xFFFF closes the menu. Returns 1 while waiting, 0 after a pick, 3 for 0xFFFF.
// The result is a u8 here; callers see s32 (the prototype in game.h; MsgStep returns it as is).
u8 ChoiceMenuUpdate_(void) asm("ChoiceMenuUpdate");
u8 ChoiceMenuUpdate_(void)
{
    s32 i;

    if (gGameState.choiceMenu == 1)
    {
        if (gMain.newKeys & 0xC0)
        {
            gGameState.choiceCursor ^= 1;
            SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
            gGameState.choiceCursorFrame = 0;
            gGameState.choiceCursorTimer = gChoiceCursorSprite[0].duration;
        }
    }
    else if (gMain.newKeys & 0xC0)
    {
        SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
        if (gMain.newKeys & 0x80)
        {
            if (++gGameState.choiceCursor > 2)
                gGameState.choiceCursor = 0;
        }
        else
        {
            if (--gGameState.choiceCursor > 2)
                gGameState.choiceCursor = 2;
        }
        gGameState.choiceCursorFrame = 0;
        gGameState.choiceCursorTimer = gChoiceCursorSprite[0].duration;
    }
    DrawChoiceCursor();
    if (gMain.newKeys & 1)
    {
        i = 0;
        if (gGameState.choiceMenu == 1)
        {
            while (gTwoChoiceIds[i] != gGameState.msgId)
                i++;
            if (gTwoChoiceMsgIds[i * 2 + gGameState.choiceCursor] == 0xFFFF)
            {
                gGameState.choiceMenu = 0;
                gGameState.choiceY = 0;
                gGameState.choiceCursor = 0xFF;
                gGameState.choiceIndex = 0;
                gGameState.msgWaitTimer = 300;
                return 3;
            }
            if (gTwoChoiceMsgIds[i * 2 + gGameState.choiceCursor] == 0x1A7)
                gGameState.placeTargetAtCursor = 1;
            ShowMessage(gTwoChoiceMsgIds[i * 2 + gGameState.choiceCursor], gGameState.tingleAnim);
        }
        else
        {
            while (gThreeChoiceIds[i] != gGameState.msgId)
                i++;
            if (gThreeChoiceMsgIds[i * 3 + gGameState.choiceCursor] == 0xFFFF)
            {
                gGameState.choiceMenu = 0;
                gGameState.choiceY = 0;
                gGameState.choiceCursor = 0xFF;
                gGameState.choiceIndex = 0;
                return 3;
            }
            ShowMessage(gThreeChoiceMsgIds[i * 3 + gGameState.choiceCursor], gGameState.tingleAnim);
        }
        gGameState.choiceMenu = 0;
        gGameState.choiceY = 0;
        gGameState.choiceCursor = 0xFF;
        gGameState.choiceIndex = 0;
        gMain.newKeys ^= 1;
        SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        return 0;
    }
    return 1;
}

// Message code F6 (and the sound byte of slots 5 and 15): play gMsgSoundIds[id]. 0x0C-0x0F
// are a random one of three voice blips, 0x20-0x22 a loop sound (msgLoopSound; a following
// F9 sets the delay), 0x16 nothing.
void PlayMsgSound(u8 id)
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
        SoundPlay(gMsgSoundIds[id], params);
    }
    else if ((u8)(id - 0x20) <= 2)
    {
        gGameState.msgLoopSound = gMsgSoundIds[id];
        SoundPlayLoop(gGameState.msgLoopSound, gGameState.msgLoopSound, 0);
        if (gGameState.msgPtr[1] == 0xF9)
        {
            gGameState.msgPtr++;
            gGameState.textWait = *++gGameState.msgPtr;
        }
    }
    else if (id != 0x16)
    {
        SoundPlay(gMsgSoundIds[id], 0);
    }
}

// Start Tingle's animation gGameState.tingleAnim from its first frame (with its scale).
void StartTingleAnim(void)
{
    u8 i;
    const u8 *const *const *entry;

    gGameState.tingleScaleDir = 0;
    if (gGameState.tingleAnim == 2)
    {
        gGameState.tingleScale = 0x300;
    }
    else if (gGameState.tingleAnim == 3)
    {
        gGameState.tingleScale = 0x100;
    }
    else if (gGameState.tingleAnim == 11)
    {
        gGameState.tingleScale = 0;
        gGameState.tinglePulseGrow = 0;
        gGameState.tinglePulse = 0x100;
    }
    else
    {
        gGameState.tingleScale = 0;
    }
    gGameState.tingleFrame = 0;
    gGameState.tingleAnimPart = 0;
    i = gGameState.tingleAnim;
    entry = gTingleAnims + i;
    gGameState.tingleTimer = (**entry)[4];
}

// Wrap the text cursor to the next line at the right edge; FALSE when the box is full.
bool32 TextWrapLine(void)
{
    u8 step;

    if (gGameState.textX < 0x60)
        return TRUE;
    gGameState.textX = 0;
    if (gGameState.textBig)
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
    u8 width = gGameState.glyphWidth;
#endif
    s32 i;

#ifndef VERSION_GZLJ01
    gGameState.glyphWidth = 8;
#endif
    for (i = 0; i < n; i++)
    {
        if (!TextWrapLine())
            return;
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, ch);
        ch++;
        if (gGameState.textBig)
            gGameState.textX += gGameState.glyphWidth * 2;
        else
            gGameState.textX += gGameState.glyphWidth;
    }
#ifndef VERSION_GZLJ01
    gGameState.glyphWidth = width;
#endif
}

#ifdef VERSION_GZLJ01
bool32 DrawChar(u8 ch)
{
    if (!TextWrapLine())
        return FALSE;
    if (ch == 0xBC)
        gGameState.charLoopSound = 1;
    else
        gGameState.charLoopSound = 0;
    if (ch > 0xE5)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0xC4);
        if (ch <= 0xEA)
            ch += 0x33;
        else
            ch = gVoicedKanaGlyphs[ch - 0xEB];
    }
    else if (ch > 0xBC)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0xC2);
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
            ch = gVoicedKanaGlyphs[ch - 0xE0];
    }
    TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, ch);
    if (gGameState.textBig)
        gGameState.textX += gGameState.glyphWidth * 2;
    else
        gGameState.textX += gGameState.glyphWidth;
    if ((u16)gGameState.msgLoopSound == 0xFFFF)
        SoundPlay(0x28, 0);
    return TRUE;
}
#else
// The US font has accented letters: the base glyph, then a small accent glyph over it (the
// accent only outside the link-lost screen; 0x9E/0x9F draw the small glyph first).
// The accent offset is read as table[0][index * 2 + 1]: with table[index][1], agbcc folds the
// + 1 into the table address instead of the index.
#define DRAW_ACCENTED(table, first, accent)                                                   \
    {                                                                                         \
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor,                  \
                      table[ch - first][0]);                                                  \
        if (gMain.mode != 2)                                                                  \
            TextDrawSmallGlyph(gGameState.textX,                                              \
                               gGameState.textY + table[0][(ch - first) * 2 + 1],             \
                               gGameState.textColor, accent);                                    \
    }

bool32 DrawChar(u8 ch)
{
    if (ch == 0x9F)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0x8A);
        if (gMain.mode != 2)
            TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0);
    }
    else if (ch == 0x9E)
    {
        TextDrawSmallGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0x89);
        if (gMain.mode != 2)
            TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, 0);
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
        TextDrawGlyph(gGameState.textX, gGameState.textY, gGameState.textColor, ch);
    if (gGameState.textBig)
        gGameState.textX += gGameState.glyphWidth * 2;
    else
        gGameState.textX += gGameState.glyphWidth;
    if ((u16)gGameState.msgLoopSound == 0xFFFF)
        SoundPlay(0x28 - SOUND_ID_SHIFT, 0);
    return TRUE;
}
#undef DRAW_ACCENTED
#endif

#ifndef VERSION_GZLJ01
bool32 UsDrawChar(u8 ch)
{
    if (!TextWrapLine())
        return FALSE;
    if (ch == 0x3F)
    {
        gGameState.charLoopSound = 1;
        ch = 0x18;
    }
    else
    {
        gGameState.charLoopSound = 0;
    }
    return DrawChar(ch);
}
#endif

void MsgCmd00(void)
{
    u16 *map = (u16 *)0x0600704E;

    gGameState.balloonDiscount = 1;
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

    gGameState.tingDiscount = 1;
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
        gGameState.textX += gGameState.glyphWidth;
    }
    DRAW_CHAR(gGameState.flags.hour % 10 + GLYPH_DIGIT_0);
#else
    if (DRAW_CHAR(gGameState.flags.hour / 10 + GLYPH_DIGIT_0))
        DRAW_CHAR(gGameState.flags.hour % 10 + GLYPH_DIGIT_0);
#endif
}

void DrawMinute(void)
{
    if (DRAW_CHAR(gGameState.flags.minute / 10 + GLYPH_DIGIT_0))
        DRAW_CHAR(gGameState.flags.minute % 10 + GLYPH_DIGIT_0);
}

void MsgCmd05(void)
{
    gGameState.shopUnlocked = 1;
    SetSlotItem(6, 7);
}

void MsgCmd06(void)
{
    gGameState.shopUnlocked = 0;
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
    gGameState.textFast = 1;
}

void MsgCmd0E(void)
{
    gGameState.textFast = 0;
}

void MsgCmd0F(void)
{
    gGameState.textBig = 1;
}

void MsgCmd10(void)
{
    gGameState.glyphWidth = 6;
}

void MsgCmd11(void)
{
    gGameState.glyphWidth = 8;
}

void MsgCmd12(void)
{
    SendItemUse(0x15, 0);
    gGameState.itemsEnabled = 0;
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

    if ((u16)gGameState.msgLoopSound != 0xFFFF)
    {
        if ((u8)(gGameState.msgStatus - 2) > 1 || gGameState.textWait != 0)
            SoundPlayLoop((u16)gGameState.msgLoopSound, gGameState.msgLoopSound, 0);
        else
            gGameState.msgLoopSound = (u16)gGameState.msgLoopSound | 0xFFFF;
    }
    if (gGameState.textBig)
        n = 12;
    else
        n = 0;
    if (gGameState.charLoopSound && (u16)gGameState.msgLoopSound == 0xFFFF)
        SoundPlayLoop(0, 0, 0);
    if (gGameState.textWait != 0)
    {
        gGameState.textWait--;  // delay (code 0xF9)
    }
    else
    {
        gGameState.textWait = gGameState.textDelay;
        if (gGameState.msgStatus == 2 || gGameState.textY >= 0x50 - n)
        {
            // Page full: show the arrow until A (or the timeout), then clear the window.
            if (gGameState.flags.gcInput)
                gGameState.msgWaitTimer++;
            if (gGameState.msgWaitTimer <= 0x12B && !(gMain.newKeys & 1))
            {
                DrawSprite(gMsgNextArrowSprite, (gMain.frameCount & 0xC) == 0, 0xE0, 0x84, 0, mosaic,
                           0, 0, 0, 0, 0);
                return 2;
            }
            gMain.newKeys &= ~1;
            MsgNewPage(1);
            gGameState.textBig = 0;
            gGameState.msgWaitTimer = 0;
            gGameState.msgStatus = 0;
            return 0;
        }
        if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && gGameState.choiceMenu != 0)
            return ChoiceMenuUpdate();
        while (ch > 0xF1)
        {
            if (ch == 0xFF)  // end
            {
                gGameState.textFast = 0;
                gGameState.charLoopSound = 0;
                gGameState.textBig = 0;
                #ifndef VERSION_GZLJ01
                if (gGameState.choiceMenu)
                    return 1;
#endif
                return 3;
            }
            if (ch == 0xFD || ch == 0xF4)  // space
            {
            space:
                gGameState.msgPtr++;
                if (gGameState.textBig)
                    gGameState.textX += gGameState.glyphWidth * 2;
                else
                    gGameState.textX += gGameState.glyphWidth;
                return 0;
            }
            if (ch == 0xFA || ch == 0xFE)  // 0xFA: wait, 0xFE: new line
            {
                if (gGameState.textBig)
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
                gGameState.textWait = *++gGameState.msgPtr;
                gGameState.msgPtr++;
                // (A return here would be laid out differently.)
                goto end;
            }
            else if (ch == 0xF8)
            {
                n = *++gGameState.msgPtr;
                if (gGameState.choiceY == 0)
                    gGameState.choiceY = gGameState.textY;
                if (n == 0)
                {
                    gGameState.choiceCursor = gGameState.choiceIndex;
                    gGameState.choiceMenu = 1;
                    gGameState.choiceCursorFrame = n;
                    gGameState.choiceCursorTimer = gChoiceCursorSprite[0].duration;
                }
                else if (gGameState.choiceCursor == 0xFF)
                {
                    gGameState.choiceIndex++;
                }
                goto space;
            }
            else if (ch == 0xF7)
            {
                n = *++gGameState.msgPtr;
                if (gGameState.choiceY == 0)
                    gGameState.choiceY = gGameState.textY;
                if (n == 0)
                {
                    gGameState.choiceCursor = gGameState.choiceIndex;
                    gGameState.choiceMenu = 2;
                    gGameState.choiceCursorFrame = n;
                    gGameState.choiceCursorTimer = gChoiceCursorSprite[0].duration;
                }
                else if (gGameState.choiceCursor == 0xFF)
                {
                    gGameState.choiceIndex++;
                }
                goto space;
            }
            else if (ch == 0xF6)
            {
                n = *++gGameState.msgPtr;
                PlayMsgSound(n);
                gGameState.msgPtr++;
                return 1;
            }
            else if (ch == 0xF5)
            {
                gGameState.tingleAnim = gMsgTingleAnims[*++gGameState.msgPtr];
                StartTingleAnim();
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
                gGameState.textColor = *++gGameState.msgPtr;
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
// while A is held), then run Tingle's animation (switch on gGameState.tingleAnim; tingleAnimPart
// picks the sprite list, tingleFrame the frame, tingleTimer counts the frame's duration,
// tingleScale the scale) and draw him, and the wind arrow at (0xC8, 0x58) while
// flags.showWind is set.
// agbcc -O2 runs gcse (LCM-based PRE) on the pool load of gGameState's address; calls kill
// it, so case 11's DrawSprite call feeding the switch join directly (its own `sprite = ...;
// break;`, not a fall into talk) makes the other cases' last gGameState reads reload it.
void UpdateMsgAndTingle(void)
{
    s32 target;
    u32 mosaic;
    u8 frame;
    s32 x;
    u8 a10;
    const u8 *sprite;
    u8 r;
    u8 anim;
    s32 ofsY;

    if (gGameState.msgOpen)
        target = 10;
    else
        target = 0;
    mosaic = (gMain.bgcnt[0] >> 6) & 1;
    frame = 0;
    a10 = 0;
    if (gGameState.msgBoxPos != target)
    {
        StepTowardS8(&gGameState.msgBoxPos, target, 2);
        if (gGameState.msgOpen)
        {
            DrawMsgBoxRow(gGameState.msgBoxPos);
            gGameState.msgStatus = 1;
        }
        else
        {
            RestoreMsgBoxRow(gGameState.msgBoxPos);
            if (gGameState.msgBoxPos == 0)
            {
                if (gGameState.unk21C)
                {
                    gGameState.unk0E5 = 0;
                    gGameState.unk21C = 0;
                }
                gGameState.inMessage = 0;
                gGameState.tingleAnim = 0;
                gGameState.tingleAnimPart = 0;
                gGameState.tingleFrame = 0;
                if (gGameState.flags.restarting && gMain.modeState == 2)
                    Mode1StartFadeOut();
                gGameState.msgId = 0xFFFF;
                gGameState.active = 1;
            }
        }
        DrawMsgBoxFrame(gGameState.msgBoxPos);
    }
    else if (gGameState.msgOpen && gGameState.msgId >= 0)
    {
        do
        {
            r = MsgStep();
            if (gGameState.effectStep == 1 || r != 0)
                break;
        } while ((gMain.newKeys & 1) || gGameState.textFast);
        gGameState.msgStatus = r;
        DrawMsgBoxFrame(gGameState.msgBoxPos);
    }
    else if (gGameState.inMessage)
    {
        gGameState.inMessage = 0;
    }
    if (gGameState.inMessage || gGameState.menuOpen || gGameState.chartOpen
     || (!gGameState.flags.busy && gGameState.active))
    {
        gGameState.animateMarkers = 1;
        anim = gGameState.tingleAnim;
        sprite = (*(anim + gTingleAnims))[gGameState.tingleAnimPart];
        x = 0xB8;
        switch (anim)
        {
        case 14:
            x = 0x98;
            a10 = 2;
        case 1:
            gGameState.tingleScale = 0;
            if (gGameState.msgStatus)
                goto talk;
            if (gGameState.tingleTimer == 0)
            {
                if (++gGameState.tingleFrame > 8)
                    gGameState.tingleFrame = 0;
                gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 2:
            if (gGameState.tingleAnimPart && gGameState.msgStatus)
                goto talk;
            if (gGameState.tingleTimer == 0)
            {
                gGameState.tingleFrame++;
                if (gGameState.tingleAnimPart)
                {
                    if (gGameState.tingleFrame > 8)
                        gGameState.tingleFrame = 0;
                    gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
                }
                else
                {
                    gGameState.tingleFrame = 0;
                    if (gGameState.tingleScaleDir == 0)
                    {
                        if (gGameState.tingleScale != 0xA0)
                        {
                            gGameState.tingleScale -= 0x20;
                        }
                        else
                        {
                            gGameState.tingleTimer = 10;
                            gGameState.tingleScaleDir = 1;
                        }
                    }
                    else if (gGameState.tingleScale != 0x100)
                    {
                        gGameState.tingleScale += 0x10;
                    }
                    else
                    {
                        gGameState.tingleAnimPart++;
                        sprite = gTingleAnims[gGameState.tingleAnim][gGameState.tingleAnimPart];
                        gGameState.tingleTimer = sprite[4];
                    }
                }
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 3:
            if (gGameState.tingleScaleWait == 0)
            {
                if (gGameState.tingleScaleDir == 0)
                {
                    if (gGameState.tingleScale != 0xA0)
                        gGameState.tingleScale -= 8;
                    else
                        gGameState.tingleScaleDir = 1;
                }
                else if (gGameState.tingleScale != 0x100)
                {
                    gGameState.tingleScale += 8;
                }
                else
                {
                    gGameState.tingleScaleWait = 10;
                    gGameState.tingleScaleDir = 0;
                }
            }
            else
            {
                gGameState.tingleScaleWait--;
            }
        case 6:
            if (gGameState.msgStatus)
                goto talk;
            if (gGameState.tingleTimer == 0)
            {
                if (++gGameState.tingleFrame > 8)
                    gGameState.tingleFrame = 0;
                gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 4:
            gGameState.tingleScale = 0;
            if (gGameState.tingleAnimPart && gGameState.msgStatus)
                goto talk;
            if (gGameState.tingleTimer == 0)
            {
                gGameState.tingleFrame++;
                if (gGameState.tingleAnimPart)
                {
                    if (gGameState.tingleFrame > 8)
                        gGameState.tingleFrame = 0;
                    gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
                }
                else
                {
                    if (gGameState.tingleFrame > 0x1D)
                    {
                        gGameState.tingleFrame = 0;
                        gGameState.tingleAnimPart++;
                        sprite = gTingleAnims[gGameState.tingleAnim][gGameState.tingleAnimPart];
                    }
                    gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
                }
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 5:
            gGameState.tingleScale = 0;
            if (gGameState.tingleTimer == 0)
            {
                if (++gGameState.tingleFrame > 0x1D)
                {
                    if (gGameState.effectTimer)
                    {
                        gGameState.tingleFrame = 0;
                        gGameState.tingleTimer = sprite[4];
                    }
                    else
                    {
                        gGameState.tingleFrame = 0;
                        gGameState.tingleAnim = 0;
                        gGameState.tingleAnimPart = 0;
                        sprite = gTingleAnims[0][0];
                    }
                }
                else
                {
                    gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
                }
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 7:
            gGameState.tingleScale = 0;
            if (gGameState.tingleTimer == 0)
            {
                gGameState.tingleFrame = (gGameState.tingleFrame + 1) & 7;
                gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 8:
            gGameState.tingleScale = 0;
            if (gGameState.tingleTimer == 0)
            {
                gGameState.tingleFrame = (gGameState.tingleFrame + 1) & 15;
                gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
            }
            else
            {
                gGameState.tingleTimer--;
            }
            frame = gGameState.tingleFrame;
            break;
        case 9:
            gGameState.tingleScale = 0;
            frame = gGameState.tingleFrame;
            break;
        case 10:
            if (gGameState.tingleTimer == 0)
            {
                gGameState.tingleFrame++;
                if (gGameState.tingleAnimPart)
                {
                    if (gGameState.tingleScale > 0x2FF)
                        gGameState.tingleScale = 0x300;
                    else
                        gGameState.tingleScale += 0x10;
                }
                else
                {
                    if (gGameState.tingleFrame > 6)
                    {
                        gGameState.tingleFrame = 0;
                        gGameState.tingleScale = 0x100;
                        gGameState.tingleAnimPart++;
                        sprite = gTingleAnims[gGameState.tingleAnim][gGameState.tingleAnimPart];
                    }
                    else
                    {
                        gGameState.tingleScale = 0;
                    }
                    gGameState.tingleTimer = sprite[gGameState.tingleFrame * 8 + 4];
                }
            }
            else
            {
                gGameState.tingleTimer--;
            }
            break;
        case 11:
            gGameState.tingleScale = 0;
            if (gGameState.tinglePulseGrow)
            {
                gGameState.tinglePulse += 4;
                if (gGameState.tinglePulse > 0xFF)
                    gGameState.tinglePulseGrow = 0;
            }
            else
            {
                gGameState.tinglePulse -= 4;
                if (gGameState.tinglePulse <= 0xC0)
                    gGameState.tinglePulseGrow = 1;
            }
            DrawSprite(sprite, 0, 0xCC, 0xE, 0, mosaic, gGameState.tinglePulse, 0, 0, 0, 0);
            frame = 1;
            sprite = gTingleSprite;
            break;
        talk:
            sprite = gTingleSprite;
            break;
        case 12:
            gGameState.tingleScale = 0;
            frame = 1;
            break;
        case 13:
            gGameState.tingleScale = 0;
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
            if (!mosaic && gGameState.tingleScale != 0 && gGameState.tingleScale != 0x100)
            {
                if (gGameState.tingleScale > 0x100)
                    gGameState.tingleScale -= 8;
                else
                    gGameState.tingleScale += 8;
                if (gGameState.tingleScale == 0x100)
                    gGameState.tingleScale = 0;
            }
            frame = gGameState.tingleFrame;
            break;
        case 15:
            return;
        }
        ofsY = 0x10;
        DrawSprite(sprite, frame, x, 0x18, gGameState.tingleAngle, 0, gGameState.tingleScale, 0, ofsY, 0, a10);
    }
    else
    {
        gGameState.animateMarkers = 0;
    }
    if (gGameState.flags.showWind && gGameState.msgBoxPos == 0)
    {
        if (gGameState.windArrowTimer == 0)
        {
            const u8 *s;

            gGameState.windArrowFrame = (gGameState.windArrowFrame + 1) & 3;
            s = gWindArrowSprite;
            gGameState.windArrowTimer = s[gGameState.windArrowFrame * 8 + 4];
        }
        else
        {
            gGameState.windArrowTimer--;
        }
        DrawSprite(gWindArrowSprite, gGameState.windArrowFrame, 0xC8, 0x58, gGameState.flags.windDir + 0x80, 0,
                   0x100, 0, 0, 0, 0);
    }
}

bool32 MsgReset(void)
{
    gGameState.msgOpen = 0;
    gGameState.msgBoxPos = 0;
    gGameState.msgId = 0xFFFF;
    return TRUE;
}

void MsgStart(s16 id)
{
    if (gGameState.msgId != id)
    {
        MsgNewPage(0);
        gGameState.msgId = id;
#ifdef VERSION_GZLJ01
        gGameState.msgPtr = (u8 *)0x02000000 + ((u16 *)0x02000000)[id];
#else
        // The US message block starts with the offset of the text after the table.
        gGameState.msgPtr = (u8 *)(*(u16 *)0x02000000 + (((u16 *)0x02000002)[id] | 0x02000000));
#endif
        gGameState.textColor = 0;
        gGameState.msgStatus = 0;
        gGameState.choiceY = 0;
        gGameState.choiceCursor = 0xFF;
        gGameState.choiceIndex = 0;
        gGameState.choiceMenu = 0;
        gGameState.textDelay = 0;
        gGameState.textWait = 0;
        gGameState.msgWaitTimer = 0;
        gGameState.textFast = 0;
        gGameState.charLoopSound = 0;
        gGameState.textBig = 0;
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
        if (gGameState.msgOpen != 1)
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
        gGameState.msgOpen = 1;
        gGameState.inMessage = 1;
        gGameState.tingleAnim = arg;
        gGameState.animateMarkers = 1;
        StartTingleAnim();
        MsgStart(id);
    }
    return TRUE;
}

bool32 MsgStartSilent(s16 id)
{
    if ((u16)id <= 0x437)
    {
        gGameState.msgOpen = 1;
        MsgStart(id);
    }
    return TRUE;
}

void MsgClose(void)
{
    if (gGameState.msgOpen)
        SoundPlay(0x17 - SOUND_ID_SHIFT, 0);
    gGameState.msgOpen = 0;
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
                gGameState.textX += gGameState.glyphWidth;
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
                    gGameState.textX += gGameState.glyphWidth;
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
