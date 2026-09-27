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

void Func_0201F420(void)
{
    if (gMain.random & 1)
        GamePlaySound(0x3A, 14);
    else
        GamePlaySound(0x3B, 14);
    gGameState.unk1E2 = 0;
}

INCLUDE_ASM(Func_0201F460);

INCLUDE_ASM(Func_0201F878);

INCLUDE_ASM(Func_0201F8C4);

INCLUDE_ASM(Func_0201FA2C);

INCLUDE_ASM(Func_0201FCA0);

INCLUDE_ASM(Func_0201FD04);

void Func_0201FEEC(void)
{
    gGameState.unk132 = 8;
    gGameState.unk120 = 0;
    gGameState.unk12B = 4;
}

INCLUDE_ASM(Func_0201FF14);

void Func_02020150(void)
{
    GamePlaySound(0x50, 1);
    gGameState.unk1C8 = 300;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

INCLUDE_ASM(Func_02020190);

INCLUDE_ASM(Func_02020298);

// Link lost: load the title graphics with the "link lost" BG map and fade in (state 1).
void Mode2State0(void)
{
    u8 blend;

    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x1100;  // BG0 and OBJ
    gMain.bgcnt[0] = 0x4C08;
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    LZ77UnCompVram(gMode2BgMap, (void *)0x06006000);
#ifndef VERSION_GZLJ01
    Func_0201BAD8();
    gGameState.unk144 = MSG_DATA + *(u16 *)(MSG_DATA + *(u16 *)MSG_DATA + 0x3E);
    gGameState.unk14A = 6;
    gGameState.unk14B = 0;
    gGameState.unk157 = 6;
    gGameState.unk15A = 0;
    DrawString(gMode2UsString, 0, 0, 8);
#endif
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.hblankScroll = 0;
    gMode0Work.blend = blend = 0;
    gMain.bldcnt = 0x2071;  // BG0 over BG0-3, OBJ and backdrop
    gMain.bldalpha = ((16 - blend) << 8) | blend;
    gGameState.flagsReceived = 0;
    gGameState.unk23C = 1;
    gMode0Work.timer = 1;
    gMode0Work.fadingOut = 0;
    gMain.modeState++;
}

// Pulse the blend (between 6 and 16, holding 3 s at full) until the link is back, then
// restart mode 1.
void Mode2State1(void)
{
    Func_02021284(2, 2, 0);
    if (--gMode0Work.timer == 0)
    {
        gMode0Work.timer = 2;
        if (gMode0Work.fadingOut == 0)
        {
            gMode0Work.blend++;
            if (gMode0Work.blend == 16)
            {
                gMode0Work.timer = 180;
                gMode0Work.fadingOut ^= 1;
            }
        }
        else
        {
            gMode0Work.blend--;
            if (gMode0Work.blend == 6)
                gMode0Work.fadingOut ^= 1;
        }
        gMain.bldalpha = ((16 - gMode0Work.blend) << 8) | gMode0Work.blend;
    }
    if (gMain.linked)
    {
        gMain.mode = 1;
        gMain.modeState = 0;
    }
    DrawSpriteAt(gSprite_0202C848, 0, 0xD0, 0x80, 0, 0, 0);
}

void MainMode2(void)
{
    gMode2StateFuncs[gMain.modeState]();
}

// Slow vx toward 0 by decel (it can overshoot) and move x.
void ObjDecelX(struct Obj *obj, s16 decel)
{
    if (obj->vx >= 0)
        decel = -decel;
    if (obj->vx != 0)
        obj->vx += decel;
    obj->x += obj->vx;
}

// Slow vy toward 0 by decel (it can overshoot) and move y.
void ObjDecelY(struct Obj *obj, s16 decel)
{
    if (obj->vy >= 0)
        decel = -decel;
    if (obj->vy != 0)
        obj->vy += decel;
    obj->y += obj->vy;
}

// While in the air (z < 0 or moving), accelerate vz by gravity and land at z = 0.
void ObjFall(struct Obj *obj, s16 gravity)
{
    if (obj->vz | obj->z)
    {
        obj->vz += gravity;
        obj->z += obj->vz;
        if (obj->z >= 0)
        {
            obj->vz = 0;
            obj->z = 0;
        }
    }
}

void ObjReset(struct Obj *obj)
{
    obj->x = 0;
    obj->y = 0;
    obj->z = 0;
    obj->vx = 0;
    obj->vy = 0;
    obj->vz = 0;
    obj->unk12 = 0;
    obj->unk14 = 0;
    obj->unk16 = 0;
    obj->unk18 = 0;
    obj->unk1A = 0;
    obj->unk1C = 0;
    obj->unk1E = 0;
    obj->active = 0;
    obj->type = 0;
}

// Run every active object's type function.
void ObjUpdateAll(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            gObjFuncs[gObjs[i].type](&gObjs[i]);
    }
}

// Index of the first free object, or -1.
s32 ObjFindFree(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (!gObjs[i].active)
            return i;
    }
    return -1;
}

// Start a free object of the given type (its function sees active = 1 and sets it up).
bool32 ObjSpawn(u8 type)
{
    s32 i = ObjFindFree();

    if (i != -1)
    {
        gObjs[i].type = type;
        gObjs[i].active = 1;
        gObjFuncs[gObjs[i].type](&gObjs[i]);
        return TRUE;
    }
    return FALSE;
}

void ObjResetActive(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            ObjReset(&gObjs[i]);
    }
}

INCLUDE_ASM(Func_02020668);

INCLUDE_ASM(Func_020207D4);

INCLUDE_ASM(Func_0202081C);

// Fill a per-line BG0HOFS table: BG0's scroll plus a sine wave of the given amplitude (8.8),
// advancing the phase by 4 per line.
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude)
{
    s32 i;
    const s16 *sin;
    u16 hofs;

    i = 160;
    sin = gSinTable;
    hofs = *gMain.bgOfs[0];
    for (; i != 0; i--)
    {
        *table = hofs + ((sin[phase] * amplitude) >> 8);
        phase += 4;
        table++;
    }
}

INCLUDE_ASM(Func_020208FC);

void Func_020209A0(void)
{
    DrawSprite(gSprite_0202BD78, 0, gGameState.unk1E8, gGameState.unk1EA, 0, 0, 0, 0, 0, 0, 0);
}

INCLUDE_ASM(Func_020209E0);

void Func_02020BEC(void)
{
    gGameState.unk1E8 = 0xF8;
    gGameState.unk1EA = 0x50;
    gGameState.unk1EE = 0;
}

INCLUDE_ASM(Func_02020C10);
