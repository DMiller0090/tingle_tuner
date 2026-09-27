#include "global.h"
#include "main.h"

// Game mode 0 (gMain.mode starts at 0). MainMode0 runs one state per frame.

void Mode0State0(void);
void Mode0State1(void);
void Mode0State2(void);
void Mode0State3(void);

#ifndef VERSION_GZLJ01
// 0xFF-terminated byte string used by Mode0State0; not in the Japanese build.
const u8 gMode0UsString[] = {0x03, 0x1B, 0x26, 0x26, 0x23, 0x28, 0x21, 0x5F, 0x5F, 0x5F, 0xFF};
#endif

const s16 gMode0Unk_02023EE4[4] = {0, -1, 0, 1};

void (*const gMode0StateFuncs[])(void) =
{
    Mode0State0,
    Mode0State1,
    Mode0State2,
    Mode0State3,
};

INCLUDE_ASM(Mode0State0);
INCLUDE_ASM(Mode0Func_020182B0);
INCLUDE_ASM(Mode0Func_02018324);
INCLUDE_ASM(Mode0State1);
INCLUDE_ASM(Mode0State2);
INCLUDE_ASM(Mode0State3);

void MainMode0(void)
{
    gMode0StateFuncs[gMain.modeState]();
}
