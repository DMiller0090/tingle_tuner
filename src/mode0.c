#include "global.h"
#include "main.h"
#include "game.h"
#include "sound.h"
#include "gba/syscall.h"

// Game mode 0 (gMain.mode starts at 0). MainMode0 runs one state per frame.

void Mode0State0(void);
void Mode0State1(void);
void Mode0State2(void);
void Mode0State3(void);

#ifndef VERSION_GZLJ01
// 0xFF-terminated byte string used by Mode0State0; not in the Japanese build.
const u8 gMode0UsString[] = {0x03, 0x1B, 0x26, 0x26, 0x23, 0x28, 0x21, 0x5F, 0x5F, 0x5F, 0xFF};
#endif

// Title sprite bob, indexed by (frameCount >> 4) & 3.
const s16 gMode0BobOffsets[4] = {0, -1, 0, 1};

void (*const gMode0StateFuncs[])(void) =
{
    Mode0State0,
    Mode0State1,
    Mode0State2,
    Mode0State3,
};

// Title screen: load the graphics, write the title text, fade from a blend (state 1).
void Mode0State0(void)
{
#ifdef VERSION_GZLJ01
    u16 *map = (u16 *)0x06006296;
#else
    u16 tile = 0x1388;
    u16 *map;
    s32 i;
#endif

    u8 blend;

    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x1100;  // BG0 and OBJ
    gMain.bgcnt[0] = 0x4C08;
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuFill16(0x109D, (void *)0x06006000, 0x800);
#ifdef VERSION_GZLJ01
    *map++ = 0x1188;
    *map++ = 0x1182;
    *map++ = 0x1181;
    *map++ = 0x1181;
    *map++ = 0x118F;
    *map++ = 0x1193;
    *map = 0x118E;
#else
    map = (u16 *)0x06006294;
    for (i = 9; i != 0; i--)
        *map++ = tile++;
    gGameState.unk14A = 6;
    gGameState.unk157 = 6;
    DrawString(gMode0UsString, 0, 0, 0);
#endif
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.modeState++;
    gMode0Work.blend = blend = 0;
    gMain.bldcnt = 0x2071;  // BG0 over BG0-3, OBJ and backdrop
    gMain.bldalpha = ((16 - blend) << 8) | blend;
    gMode0Work.timer = 1;
    gMode0Work.fadingOut = 0;
    gMode0Work.unk3 = 120;
    gGameState.unk034 = 0x20;
    gGameState.unk032 = -24;
    SoundPlay(SE_TITLE, 0);
}
// The title sprite scrolls right (every other frame) and bobs.
void Mode0DrawTitleSprite(void)
{
    s16 bob = gMode0BobOffsets[(gMain.frameCount & 0x30) >> 4];

    if (gMain.frameCount & 1)
    {
        if (++gGameState.unk032 > 0xFF)
            gGameState.unk032 = -24;
    }
    DrawSpriteAt(gSprite_0202C938, 0, gGameState.unk032, bob + gGameState.unk034, 0, 0, 0);
}
// Every other frame, step the title's blend (gMode0Work.blend, 1-16) toward the end it is
// heading for (gMode0Work.fadingOut); hold for 8 frames at full.
void Mode0UpdateBlend(void)
{
    if (--gMode0Work.timer == 0)
    {
        gMode0Work.timer = 2;
        if (gMode0Work.fadingOut == 0)
        {
            gMode0Work.blend++;
            if (gMode0Work.blend == 16)
            {
                gMode0Work.timer = 8;
                gMode0Work.fadingOut ^= 1;
            }
        }
        else
        {
            gMode0Work.blend--;
            if (gMode0Work.blend == 1)
                gMode0Work.fadingOut ^= 1;
        }
        gMain.bldalpha = ((16 - gMode0Work.blend) << 8) | gMode0Work.blend;
    }
    Mode0DrawTitleSprite();
}
INCLUDE_ASM(Mode0State1);
INCLUDE_ASM(Mode0State2);
INCLUDE_ASM(Mode0State3);

void MainMode0(void)
{
    gMode0StateFuncs[gMain.modeState]();
}
