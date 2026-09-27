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

// What the GC's daAgb_c::FlagsSend puts in slot 9 (daAgb_Flags, 12 bytes). The GC's
// bitfields are big-endian (first member = bit 7) and the data words cross the link
// unswapped, so each byte's fields appear here in reverse order. GC names in comments.
struct GbaComFlags
{
    /*0x0*/ u16 rupees;         // field_0x0 (the GC byte-swaps it for us)
    /*0x2*/ u8 unk2;            // field_0x2
    /*0x3*/ u8 hasTact:1;       // field_0x3_7: dComIfGs_isTact(0)
            u8 unk3_1:1;        // field_0x3_6
            u8 unk3_2:1;        // field_0x3_5
            u8 dungeonChests:5; // field_0x3_0..4: isStageTbox(0xF) for WT, ET, TOTG, FW, DRC
    /*0x4*/ u8 timer;           // field_0x4: (field_0x65c + 29) / 30
    /*0x5*/ u8 unk5;
    /*0x6*/ u8 unk6;
    /*0x7*/ u8 unk7;
    /*0x8*/ u8 unk8;
    /*0x9*/ u8 unk9;
    /*0xA*/ u8 unkA_0:1;        // field_0xa_7
            u8 menuOpen:1;      // field_0xa_6: dMenu_flag()
            u8 eventRunning:1;  // field_0xa_5: dComIfGp_event_runCheck()
            u8 unkA_3:1;        // field_0xa_4
            u8 unkA_4:1;        // field_0xa_3
            u8 unkA_5:1;        // field_0xa_2
            u8 unkA_6:1;        // field_0xa_1
            u8 isFree:1;        // field_0xa_0: daAgb_c::mIsFree
    /*0xB*/ u8 followTarget:1;  // field_0xb_7
            u8 unkB_1:1;        // field_0xb_6
            u8 dungeonNo:5;     // field_0xb_3: DungeonNoGet()
            u8 unkB_7:1;        // field_0xb_0
};

// What the GBA sends in slot 4 (daAgb_GbaFlg, read by daAgb_c::FlagsRecv).
struct GbaComGbaFlg
{
    /*0x0*/ u16 keys;           // field_0x0
    /*0x2*/ u8 unk2;            // field_0x2 (bitfield byte on the GC)
    /*0x3*/ u8 unk3;            // field_0x3 -> daAgb_c::field_0x673
    /*0x4*/ u16 x;              // field_0x4 -> field_0x630
    /*0x6*/ u16 y;              // field_0x6 -> field_0x632
};

// Receive buffers in EWRAM just below the program. The message and map image slots share one.
extern u8 gGbaComMessageBuf[0xA000];
extern u8 gGbaComBuf3[0x100];
extern struct GbaComGbaFlg gGbaComBufGbaFlg;
extern u8 gGbaComBufMail[0x10];
extern u8 gGbaComBufSwitch[0x10];
extern u8 gGbaComBufControl[0x10];
extern u8 gGbaComBufItem[0x10];
extern u8 gGbaComBufEffect[0x10];
extern u32 gGbaComBufFlags[4];      // struct GbaComFlags
extern u8 gGbaComBufName[0x10];
extern u8 gGbaComBuf11[0x10];
extern u8 gGbaComBuf14[0x100];
extern u8 gGbaComBufShop[0x10];
extern u8 gGbaComBufItemBuy[0x10];
extern u8 gGbaComBufSE[0x10];

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
