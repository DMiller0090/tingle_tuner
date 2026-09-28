#include "global.h"
#include "crt0.h"
#include "game.h"
#include "gba/dma.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "heap.h"
#include "joy_com.h"
#include "main.h"
#include "sound.h"

const IntrFunc gIntrTableInit[14] =
{
    JoyComIntr,  // serial
    VBlankIntr,
    IntrDummy,   // HBlank
    IntrDummy,   // VCount
    IntrDummy,   // DMA0
    IntrDummy,   // DMA1
    IntrDummy,   // DMA2
    IntrDummy,   // DMA3
    IntrDummy,   // keypad
    IntrDummy,   // cartridge
    IntrDummy,
    IntrDummy,
    IntrDummy,
    IntrDummy,
};

const struct OamPair gOamHiddenEntry = {160, 0};

void (*const gMainModeFuncs[])(void) =
{
    MainMode0,
    MainMode1,
    MainMode2,
};

void AgbMain(void)
{
    InitSystem();
    gMain.dispCopied = TRUE;
    MainLoop();
}

void VBlankIntr(void)
{
    s32 shake = 0;
    u16 vofs = 0;

    DmaStop(0);
    SoundVSync();
    if (gMain.soundDmaStop)
    {
        SoundDmaStop();
        gMain.soundDmaStop = FALSE;
    }
    if (!gMain.dispCopied)
    {
        gMain.dispCopied = TRUE;
        gMain.frameCount++;
        if (gGameState.shake)
            shake = (gGameState.shake >> 1) & 1;
        if (gMain.mode == 1 && !gMain.mapImageMode)
            Mode1VBlank();
        CpuFastCopy(gOamBuffer, (void *)OAM, sizeof(gOamBuffer));
        REG_DISPCNT = gMain.dispcnt;
        REG_BG0CNT = gMain.bgcnt[0];
        REG_BG1CNT = gMain.bgcnt[1];
        REG_BG2CNT = gMain.bgcnt[2];
        REG_BG3CNT = gMain.bgcnt[3];
        REG_MOSAIC = gMain.mosaic;
        REG_BG0HOFS = gMain.bgOfs[0][0];
        vofs = gMain.bgOfs[0][1] + shake;
        REG_BG0VOFS = vofs;
        REG_BG1HOFS = gMain.bgOfs[1][0];
        vofs = gMain.bgOfs[1][1] + shake;
        REG_BG1VOFS = vofs;
        REG_BG2HOFS = gMain.bgOfs[2][0];
        vofs = gMain.bgOfs[2][1] + shake;
        REG_BG2VOFS = vofs;
        REG_BG3HOFS = gMain.bgOfs[3][0];
        vofs = gMain.bgOfs[3][1] + shake;
        REG_BG3VOFS = vofs;
        REG_WIN0H = gMain.win0h;
        REG_WIN1H = gMain.win1h;
        REG_WIN0V = gMain.win0v;
        REG_WIN1V = gMain.win1v;
        REG_WININ = gMain.winin;
        REG_WINOUT = gMain.winout;
        REG_BLDCNT = gMain.bldcnt;
        REG_BLDALPHA = gMain.bldalpha;
        REG_BLDY = gMain.bldy;
        gGameState.scrollTable = gGameState.scrollTableNext;
    }
    if (gMain.hblankScroll)
    {
        // One BG0HOFS value per line, starting with line 1.
        DmaSet(0, &gBgScrollTables[gGameState.scrollTable][1], &REG_BG0HOFS,
               DMA_ENABLE | DMA_START_HBLANK | DMA_REPEAT | DMA_DEST_RELOAD | DMA_16BIT | 1);
    }
    else
    {
        REG_DMA0CNT = 0;
    }
    SoundVSyncProc();
    REG_IF |= INTR_FLAG_VBLANK;
    // FAKEMATCH: this self-assignment (found with decomp-permuter) emits nothing, but it
    // makes reload take r2, not r1, for the 0x414 offset below. Its load and store survive
    // until reload (the .greg shows an extra "Spilling reg 1" for them, before the 0x414
    // reload's) and are dropped afterwards, so the original needs something else there that
    // occupies r1 during reload and then vanishes. Priorities and hard-reg use counts don't
    // explain it (r1 has more uses than r2 either way).
    gGameState.scrollTableNext = gGameState.scrollTableNext;
    gMain.intrCheck = INTR_FLAG_VBLANK;
}

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
    HeapInit(&gMain.heap.heap, sizeof(gMain.heap));
#else
    HeapInit(&gMain.heap.heap, sizeof(gMain.heap));
    GbaComInit();
#endif
    gMain.dispCopied = FALSE;
    SeedRandom();
    CpuCopy16(__data_lma_start, (void *)IWRAM_START, __data_lma_end - __data_lma_start);
    SoundInit();
    CpuFill16(0, gMain.soundParams, sizeof(gMain.soundParams));
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
    gMain.oamCount = 0;
    gMain.affineCount = 0;
}

// The buffer fields are written through byte offsets from the constant buffer addresses:
// that makes agbcc fold each address into one pool constant and derive the next from it, as
// the original does (struct field access gives base + offset instead).
#ifdef VERSION_PAL
#define GAMESTATE_BITS_OFS 0x140  // the byte of gGameState.unk138_0 ... (GbaFlg.bits)
#else
#define GAMESTATE_BITS_OFS 0x138
#endif

void MainLoop(void)
{
    // The original reserves 4 bytes of stack it never touches: most likely a leftover local
    // buffer (agbcc only gives stack to unused aggregates, never to unused scalars).
    u8 unused[4];

    while (TRUE)
    {
        gMain.linked = GbaComMain();
        if (gMain.linked)
        {
            *(u8 *)(GBACOM_BUF_GBA_FLG + 2) = *((u8 *)&gGameState + GAMESTATE_BITS_OFS);
            *(u16 *)(GBACOM_BUF_GBA_FLG + 4) = gMain.bgOfs[3][0] + 8;
            *(u16 *)(GBACOM_BUF_GBA_FLG + 6) = gMain.bgOfs[3][1] + 8;
            *(u16 *)(GBACOM_BUF_GBA_FLG + 0) = gMain.heldKeys;
            if (gGameState.inMessage == 0 && gGameState.selectWinOpen == 0 && gGameState.chartOpen == 0
             && gGameState.menuOpen == 0 && gMain.modeState == 2)
                *(u8 *)(GBACOM_BUF_GBA_FLG + 3) = TRUE;
            else
                *(u8 *)(GBACOM_BUF_GBA_FLG + 3) = FALSE;
            GbaComSendDataSet(GBACOM_BUF_GBA_FLG, sizeof(gGbaComBufGbaFlg), GBACOM_SLOT_GBA_FLG, 0);
        }
        gGameState.timer = gGameState.flags.timer;
        gGameState.dungeonChests = gGameState.flags.dungeonChests;
        gGameState.isFree = gGameState.flags.isFree;
        gGameState.hasMagic = gGameState.flags.hasMagic;
        if (GbaComRecvStatusCheck(GBACOM_SLOT_FLAGS))
        {
            // Ignore an all-zero packet.
            if (*(u32 *)(GBACOM_BUF_FLAGS + 0) | *(u32 *)(GBACOM_BUF_FLAGS + 4) | *(u32 *)(GBACOM_BUF_FLAGS + 8))
            {
                GbaComRecvDataGet(&gGameState.flags, GBACOM_SLOT_FLAGS, sizeof(gGameState.flags));
                gGameState.flagsReceived = TRUE;
            }
            else
            {
                GbaComDataStatusReset(GBACOM_SLOT_FLAGS);
            }
        }
        gMain.inDungeon = gGameState.flags.dungeonNo != 0;
        gGameState.rupeeDelta = gGameState.flags.rupees - gGameState.rupees;
        if (gGameState.flags.busy)
            gGameState.active = 0;
        ReadKeys();
        InitOamBuffer();
        UpdateRandom();
        gMainModeFuncs[gMain.mode]();
        gMain.dispCopied = FALSE;
        SoundMain();
        gMain.dispCopied = FALSE;
        gMain.intrCheck &= ~INTR_FLAG_VBLANK;
        while (gMain.intrCheck == 0)
            ;
        gMain.intrCheck &= ~INTR_FLAG_VBLANK;
    }
}

