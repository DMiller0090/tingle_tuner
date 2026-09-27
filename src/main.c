#include "global.h"
#include "crt0.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "heap.h"
#include "main.h"
#include "sound.h"

void AgbMain(void)
{
    InitSystem();
    gMain.unk44C = TRUE;
    MainLoop();
}

INCLUDE_ASM(VBlankIntr);

void DummyCallback_02014CA0(void)
{
}

void DummyCallback_02014CA4(void)
{
}

void IntrDummy(void)
{
}

void ReadKeys(void)
{
    u16 keys = REG_KEYINPUT ^ KEYS_MASK;

    gMain.newKeys = keys & ~gMain.heldKeys;
    gMain.heldKeys = keys;
}

void InitSystem(void)
{
    REG_IME = 0;
    REG_IE = 0;
    REG_RCNT = 0;
    REG_IME = 1;
    CpuFill32(0, (void *)IWRAM_START, 0x7B00);
    REG_WAITCNT = 0x4014;
    CpuCopy16(intr_main, gMain.intrMain, sizeof(gMain.intrMain));
    CpuCopy16(gIntrTableInit, gIntrTable, sizeof(gIntrTable));
    INTR_VECTOR = gMain.intrMain;
    gMain.gameCode = *(u32 *)0x020000AC;  // the packer's header game code
#ifdef VERSION_GZLJ01
    GbaComInit();
    HeapInit(gMain.heap, sizeof(gMain.heap));
#else
    HeapInit(gMain.heap, sizeof(gMain.heap));
    GbaComInit();
#endif
    gMain.unk44C = FALSE;
    SeedRandom();
    CpuCopy16(__data_lma_start, (void *)IWRAM_START, __data_lma_end - __data_lma_start);
    SoundInit();
    CpuFill16(0, gMain.unk40C, sizeof(gMain.unk40C));
    REG_IME = 0;
    REG_IE |= INTR_FLAG_VBLANK;
    REG_DISPSTAT = DISPSTAT_VBLANK_INTR;
    REG_IME = 1;
}

void InitOamBuffer(void)
{
    struct OamPair entry = gOamHiddenEntry;
    struct OamPair *p = gOamBuffer;
    struct OamPair *end = gOamBuffer + ARRAY_COUNT(gOamBuffer);

    while (p < end)
    {
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
        *p++ = entry;
    }
    gMain.unk450 = 0;
    gMain.unk451 = 0;
}

INCLUDE_ASM(MainLoop);
