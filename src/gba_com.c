#include "global.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "joy_com.h"
#include "main.h"

// App-side link API: per-slot status checks and copies over JoyCom (docs/protocol.md).

// Receive buffers in EWRAM just below the program. The message and map image slots share one.
extern u8 gGbaComMessageBuf[0xA000];
extern u8 gGbaComBuf3[0x100];
extern u8 gGbaComBufGbaFlg[0x10];
extern u8 gGbaComBufMail[0x10];
extern u8 gGbaComBufSwitch[0x10];
extern u8 gGbaComBufControl[0x10];
extern u8 gGbaComBufItem[0x10];
extern u8 gGbaComBufEffect[0x10];
extern u8 gGbaComBufFlags[0x10];
extern u8 gGbaComBufName[0x10];
extern u8 gGbaComBuf11[0x10];
extern u8 gGbaComBuf14[0x100];
extern u8 gGbaComBufShop[0x10];
extern u8 gGbaComBufItemBuy[0x10];
extern u8 gGbaComBufSE[0x10];

void *const gGbaComBuffers[GBACOM_SLOT_COUNT] =
{
    [GBACOM_SLOT_MESSAGE]   = gGbaComMessageBuf,
    [GBACOM_SLOT_CONTROL]   = gGbaComBufControl,
    [GBACOM_SLOT_MAP_IMAGE] = gGbaComMessageBuf,
    [GBACOM_SLOT_3]         = gGbaComBuf3,
    [GBACOM_SLOT_GBA_FLG]   = gGbaComBufGbaFlg,
    [GBACOM_SLOT_MAIL]      = gGbaComBufMail,
    [GBACOM_SLOT_ITEM]      = gGbaComBufItem,
    [GBACOM_SLOT_EFFECT]    = gGbaComBufEffect,
    [GBACOM_SLOT_SWITCH]    = gGbaComBufSwitch,
    [GBACOM_SLOT_FLAGS]     = gGbaComBufFlags,
    [GBACOM_SLOT_NAME]      = gGbaComBufName,
    [GBACOM_SLOT_11]        = gGbaComBuf11,
    [GBACOM_SLOT_SHOP]      = gGbaComBufShop,
    [GBACOM_SLOT_ITEM_BUY]  = gGbaComBufItemBuy,
    [GBACOM_SLOT_14]        = gGbaComBuf14,
    [GBACOM_SLOT_SE]        = gGbaComBufSE,
};

void GbaComInit(void)
{
    JoyComInit(gGbaComBuffers, gGbaComDataStatus, gMain.gameCode, GBACOM_SLOT_COUNT);
}

void GbaComStop(void)
{
    JoyComStop();
}

bool32 GbaComMain(void)
{
    if (gJoyCom.active && JoyComMain() && gMain.unk452 != 2)
    {
        gMain.unk452 = 2;
        gMain.unk453 = 0;
    }
    return gJoyCom.active && gJoyCom.state == JOYCOM_STATE_LINKED;
}

u8 GbaComGetDataStatus(u8 slot)
{
    return gGbaComDataStatus[slot];
}

bool32 GbaComCheckDataStatus(u8 slot, u8 status)
{
    if (gGbaComDataStatus[slot] == status)
        return TRUE;
    return FALSE;
}

bool32 GbaComSendStatusCheck(u8 slot)
{
    u8 status = gGbaComDataStatus[slot];

    if (status == GBACOM_STATUS_IDLE || status == GBACOM_STATUS_5)
        return TRUE;
    return FALSE;
}

bool32 GbaComRecvStatusCheck(u8 slot)
{
    if (gGbaComDataStatus[slot] == GBACOM_STATUS_RECEIVED)
        return TRUE;
    return FALSE;
}

void GbaComDataStatusReset(u8 slot)
{
    if (gGbaComDataStatus[slot] == GBACOM_STATUS_RECEIVED)
    {
        gGbaComDataStatus[slot] = GBACOM_STATUS_IDLE;
        JoyComStatusSendSet(slot);
    }
}

// FAKEMATCH? The original is probably an index loop that GCC strength-reduced to this
// pointer loop (signed compare), but agbcc doesn't reduce any index form tried so far.
void GbaComClearDataStatus(void)
{
    u8 *status = gGbaComDataStatus;
    u8 idle = GBACOM_STATUS_IDLE;
    u8 *p = status + GBACOM_SLOT_COUNT - 1;

    do
    {
        *p = idle;
        p--;
    } while ((s32)p >= (s32)status);
}

bool32 GbaComRequestData(u8 slot)
{
    if (gGbaComDataStatus[slot] == GBACOM_STATUS_IDLE)
    {
        gGbaComDataStatus[slot] = GBACOM_STATUS_REQUEST;
        if (JoyComStatusSendSet(slot))
            return TRUE;
        gGbaComDataStatus[slot] = GBACOM_STATUS_IDLE;
    }
    return FALSE;
}

bool32 GbaComStatusIs5(u8 slot)
{
    if (gGbaComDataStatus[slot] == GBACOM_STATUS_5)
        return TRUE;
    return FALSE;
}

bool32 GbaComSendDataSet(void *data, s32 size, u8 slot, u32 checkSum)
{
    if (GbaComSendStatusCheck(slot))
        return JoyComSendDataSet(data, size, slot, checkSum);
    return FALSE;
}

bool32 GbaComRecvDataGet(void *dest, u8 slot, s32 size)
{
    u8 *status = &gGbaComDataStatus[slot];

    if (*status == GBACOM_STATUS_RECEIVED)
    {
        CpuCopy16(gGbaComBuffers[slot], dest, size);
        *status = GBACOM_STATUS_IDLE;
        JoyComStatusSendSet(slot);
        return TRUE;
    }
    return FALSE;
}

bool32 GbaComIsClosed(void)
{
    return gJoyCom.closed;
}
