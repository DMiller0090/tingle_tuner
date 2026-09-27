#include "global.h"
#include "game.h"
#include "gba/syscall.h"
#include "main.h"
#include "sound.h"

void srand(unsigned int seed);
int rand(void);

// Game mode 1 (the Tingle Tuner screen proper). MainMode1 runs one state per frame.

void Mode1State0(void);
void Mode1State1(void);
void Mode1State2(void);
void Mode1State3(void);
void Mode1State4(void);
void Mode1State5(void);

// Not referenced by any code.
const u8 gMode1Unused_02023E38[12] = {0x7D, 0x7A, 0x01, 0x00, 0xB2, 0xB3, 0x00, 0x2B, 0x00, 0x00, 0x00, 0xFF};

// Sound per dungeon number (gGameState.flags.dungeonNo), played by Mode1State1.
const s16 gDungeonSounds[18] = {50, 51, 52, 53, 54, 55, 82, 83, 84, 85, 86, 86, 138, 142, 135, 137, 141, 57};

void (*const gMode1StateFuncs[])(void) =
{
    Mode1State0,
    Mode1State1,
    Mode1State2,
    Mode1State3,
    Mode1State4,
    Mode1State5,
};

const u16 gMode1Unk_02023E80[3] = {0xC044, 0xA044, 0x4044};
const u16 gMode1Unk_02023E86[5] = {0xE6, 0xE7, 0xE8, 0xEA, 0xE9};

void InitDisplayIntr(void)
{
    REG_IME = 0;
    REG_DISPCNT = 0x80;  // forced blank
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IE |= INTR_FLAG_VBLANK;
    REG_IME = 1;
}

void SeedRandom(void)
{
    srand(1);
}

void UpdateRandom(void)
{
    gMain.random = rand();
}

// Step the rupee counter toward the real count and draw it (up to 4 digits, two tile
// rows, no leading zeros).
void Mode1Func_020150A4(void)
{
    u16 *map = (u16 *)0x06006450;
    s32 digits[4];
    s32 step;
    s32 i;
    s32 drawn;
    s32 shown;

    if (gGameState.rupeeDelta != 0)
    {
        step = 1;
        if (gGameState.rupeeDelta < 0)
            step = -1;
        gGameState.rupees += step;
        gGameState.rupeeDelta -= step;
        if (gMain.modeState == 2)
        {
            if (gGameState.rupeeDelta == 0)
                SoundPlay(SE_RUPEE_COUNT_END, 0);
            else if (gGameState.rupeeDelta & 1)
            {
                if (step < 0)
                    SoundPlay(SE_RUPEE_COUNT_DOWN, 0);
                else
                    SoundPlay(SE_RUPEE_COUNT_UP, 0);
            }
        }
    }
    digits[0] = (s16)(gGameState.rupees / 1000) * 2;
    digits[1] = (s16)((s16)(gGameState.rupees / 100) % 10) * 2;
    digits[2] = (s16)((s16)(gGameState.rupees / 10) % 10) * 2;
    digits[3] = (s16)(gGameState.rupees % 10) * 2;
    drawn = 0;
    shown = 0;
    for (i = 0; i < 4; i++)
    {
        if (i == 0)
        {
            if (digits[i] != 0)
            {
                map[0] = digits[i] + 0xB030;
                map[0x20] = digits[i] + 0xB031;
                map++;
                shown++;
                drawn++;
            }
        }
        else
        {
            if (digits[i] != 0 || i == 3 || shown != 0)
            {
                map[0] = digits[i] + 0xB030;
                map[0x20] = digits[i] + 0xB031;
                shown++;
            }
            else
            {
                map[0] = 0xB000;
                map[0x20] = 0xB000;
            }
            map++;
            drawn++;
        }
    }
    if (drawn != 4)
    {
        map[0] = 0xB000;
        map[0x20] = 0xB000;
    }
}
void Mode1Func_0201521C(void)
{
    if (gGameState.flags.dungeonNo == 0)
    {
        CpuCopy16((void *)0x06007B40, (void *)0x06006840, 0x80);
        Func_0201A570();
        LZ77UnCompVram(gMode1SeaMap, (void *)0x06002000);
        gMain.bgcnt[2] = 0xC409;
        gMain.bgcnt[3] = 0xC809;
    }
    else
    {
        gMain.bgcnt[3] = 0x0B41;
    }
}

void Mode1Func_02015298(void)
{
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1BgMap, (void *)0x06006000);
    Mode1Func_0201521C();
}
INCLUDE_ASM(Mode1State0);
INCLUDE_ASM(Mode1State1);
INCLUDE_ASM(Mode1State5);
INCLUDE_ASM(Mode1Func_02015840);
INCLUDE_ASM(Mode1State2);
INCLUDE_ASM(Mode1State3);
INCLUDE_ASM(Mode1Func_020160C8);
INCLUDE_ASM(Mode1Func_02016350);
INCLUDE_ASM(Mode1State4);
INCLUDE_ASM(Mode1Func_02016684);
INCLUDE_ASM(MainMode1);
