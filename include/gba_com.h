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
    /*0x2*/ u8 windDir;         // field_0x2: wind direction (cM_atan2s of the wind >> 8), set while
                                //   showWind; UpdateMsgAndTingle turns the wind arrow by it
    /*0x3*/ u8 hasTact:1;       // field_0x3_7: dComIfGs_isTact(0)
            u8 gcInput:1;       // field_0x3_6: a button, the stick or the C stick is held on the GC
            u8 cursorInZone:1;  // field_0x3_5: the cursor is in a daAgbsw0 restriction-4 zone
            u8 dungeonChests:5; // field_0x3_0..4: isStageTbox(0xF) for WT, ET, TOTG, FW, DRC
    /*0x4*/ u8 timer;           // field_0x4: (field_0x65c + 29) / 30
    /*0x5*/ u8 walletSize:2;    // field_0x5_6: dComIfGs_getWalletSize()
            u8 moveSpeed:3;     // field_0x5_3: free cursor moving (1), else Link's speed 0-4 (daAgb_Execute);
                                //   Mode1State2 passes (moveSpeed - 1) ^ 3 to SoundPlayLoop
            u8 cursorHeld:1;    // field_0x5_2 = field_0x675: a daAgbsw0 caught the cursor (mail type 12;
                                //   MainMode1 sets it too)
            u8 hasCabanaDeed:1; // field_0x5_1: dComIfGs_checkGetItem(CABANA_DEED)
            u8 unk5_7:1;        // field_0x5_0 (never set)
    /*0x6*/ u8 hour:5;          // field_0x6_3: dKy_getdaytime_hour()
            u8 dayOfWeek:3;     // field_0x6_0: dKy_get_dayofweek()
    /*0x7*/ u8 minute:6;        // field_0x7_2: dKy_getdaytime_minute()
            u8 balloonDiscount:1;  // field_0x7_1: UNLOCK_TINGLE_BALLOON_DISCOUNT (GameState.balloonDiscount)
            u8 tingDiscount:1;  // field_0x7_0: UNLOCK_TING_DISCOUNT (GameState.tingDiscount)
    /*0x8*/ u8 life:7;          // field_0x8_1: dComIfGs_getLife()
            u8 miniboss:1;      // field_0x8_0: the stage type is miniboss
    /*0x9*/ u8 pearlToFind:3;   // field_0x9_7..5: bits Nayru, Din, Farore; the one to find next
                                //   (none before Korl or after all three)
            u8 pearlsToPlace:3; // field_0x9_4..2: Din's, Farore's, Nayru's pearl not placed yet
            u8 bossesToBeat:2;  // field_0x9_1..0: WT, ET boss not beaten yet
    /*0xA*/ u8 noMap:1;         // field_0xa_7: 0 only in a dungeon, FF1, miniboss or sea stage once
                                //   MapNoSet ran; 1 keeps the GBA on the Mode1State4 wait screen
            u8 busy:2;          // bit 0: field_0xa_6 dMenu_flag(), bit 1: field_0xa_5
            //                     dComIfGp_event_runCheck() (the GBA reads them as one field)
            u8 restarting:1;    // field_0xa_4: dStage_checkRestart() or player status 0x20000000
            u8 towerRaised:1;   // field_0xa_3: event bit 0x1E40; the sea chart shows square 25
                                //   (Tower of the Gods) only once it is set
            u8 unkA_5:1;        // field_0xa_2 (never set)
            u8 shopUnlocked:1;  // field_0xa_1: event bit 0x1708 (GameState.shopUnlocked)
            u8 isFree:1;        // field_0xa_0: daAgb_c::mIsFree
    /*0xB*/ u8 followTarget:1;  // field_0xb_7
            u8 showWind:1;      // field_0xb_6: dKyw_gbwind_use_check(); windDir is valid
            u8 dungeonNo:5;     // field_0xb_3: DungeonNoGet()
            u8 hasMagic:1;      // field_0xb_0: dComIfGs_getMaxMagic() != 0
};

// What the GBA sends in slot 4 (daAgb_GbaFlg, read by daAgb_c::FlagsRecv).
struct GbaComGbaFlg
{
    /*0x0*/ u16 keys;           // field_0x0
    /*0x2*/ u8 bits;            // field_0x2 (bitfield byte on the GC): GameState byte 0x138
    /*0x3*/ u8 active;          // field_0x3 -> daAgb_c::field_0x673 (the GC's isActive() needs it)
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

// The game code addresses the buffers by constant address rather than by symbol: agbcc
// then loads each address it needs (buf, buf + 1, ...) from the pool, where with a symbol
// it would derive buf + 1 from buf.
#define GBACOM_BUF_3      ((u8 *)(PROG_BASE - 0x800))
#define GBACOM_BUF_GBA_FLG ((u8 *)(PROG_BASE - 0x700))
#define GBACOM_BUF_MAIL   ((u8 *)(PROG_BASE - 0x6F0))
#define GBACOM_BUF_SWITCH ((u8 *)(PROG_BASE - 0x6E0))
#define GBACOM_BUF_FLAGS  ((u8 *)(PROG_BASE - 0x6A0))
#define GBACOM_BUF_11     ((u8 *)(PROG_BASE - 0x680))
#define GBACOM_BUF_SE     ((u8 *)(PROG_BASE - 0x550))

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
