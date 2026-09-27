#ifndef GUARD_GBA_COM_H
#define GUARD_GBA_COM_H

#include "global.h"

// App-side link API: one status byte and one receive buffer per slot. Slot numbers are
// the `type`/data manager index of m_Do_gba_com.cpp; the GC names are from d_a_agb.cpp,
// d_map.cpp and d_a_agbsw0.cpp. "->" is the direction the data travels.
enum
{
    GBACOM_SLOT_MESSAGE,    // GC -> GBA  msg_LZ.bin (daAgb_c::uploadConnect)
    GBACOM_SLOT_CONTROL,    // GC -> GBA  4 bytes: {0,1,0,0} (dMap_c), 1 (dMap_c::remove),
                            //            0x01000000 (daAgb_c::modeDelete)
    GBACOM_SLOT_MAP_IMAGE,  // GC -> GBA  room/island map (dMap_c; shares the message buffer)
    GBACOM_SLOT_3,
    GBACOM_SLOT_GBA_FLG,    // GBA -> GC  daAgb_GbaFlg (8 bytes)
    GBACOM_SLOT_MAIL,       // GC -> GBA  agb_mail_struct (8 bytes, daAgbsw0_c)
    GBACOM_SLOT_ITEM,       // GBA -> GC  daAgb_Item: item use request
    GBACOM_SLOT_EFFECT,     // GC -> GBA  item use result (daAgb_c::mEffect)
    GBACOM_SLOT_SWITCH,     // GBA -> GC  daAgb_Switch (8 bytes)
    GBACOM_SLOT_FLAGS,      // GC -> GBA  daAgb_Flags (12 bytes)
    GBACOM_SLOT_NAME,       // GC -> GBA  player name (8 bytes)
    GBACOM_SLOT_11,
    GBACOM_SLOT_SHOP,       // GBA -> GC  daAgb_Shop: purchase request
    GBACOM_SLOT_ITEM_BUY,   // GC -> GBA  purchase result (daAgb_c::mItemBuy)
    GBACOM_SLOT_14,
    GBACOM_SLOT_SE,         // GC -> GBA  sound effect id (daAgbsw0_c::mSE)
    GBACOM_SLOT_COUNT,
};

// gGbaComDataStatus values. Values the GC sends in a packet's status word overwrite the
// GBA's status for that slot, so 0/4/5/9 can also come from the GC side.
enum
{
    GBACOM_STATUS_IDLE = 0,
    GBACOM_STATUS_REQUEST = 1,      // GbaComRequestData; becomes 2 once sent
    GBACOM_STATUS_REQUESTED = 2,
    GBACOM_STATUS_RECEIVING = 3,    // GC is sending data for this slot
    GBACOM_STATUS_RECEIVED = 4,     // checksum OK, GbaComRecvDataGet may copy it
    GBACOM_STATUS_5 = 5,            // counts as "free to send"; only set by the GC
    GBACOM_STATUS_QUEUED = 6,       // JoyComSendDataSet
    GBACOM_STATUS_SENDING = 7,
    GBACOM_STATUS_SENT = 8,         // all words and the checksum sent
    GBACOM_STATUS_ERROR = 9,        // checksum mismatch (or GC closed the slot)
};

extern void *const gGbaComBuffers[GBACOM_SLOT_COUNT];
extern u8 gGbaComDataStatus[GBACOM_SLOT_COUNT];

void GbaComInit(void);
void GbaComStop(void);
bool32 GbaComMain(void);
u8 GbaComGetDataStatus(u8 slot);
bool32 GbaComCheckDataStatus(u8 slot, u8 status);
bool32 GbaComSendStatusCheck(u8 slot);
bool32 GbaComRecvStatusCheck(u8 slot);
void GbaComDataStatusReset(u8 slot);
void GbaComClearDataStatus(void);
bool32 GbaComRequestData(u8 slot);
bool32 GbaComStatusIs5(u8 slot);
bool32 GbaComSendDataSet(void *data, s32 size, u8 slot, u32 checkSum);
bool32 GbaComRecvDataGet(void *dest, u8 slot, s32 size);
bool32 GbaComIsClosed(void);

#endif // GUARD_GBA_COM_H
