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

void MsgCmd01(void)
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
