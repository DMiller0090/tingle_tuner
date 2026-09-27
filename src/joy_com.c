#include "global.h"
#include "crt0.h"
#include "gba/syscall.h"
#include "gba_com.h"
#include "heap.h"
#include "joy_com.h"
#include "main.h"

// JOY bus link with the GameCube (docs/protocol.md). JoyComIntr runs from a copy in
// gMain.heap; JoyComInit copies everything from JoyComIntr up to JoyComMain.

void JoyComInit(void *const *buffers, u8 *status, u32 gameCode, u8 numSlots)
{
    u32 size;
    u8 *intrCopy;
    s32 i, n;

    CpuFill32(0, &gJoyCom, sizeof(gJoyCom));
    gJoyCom.active = TRUE;
    gJoyCom.buffers = buffers;
    gJoyCom.status = status;
    gJoyCom.gameCode = gameCode;
    gJoyCom.numSlots = numSlots;
    n = numSlots;
    for (i = 0; i < n; i++)
        *status++ = GBACOM_STATUS_IDLE;

    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_SERIAL;
    intrCopy = HeapAlloc(&gMain.heap.heap, size = (u32)JoyComMain - (u32)JoyComIntr);
    gJoyCom.intrCopy = intrCopy;
    if (intrCopy != NULL)
    {
        CpuFastCopy(JoyComIntr, intrCopy, size);
        gIntrTable[0] = (IntrFunc)(intrCopy + 1);  // Thumb
    }
    REG_IME = 1;

    REG_JOY_TRANS = JOYCOM_IDLE;
    REG_RCNT = RCNT_JOY_BUS;
    REG_JOYCNT = JOYCNT_INTR | JOYCNT_ACK_ALL;
    REG_IF = INTR_FLAG_SERIAL;
    REG_IME = 0;
    REG_IE |= INTR_FLAG_SERIAL;
    REG_IME = 1;
}

void JoyComStop(void)
{
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_SERIAL;
    REG_IME = 1;
    if (gJoyCom.intrCopy != NULL)
    {
        REG_IME = 0;
        gIntrTable[0] = JoyComIntr;
        REG_IME = 1;
        HeapFree(&gMain.heap.heap, gJoyCom.intrCopy);
        gJoyCom.intrCopy = NULL;
    }
    REG_RCNT = 0;
    REG_JOYCNT = JOYCNT_ACK_ALL;
    REG_JOY_TRANS = 0;
    gJoyCom.active = FALSE;
    gJoyCom.state = JOYCOM_STATE_IDLE;
}

INCLUDE_ASM(JoyComIntr);

bool32 JoyComMain(void)
{
    if (gJoyCom.hardReset)
    {
        JoyHardReset();
        JoyComStop();
        return TRUE;
    }
    if (gJoyCom.state == JOYCOM_STATE_LINKED)
    {
        if (gJoyCom.idleFrames > 9)
        {
            // The GC went quiet: restart the link, keeping the context so it can reconnect.
            void *intrCopy;
            u32 context;
            void *const *buffers;
            u8 *status;
            u32 gameCode;
            u8 numSlots;
            u8 hasContext;
            s32 i, n;

            REG_IME = 0;
            REG_IE &= ~INTR_FLAG_SERIAL;
            REG_IME = 1;
            REG_RCNT = 0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG0;
            REG_JOYSTAT &= ~JOYSTAT_GP_FLAG1;
            REG_JOY_TRANS = JOYCOM_IDLE;
            REG_JOYCNT = JOYCNT_INTR | JOYCNT_ACK_ALL;
            REG_RCNT = RCNT_JOY_BUS;

            intrCopy = gJoyCom.intrCopy;
            context = gJoyCom.context;
            buffers = gJoyCom.buffers;
            status = gJoyCom.status;
            gameCode = gJoyCom.gameCode;
            numSlots = gJoyCom.numSlots;
            hasContext = gJoyCom.hasContext;
            CpuFill32(0, &gJoyCom, sizeof(gJoyCom));
            gJoyCom.active = TRUE;
            gJoyCom.intrCopy = intrCopy;
            gJoyCom.context = context;
            gJoyCom.buffers = buffers;
            gJoyCom.status = status;
            gJoyCom.gameCode = gameCode;
            gJoyCom.numSlots = numSlots;
            gJoyCom.hasContext = hasContext;
            n = numSlots;
            for (i = 0; i < n; i++)
                *status++ = GBACOM_STATUS_IDLE;

            REG_IF = INTR_FLAG_SERIAL;
            REG_IME = 0;
            REG_IE |= INTR_FLAG_SERIAL;
            REG_IME = 1;
            gJoyCom.idleFrames = 0;
            return TRUE;
        }
        gJoyCom.idleFrames++;
    }
    return FALSE;
}

bool32 JoyComSendDataSet(void *data, s32 size, u8 type, u32 checkSum)
{
    u32 i;

    if (gJoyCom.queueCount >= JOYCOM_QUEUE_LEN || size <= 0)
        return FALSE;
    if (size < 4)
        size = 4;
    i = gJoyCom.queueWrite;
    gJoyCom.queue[i].data = data;
    gJoyCom.queue[i].size = size;
    gJoyCom.queue[i].type = type;
    gJoyCom.queue[i].checkSum = checkSum;
    gJoyCom.queueWrite++;
    gJoyCom.queueCount++;
    gJoyCom.status[type] = GBACOM_STATUS_QUEUED;
    return TRUE;
}

bool32 JoyComStatusSendSet(u8 type)
{
    u32 i;

    if (gJoyCom.queueCount >= JOYCOM_QUEUE_LEN)
        return FALSE;
    i = gJoyCom.queueWrite;
    gJoyCom.queue[i].data = NULL;
    gJoyCom.queue[i].size = 0;
    gJoyCom.queue[i].type = type;
    gJoyCom.queue[i].checkSum = 0;
    gJoyCom.queueWrite++;
    gJoyCom.queueCount++;
    return TRUE;
}
