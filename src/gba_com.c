#include "global.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "joy_com.h"
#include "main.h"

// App-side link API: per-slot status checks and copies over JoyCom (docs/protocol.md).

void *const gGbaComBuffers[GBACOM_SLOT_COUNT] =
{
    [GBACOM_SLOT_MESSAGE]   = gGbaComMessageBuf,
    [GBACOM_SLOT_CONTROL]   = gGbaComBufControl,
    [GBACOM_SLOT_MAP_IMAGE] = gGbaComMessageBuf,
    [GBACOM_SLOT_MARKERS]         = gGbaComBufMarkers,
    [GBACOM_SLOT_GBA_FLG]   = &gGbaComBufGbaFlg,
    [GBACOM_SLOT_MAIL]      = gGbaComBufMail,
    [GBACOM_SLOT_ITEM]      = gGbaComBufItem,
    [GBACOM_SLOT_EFFECT]    = gGbaComBufEffect,
    [GBACOM_SLOT_SWITCH]    = gGbaComBufSwitch,
    [GBACOM_SLOT_FLAGS]     = gGbaComBufFlags,
    [GBACOM_SLOT_NAME]      = gGbaComBufName,
    [GBACOM_SLOT_VISITED]        = gGbaComBufVisited,
    [GBACOM_SLOT_SHOP]      = gGbaComBufShop,
    [GBACOM_SLOT_ITEM_BUY]  = gGbaComBufItemBuy,
    [GBACOM_SLOT_CHART_POINTS]        = gGbaComBufChartPoints,
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
    if (gJoyCom.active && JoyComMain() && gMain.mode != 2)
    {
        gMain.mode = 2;
        gMain.modeState = 0;
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

// Written counting up: agbcc reverses the loop into the original's pointer countdown.
void GbaComClearDataStatus(void)
{
    s32 i;

    for (i = 0; i < GBACOM_SLOT_COUNT; i++)
        gGbaComDataStatus[i] = GBACOM_STATUS_IDLE;
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
