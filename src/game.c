#include "global.h"
#include "main.h"
#include "game.h"
#include "sound.h"
#include "gba_com.h"

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
void Func_0201C660(u8 id)
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
bool32 Func_0201C704(u8 ch)
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
INCLUDE_ASM(Func_0201C704);
#endif

#ifndef VERSION_GZLJ01
bool32 UsFunc_02023390(u8 ch)
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
    return Func_0201C704(ch);
}
#endif

void Func_0201C86C(void)
{
    u16 *map = (u16 *)0x0600704E;

    gGameState.unk138_4 = 1;
    gGameState.items[3].count = 20;
    DrawItemCount(3, map + 0x40);
    map += 3;
    gGameState.items[4].count = 30;
    DrawItemCount(4, map + 0x40);
    map += 3;
    gGameState.items[5].count = 30;
    if (gGameState.slotItems[4] == 5)
        DrawItemCount(5, map + 0x40);
}

void Func_0201C8D8(void)
{
    u16 *map = (u16 *)0x0600718E;

    gGameState.unk138_5 = 1;
    gGameState.items[8].count = 10;
    DrawItemCount(8, map + 0x40);
    map += 3;
    gGameState.items[9].count = 20;
    gGameState.items[10].count = 40;
    if (gGameState.slotItems[8] == 9)
    {
        DrawItemCount(9, map + 0x40);
        DrawItemCount(10, map + 0x43);
    }
}

#ifndef VERSION_GZLJ01
// The US builds don't draw the weekday name.
void Func_0201C948(void)
{
}
#else
void Func_0201C948(void)
{
    s32 i;

    for (i = 0; i < 2; i++)
    {
        if (Func_0201C704(gDayNameChars[gGameState.flags.dayOfWeek * 2 + i]))
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

void Func_0201C988(void)
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

void Func_0201C9E4(void)
{
    if (DRAW_CHAR(gGameState.flags.unk7_0 / 10 + GLYPH_DIGIT_0))
        DRAW_CHAR(gGameState.flags.unk7_0 % 10 + GLYPH_DIGIT_0);
}

void Func_0201CA24(void)
{
    gGameState.unk138_3 = 1;
    Func_0201DD20(6, 7);
}

void Func_0201CA48(void)
{
    gGameState.unk138_3 = 0;
    Func_0201DD20(6, 0);
}

void Func_0201CA6C(void)
{
}

void Func_0201CA70(void)
{
}

void Func_0201CA74(void)
{
}

void Func_0201CA78(void)
{
}

void Func_0201CA7C(void)
{
}

void Func_0201CA80(void)
{
}

void Func_0201CA84(void)
{
    gGameState.unk158 = 1;
}

void Func_0201CA98(void)
{
    gGameState.unk158 = 0;
}

void Func_0201CAAC(void)
{
    gGameState.unk15A = 1;
}

void Func_0201CAC0(void)
{
    gGameState.unk157 = 6;
}

void Func_0201CAD4(void)
{
    gGameState.unk157 = 8;
}

void Func_0201CAE8(void)
{
    SendItemUse(0x15, 0);
    gGameState.unk1E5_1 = 0;
}

// Message interpreter, one step per call: draws the next character of gGameState.msgPtr or
// runs the control codes (0xF2-0xFF) before it. Returns 0 to go on, 2 while it waits (page
// full, or for A), 3 at the end (0xFF), 1 after code 0xF6.
s32 Func_0201CB10(void)
{
    u8 ch = *gGameState.msgPtr;
    u32 mosaic = (gMain.bgcnt[0] >> 6) & 1;
    bool32 repeat = FALSE;  // 0xFB: stay on the code for the next letter of the name
    u8 n;  // line height, then the argument byte of a code (one variable in the original)

    if ((u16)gGameState.unk140 != 0xFFFF)
    {
        if ((u8)(gGameState.unk14B - 2) > 1 || gGameState.unk14D != 0)
            Func_02021284((u16)gGameState.unk140, gGameState.unk140, 0);
        else
            gGameState.unk140 = (u16)gGameState.unk140 | 0xFFFF;
    }
    if (gGameState.unk15A)
        n = 12;
    else
        n = 0;
    if (gGameState.unk159 && (u16)gGameState.unk140 == 0xFFFF)
        Func_02021284(0, 0, 0);
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
                gGameState.unk226 = gTable_02024354[*++gGameState.msgPtr];
                Func_0201C568();
            }
            else if (ch == 0xF3)  // call a handler
            {
                gMsgCmdFuncs[*++gGameState.msgPtr]();
            }
            else if (ch == 0xF2)  // a glyph run
            {
                n = *++gGameState.msgPtr;
                Func_0201C660(n);
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

INCLUDE_ASM(Func_0201CFE8);

bool32 Func_0201D8AC(void)
{
    gGameState.unk13C = 0;
    gGameState.unk13D = 0;
    gGameState.unk142 = 0xFFFF;
    return TRUE;
}

void Func_0201D8D8(s16 id)
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
        Func_0201D8D8(id);
    }
    return TRUE;
}

bool32 Func_0201DA24(s16 id)
{
    if ((u16)id <= 0x437)
    {
        gGameState.unk13C = 1;
        Func_0201D8D8(id);
    }
    return TRUE;
}

void Func_0201DA50(void)
{
    if (gGameState.unk13C)
        SoundPlay(0x17 - SOUND_ID_SHIFT, 0);
    gGameState.unk13C = 0;
}

#ifndef VERSION_GZLJ01
INCLUDE_ASM(DrawString);
#endif

// Draw the blinking cursor on menu slot (rows 0-1: the item grid, row 2: the bottom row).
void Func_0201DA78(u8 slot)
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
void Func_0201DB50(u8 item, u8 x, u8 y, u8 mosaic)
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
    u8 count = slot->count;
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

void Func_0201DCEC(u8 item)
{
    u16 *map = (u16 *)0x06006464;

    gGameState.unk1DE = item;
    DrawItemIcon(item, map);
    map += 2;
    DrawItemCount(item, map);
}

// Put item in menu slot (5 per row, 3 tiles apart; count below the icon).
void Func_0201DD20(u8 slot, u8 item)
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
void Func_0201DDA0(void)
{
    if (gGameState.unk1E0 == 3)
    {
        if (--gGameState.unk1CA == 0 || gMain.newKeys != 0)
        {
            Func_0201E2FC();
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
        Func_0201E2FC();
        if (gGameState.unk1FA == 0 && (gGameState.unk1CA & 0x20))
        {
            gGameState.unk1CA = 300;
            gGameState.unk1E0++;
        }
    }
}

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
