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

INCLUDE_ASM(Func_0201C660);

INCLUDE_ASM(Func_0201C704);

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

INCLUDE_ASM(Func_0201CB10);

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

INCLUDE_ASM(Func_0201DA78);

INCLUDE_ASM(Func_0201DB50);

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
