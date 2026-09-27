#ifndef GUARD_MAIN_H
#define GUARD_MAIN_H

#include "global.h"

// Main work area in IWRAM (0x03003080 in every version).
struct Main
{
    u8 filler0[0x44C];
    u8 unk44C;
};

extern struct Main gMain;

void InitSystem(void);
void MainLoop(void);

#endif // GUARD_MAIN_H
