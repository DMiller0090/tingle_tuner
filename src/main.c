#include "global.h"
#include "main.h"

void AgbMain(void)
{
    InitSystem();
    gMain.unk44C = TRUE;
    MainLoop();
}
