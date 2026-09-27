#include "global.h"
#include "main.h"
#include "game.h"
#include "gba/syscall.h"
#include "sound.h"
#include "gba_com.h"

// Game code after heap.c, up to the sound driver (GZLJ01 02018884-02020E28). Not split into
// objects yet: see docs/layout.md for the groups the .rodata order implies.

INCLUDE_ASM(Func_02018884);

INCLUDE_ASM(Func_02018944);

INCLUDE_ASM(Func_02018988);

INCLUDE_ASM(StepToward);

INCLUDE_ASM(Func_02018A38);

INCLUDE_ASM(Func_02018A90);

INCLUDE_ASM(Func_02018AA4);

INCLUDE_ASM(DrawSprite);

INCLUDE_ASM(DrawSpriteAt);

INCLUDE_ASM(Func_020190A8);

INCLUDE_ASM(Func_0201920C);

INCLUDE_ASM(Func_020192D8);

INCLUDE_ASM(Func_02019350);

INCLUDE_ASM(Func_02019510);

INCLUDE_ASM(Func_02019CAC);

INCLUDE_ASM(Func_02019F5C);

INCLUDE_ASM(Func_0201A0A8);

INCLUDE_ASM(Func_0201A224);

INCLUDE_ASM(Func_0201A540);

INCLUDE_ASM(Func_0201A570);

INCLUDE_ASM(Func_0201A69C);

INCLUDE_ASM(Func_0201A6D4);

INCLUDE_ASM(FindMarker);

INCLUDE_ASM(Func_0201A8AC);

INCLUDE_ASM(Func_0201A944);

INCLUDE_ASM(Mode1VBlank);

INCLUDE_ASM(Func_0201AC94);

INCLUDE_ASM(Func_0201ACD0);

INCLUDE_ASM(Func_0201ACF8);

INCLUDE_ASM(Func_0201AD24);

INCLUDE_ASM(Func_0201AD4C);

INCLUDE_ASM(Func_0201AD90);

INCLUDE_ASM(Func_0201AF48);

INCLUDE_ASM(Func_0201AFCC);

INCLUDE_ASM(Func_0201B0F4);

INCLUDE_ASM(Func_0201B2C0);

INCLUDE_ASM(Func_0201B3A4);

INCLUDE_ASM(Func_0201B7D4);

INCLUDE_ASM(Func_0201B8C8);

INCLUDE_ASM(Func_0201BA2C);

INCLUDE_ASM(Func_0201BA9C);

INCLUDE_ASM(Func_0201BAD8);

INCLUDE_ASM(Func_0201BAF8);

INCLUDE_ASM(Func_0201BC0C);

INCLUDE_ASM(Func_0201BCC0);

INCLUDE_ASM(Func_0201BDE4);

INCLUDE_ASM(Func_0201BEA0);

INCLUDE_ASM(Func_0201C0E8);

INCLUDE_ASM(Func_0201C164);

INCLUDE_ASM(Func_0201C204);

INCLUDE_ASM(Func_0201C484);

INCLUDE_ASM(Func_0201C568);

INCLUDE_ASM(Func_0201C60C);

INCLUDE_ASM(Func_0201C660);

INCLUDE_ASM(Func_0201C704);

#ifndef VERSION_GZLJ01
INCLUDE_ASM(UsFunc_02023390);
#endif

INCLUDE_ASM(Func_0201C86C);

INCLUDE_ASM(Func_0201C8D8);

INCLUDE_ASM(Func_0201C948);

INCLUDE_ASM(Func_0201C988);

INCLUDE_ASM(Func_0201C9E4);

INCLUDE_ASM(Func_0201CA24);

INCLUDE_ASM(Func_0201CA48);

INCLUDE_ASM(Func_0201CA6C);

INCLUDE_ASM(Func_0201CA70);

INCLUDE_ASM(Func_0201CA74);

INCLUDE_ASM(Func_0201CA78);

INCLUDE_ASM(Func_0201CA7C);

INCLUDE_ASM(Func_0201CA80);

INCLUDE_ASM(Func_0201CA84);

INCLUDE_ASM(Func_0201CA98);

INCLUDE_ASM(Func_0201CAAC);

INCLUDE_ASM(Func_0201CAC0);

INCLUDE_ASM(Func_0201CAD4);

INCLUDE_ASM(Func_0201CAE8);

INCLUDE_ASM(Func_0201CB10);

INCLUDE_ASM(Func_0201CFE8);

INCLUDE_ASM(Func_0201D8AC);

INCLUDE_ASM(Func_0201D8D8);

INCLUDE_ASM(GamePlaySound);

INCLUDE_ASM(Func_0201DA24);

INCLUDE_ASM(Func_0201DA50);

#ifndef VERSION_GZLJ01
INCLUDE_ASM(DrawString);
#endif

INCLUDE_ASM(Func_0201DA78);

INCLUDE_ASM(Func_0201DB50);

INCLUDE_ASM(DrawItemIcon);

INCLUDE_ASM(DrawItemCount);

INCLUDE_ASM(Func_0201DCEC);

INCLUDE_ASM(Func_0201DD20);

INCLUDE_ASM(Func_0201DDA0);

#ifdef VERSION_GZLE01_ud
// This version adds a sound test (L toggles it; see docs/layout.md).
INCLUDE_ASM(ItemMenuUpdate);
#else
// Item menu: move the cursor over the 5 x 3 slots with the d-pad (with key repeat); B picks
// the item under it, and the cursor sprite slides there.
void ItemMenuUpdate(void)
{
    s32 col = (u8)(gGameState.menuCursor % 5);
    s32 row = (u8)(gGameState.menuCursor / 5);
    u16 keys = gMain.heldKeys;
    u8 item;
    struct ShopItem *entry;

#ifdef VERSION_GZLJ01
    if (gGameState.unk1E0 != 0)
#else
    if (gGameState.unk1E0 > 1)
#endif
    {
        Func_0201DDA0();
        return;
    }
    if (gGameState.unk1E5_0)
    {
        gGameState.menuCursorX += gGameState.menuCursorDX;
        gGameState.menuCursorY += gGameState.menuCursorDY;
        if (gGameState.menuCursorX == 0x9000)
        {
            gGameState.unk1E5_0 = 0;
            Func_0201DCEC(gGameState.menuSlots[gGameState.menuCursor]);
            SoundPlay(0x15 - SOUND_ID_SHIFT, 0);
        }
        Func_0201DB50(gGameState.menuSlots[gGameState.menuCursor], gGameState.menuCursorX >> 8,
                      gGameState.menuCursorY >> 8, gMain.mosaic != 0);
        return;
    }
    if (gMain.newKeys & keys & 0xF0)
    {
        gGameState.menuKeyRepeat = 16;
        keys = gMain.newKeys;
        gGameState.unk1DF = 0;
        SoundPlay(0x29 - SOUND_ID_SHIFT, 0);
    }
    else if (Func_02018A90(&gGameState.menuKeyRepeat))
    {
        keys = 0;
    }
    else if (keys & 0xF0)
    {
        gGameState.menuKeyRepeat = 4;
        SoundPlay(0x15 - SOUND_ID_SHIFT, 0);
    }
    if (keys & 0x20)
    {
        if (--col < 0)
            col = 4;
    }
    else if (keys & 0x10)
    {
        if (++col > 4)
            col = 0;
    }
    if (keys & 0x40)
    {
        if (--row < 0)
            row = 2;
    }
    else if (keys & 0x80)
    {
        if (++row > 2)
            row = 0;
    }
    gGameState.menuCursor = row * 5 + col;
    item = gGameState.menuSlots[gGameState.menuCursor];
    if (row == 2)
    {
        if ((gGameState.flags.dungeonChests >> col) & 1)
            Func_0201DA24(gChestRowIds[col]);
        else
            gGameState.unk13C = 0;
    }
    else
    {
        if (gMain.newKeys & 2)
        {
            if (item == 0)
            {
                gGameState.unk1DF = 1;
            }
            else if (item != gGameState.unk1DE)
            {
                gGameState.menuCursorX = ((gGameState.menuCursor % 5) * 24 + 20) << 8;
                gGameState.menuCursorY = ((gGameState.menuCursor / 5) * 40 + 20) << 8;
                gGameState.menuCursorDX = (0x9000 - gGameState.menuCursorX) >> 4;
                gGameState.menuCursorDY = (0x8800 - gGameState.menuCursorY) >> 4;
                gGameState.unk1E5_0 = 1;
                SoundPlay(0x14 - SOUND_ID_SHIFT, 0);
            }
            else
            {
                SoundPlay(0x1C - SOUND_ID_SHIFT, 0);
            }
        }
        entry = &gGameState.items[item];
        if (item == 0)
            gGameState.unk13C = 0;
        else
            Func_0201DA24(entry->unk4);
    }
    Func_0201DA78(gGameState.menuCursor);
}
#endif

// Set up the item menu: reset the item table, adjust prices and the dungeon item, and draw
// the ten slots.
bool32 ItemMenuInit(void)
{
    s32 i;

    CpuSet(gShopItemsInit, gGameState.items, 0x30);
    if (gGameState.unk138_4)
    {
        gGameState.items[3].price = 20;
        gGameState.items[4].price = 30;
        gGameState.items[5].price = 30;
    }
    if (gGameState.unk138_5)
    {
        gGameState.items[8].price = 10;
        gGameState.items[9].price = 20;
        gGameState.items[10].price = 40;
    }
    if (gGameState.flags.dungeonNo == 0)
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x1000;
        gGameState.items[11].unk7 = 9;
        gGameState.items[11].unk4 = 7;
        gGameState.unk1E5_3 = 1;
    }
    else
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x8000;
        gGameState.items[11].unk7 = 8;
        gGameState.items[11].unk4 = 6;
        gGameState.unk1E5_3 = 0;
    }
    i = 0;
    do
    {
        if (i == 6 && !gGameState.unk138_3)
        {
            Func_0201DD20(6, 11);
            i = 7;
        }
        else if (!gGameState.flags.unkB_7 && (i == 4 || i > 7))
        {
            Func_0201DD20(i, 0);
            i++;
        }
        else
        {
            Func_0201DD20(i, i + 1);
            i++;
        }
    } while (i <= 9);
    if (gGameState.unk1DE == 0)
        gGameState.unk1DE = 2;
    Func_0201DCEC(gGameState.unk1DE);
    UpdateHudPanels();
    DrawChestRow();
    gGameState.unk1CC = 0;
    gGameState.menuCursor = 0;
    gGameState.unk1E5_0 = 0;
    return TRUE;
}

// Draw the dungeon chest row of the item menu (a 2x2 tile icon per flags.dungeonChests bit).
// Nearly matches, but the original copies the running tile value to r0 before each store
// (`adds r0, r1, #0; strh r0, ...`) and loads it in both arms of the if (`b` over the pool),
// which neither u16, u32 nor s32 `tile` reproduces:
//     void DrawChestRow(void)
//     {
//         u16 *top = (u16 *)0x06007306;
//         s32 i;
//         u16 *bottom = (u16 *)0x06007346;
//         u32 tile;
//
//         for (i = 0; i <= 4; i++)
//         {
//             if ((gGameState.flags.dungeonChests >> i) & 1)
//                 tile = 0xB0C6;
//             else
//                 tile = 0xC0C2;
//             top[0] = tile;
//             top[1] = tile + 1;
//             bottom[0] = tile + 0x10;
//             bottom[1] = tile + 0x11;
//             bottom += 2;
//             top += 2;
//         }
//     }
INCLUDE_ASM(DrawChestRow);

bool32 SendItemUse(u8 item, u8 arg)
{
    GBACOM_BUF_ITEM[0] = item;
    GBACOM_BUF_ITEM[1] = arg;
    if (GbaComSendDataSet(GBACOM_BUF_ITEM, 2, GBACOM_SLOT_ITEM, *(u32 *)GBACOM_BUF_ITEM))
        return TRUE;
    return FALSE;
}

// Use the item in menu entry `item` (16: from the other button?). Returns FALSE if it can't be
// used right now.
// Nearly matches: the register allocation (item in r4, the entry in r5 and &items in r6)
// only comes out right when the marker test is written relative to gGameState.items, e.g.
// `((u8 *)gGameState.items)[FindMarker(1) * 8 - 0xF6]`, and even then items - 0xF6 is
// computed before adding the index instead of after (`adds r0, r6, r0; subs r0, #0xf6`).
//     bool32 UseItem(u8 item)
//     {
//         struct ShopItem *entry = &gGameState.items[item];
//         u8 price;
//
//         if (item == 0)
//             return FALSE;
//         if (!gMain.linked)
//             return TRUE;
//         if (gGameState.markers[FindMarker(1)].unk6 != 0
//          && ((item >= 3 && item <= 5) || item == 8 || item == 9 || item == 10 || item == 16))
//         {
//             if (item == 16)
//                 GamePlaySound(0x1F, 1);
//             else
//                 GamePlaySound(0xE, 1);
//             return FALSE;
//         }
//         if (item == 6)
//         {
//             GamePlaySound(0xD, 1);
//         }
//         else if (item == 11)
//         {
//             gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0x88;
//             Func_0201DCEC(gGameState.unk1DE);
//             Func_0201FCA0();
//         }
//         else if (item == 1)
//         {
//             if (gMain.unk457 == 0)
//             {
//                 if (Func_0201A8AC(1) <= 24)
//                 {
//                     GamePlaySound(0xC, 1);
//                 }
//                 else if (gGameState.unk058 == 0)
//                 {
//                     u8 i = FindMarker(3);
//
//                     gGameState.unk058 = 3;
//                     gGameState.unk054 = gGameState.markers[i].x;
//                     gGameState.unk056 = gGameState.markers[i].y;
//                     SoundPlay(0xB - SOUND_ID_SHIFT, 0);
//                 }
//                 else
//                 {
//                     GamePlaySound(0x1A6, 1);
//                 }
//             }
//             else
//             {
//                 GamePlaySound(0xE, 1);
//             }
//         }
//         else if (item >= 3 && item <= 4)
//         {
//             if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
//                 goto no_money;
//             SendItemUse(item == 3 ? 0x11 : 0x12, 0);
//         }
//         else if (item >= 8 && item <= 10)
//         {
//             if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
//             {
//                 GamePlaySound(0xF, 1);
//                 SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
//             }
//             else
//             {
//                 gGameState.effect[0] = item;
//                 Func_0201F878();
//             }
//         }
//         else
//         {
//             if (item == 5 && gGameState.rupees - gGameState.rupeeDelta < entry->price)
//             {
//             no_money:
//                 GamePlaySound(0xF, 1);
//                 SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
//                 return FALSE;
//             }
//             if (item == 16 || item == 5)
//                 price = 0;
//             else
//                 price = entry->price;
//             SendItemUse(item, price);
//         }
//         return TRUE;
//     }
INCLUDE_ASM(UseItem);

void Func_0201E584(void)
{
    if (gGameState.unk21E == 0)
    {
        if (gGameState.unk0FD == 0)
        {
            Func_0201DCEC(gGameState.unk1DE);
        }
        else
        {
            u16 tile = 0xB080;
            u16 *map = (u16 *)0x06006464;
            s32 i;

            for (i = 0; i < 4; i++)
            {
                map[0] = tile;
                map[0x20] = tile + 0x10;
                tile++;
                map++;
            }
        }
    }
}

// Slide the two HUD panels at the top of the screen in and out (unk21F/unk21D: the left one,
// unk220/unk21E: the right one with the held item and its price) depending on the game's
// state, and draw them.
// Nearly matches. The conditions of the second `else if` (unk220) come out with one jump
// threaded differently (the original re-tests dungeonNo and unk224 after the unk224 test
// fails), and in cases 0 and 1 the original keeps the digit doubled ((u8)x * 2, in r3, the
// same register as the tile in cases 2 and 3); writing it that way shifts the register
// allocation of the whole function.
//     void UpdateHudPanels(void)
//     {
//         u16 *map;
//         u16 tile;
//         s32 i;
//
//         if (gGameState.unk224 != 0)
//             gGameState.unk0FC = 2;
//         else if (gGameState.unk11A != 0)
//             gGameState.unk0FC = 0;
//         else
//             gGameState.unk0FC = 1;
//
//         if (gGameState.unk21F == 0)
//         {
//             if (gMain.modeState != 2 || !gGameState.unk1E5_1
//              || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.unk104 == 0)
//              || gGameState.unk1CC != 0
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy != 0
//                  && gGameState.unk104 == 0))
//                 gGameState.unk21F = 1;
//         }
//         else if (gGameState.unk1E5_1 && gGameState.unk1CC == 0 && gMain.modeState == 2)
//         {
//             if (((gGameState.flags.dungeonNo != 0 || gGameState.unk224 == 0) && gGameState.flags.busy == 0
//                  && gGameState.unk104 != 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 != 0))
//                 gGameState.unk21F = 0;
//         }
//
//         if (gGameState.unk220 == 0)
//         {
//             if (gGameState.unk1CC == 0)
//             {
//                 if (gMain.modeState != 2 || (!gGameState.unk1E5_1 && gGameState.unk1DE != 11)
//                  || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.unk104 == 0)
//                  || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy != 0
//                      && gGameState.unk104 == 0)
//                  || gGameState.unk121 != 0)
//                     gGameState.unk220 = 1;
//             }
//         }
//         else if (gGameState.unk1E5_1 && gMain.modeState == 2)
//         {
//             if ((gGameState.flags.dungeonNo != 0 && gGameState.flags.busy == 0 && gGameState.unk104 != 0
//                  && gGameState.unk121 == 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 == 0 && gGameState.flags.busy == 0
//                  && gGameState.unk104 != 0 && gGameState.unk121 == 0)
//              || (gGameState.flags.dungeonNo == 0 && gGameState.unk224 != 0)
//              || gGameState.unk1CC != 0)
//                 gGameState.unk220 = 0;
//         }
//
//         if (gGameState.unk21F != 0)
//         {
//             if (gGameState.unk21D <= 3)
//             {
//                 gGameState.unk21D++;
//                 map = (u16 *)0x06006440 + (0x1C - gGameState.unk21D);
//                 tile = 0xE01B;
//                 for (i = 0; i < gGameState.unk21D; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 gGameState.unk21D = 4;
//             }
//         }
//         else
//         {
//             tile = gHudPanelTiles[gGameState.unk0FC];
//             map = (u16 *)0x06006470;
//             if (gGameState.unk21D != 0)
//             {
//                 gGameState.unk21D--;
//                 for (i = 4; i != gGameState.unk21D; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//                 tile = 0xE01B;
//                 for (; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 for (i = 0; i < 4; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//             }
//         }
//
//         if (gGameState.unk220 != 0)
//         {
//             if (gGameState.unk21E <= 3)
//             {
//                 gGameState.unk21E++;
//                 map = (u16 *)0x06006440 + (0x16 - gGameState.unk21E);
//                 tile = 0xE01B;
//                 for (i = 0; i < gGameState.unk21E; i++)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 gGameState.unk21E = 4;
//             }
//         }
//         else if (gGameState.unk21E != 0)
//         {
//             gGameState.unk21E--;
//             if (gGameState.unk0FD == 0)
//             {
//                 u8 n = gGameState.unk21E;
//                 u8 num;
//
//                 map = (u16 *)0x06006440 + (0x15 - n);
//                 switch (n)
//                 {
//                 case 0:
//                     num = gGameState.items[gGameState.unk1DE].price % 10;
//                     map[0] = num * 2 + 0x4030;
//                     map[0x20] = num * 2 + 0x4031;
//                     break;
//                 case 1:
//                     num = gGameState.items[gGameState.unk1DE].price / 10;
//                     if (num != 0)
//                     {
//                         map[0] = num * 2 + 0x4030;
//                         map[0x20] = num * 2 + 0x4031;
//                     }
//                     else
//                     {
//                         map[0] = 0xD09F;
//                         map[0x20] = 0xD09F;
//                     }
//                     break;
//                 case 2:
//                     tile = gGameState.items[gGameState.unk1DE].tile;
//                     map[0] = tile + 1;
//                     map[0x20] = tile + 1 + 0x10;
//                     break;
//                 case 3:
//                     tile = gGameState.items[gGameState.unk1DE].tile;
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     break;
//                 }
//                 map++;
//                 tile = 0xE01B;
//                 for (i = gGameState.unk21E; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//             else
//             {
//                 map = (u16 *)0x06006464;
//                 tile = 0xB080;
//                 for (i = 4; i != gGameState.unk21E; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile + 0x10;
//                     tile++;
//                     map++;
//                 }
//                 tile = 0xE01B;
//                 for (; i != 0; i--)
//                 {
//                     map[0] = tile;
//                     map[0x20] = tile | 0x800;
//                     tile ^= 0x400;
//                     map++;
//                 }
//             }
//         }
//     }
INCLUDE_ASM(UpdateHudPanels);

void Func_0201EBAC(void)
{
    gGameState.unk1E5_1 = 1;
    gGameState.unk1E5_2 = 0;
    gGameState.unk120 = 0;
}

void Func_0201EBD4(void)
{
    gGameState.unk1C8 = 180;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
    Func_020205F8(0);
}

void Func_0201EC10(void)
{
    u8 mosaic = gMain.mosaic != 0;
    u8 i;
    u8 unk;

    if (gGameState.flags.busy & 1)
        gGameState.unk1C8++;
    if (gGameState.unk1C8 == 0)
    {
        Func_0201EBAC();
        return;
    }
    i = FindMarker(3);
    gGameState.shake = gGameState.unk1C8;
    unk = gGameState.unk1EE;
    if (unk == 0)
        Func_020190A8(gSprite_0202CC60, gGameState.markers[i].x - gMain.bgOfs[3][0],
                      gGameState.markers[i].y - gMain.bgOfs[3][1], mosaic, unk);
}

void Func_0201ECC4(void)
{
    gGameState.unk12D = 0;
    gGameState.unk120 = 1;
    gGameState.unk034 = gGameState.unk032 = 0x44;
    gGameState.unk12C = gSprite_0202C950[0].duration;
    gGameState.unk12B = 1;
}

void Func_0201ED08(void)
{
    u32 mosaic = gMain.mosaic != 0;
    const struct SpriteAnimFrame *anim;

    if (gGameState.unk12C == 0)
    {
        if (gGameState.unk12D < 22)
        {
            gGameState.unk12D++;
        }
        else if (gGameState.unk12B < 2)
        {
            gGameState.unk12B = 2;
            gGameState.unk132 = 10;
            gGameState.unk13E = 300;
        }
        gGameState.unk12C = gSprite_0202C950[gGameState.unk12D].duration;
        anim = gSprite_0202C950;
    }
    else
    {
        gGameState.unk12C--;
        anim = gSprite_0202C950;
    }
    DrawSpriteAt(anim, gGameState.unk12D, gGameState.unk032, gGameState.unk034, mosaic, 0, 1);
}

void Func_0201EDC4(void)
{
    Func_0201ECC4();
    gGameState.unk12E = gSprite_0202C928[0].duration;
    gGameState.unk12F = 0;
    GamePlaySound(0x23, 1);
}

// Draw the item effect sprite (kind 0: gSprite_0202C928, else gSprite_0202C940) at a map
// position. Nearly matches, but the original sign-extends x and y again before each
// subtraction (agbcc drops that, since only the low half reaches DrawSpriteAt), and y and
// the mosaic flag swap r7/r8:
//     void Func_0201EDF8(s16 x, s16 y, u8 kind)
//     {
//         u32 mosaic = gMain.mosaic != 0;
//         const struct SpriteAnimFrame *anim;
//         u8 flip;
//
//         if (kind == 0)
//         {
//             if (gGameState.unk12E == 0)
//             {
//                 gGameState.unk12F ^= 1;
//                 gGameState.unk12E = gSprite_0202C928[gGameState.unk12F].duration;
//                 anim = gSprite_0202C928;
//             }
//             else
//             {
//                 gGameState.unk12E--;
//                 anim = gSprite_0202C928;
//             }
//             flip = 0;
//             if (gGameState.effect[0] == 12)
//                 flip = 16;
//             DrawSpriteAt(anim, gGameState.unk12F, x - gMain.bgOfs[3][0] + 3,
//                          y - gMain.bgOfs[3][1] - 24, mosaic, flip, 1);
//         }
//         else
//         {
//             if (gGameState.unk130 == 0)
//             {
//                 gGameState.unk131 ^= 1;
//                 gGameState.unk130 = gSprite_0202C940[gGameState.unk131].duration;
//                 anim = gSprite_0202C940;
//             }
//             else
//             {
//                 gGameState.unk130--;
//                 anim = gSprite_0202C940;
//             }
//             flip = 0;
//             if (gGameState.effect[0] == 12)
//                 flip = 8;
//             if (gGameState.unk131 == 0)
//                 DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] - 4,
//                              y - gMain.bgOfs[3][1], mosaic, flip, 1);
//             else
//                 DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] + 4,
//                              y - gMain.bgOfs[3][1], mosaic, flip, 1);
//         }
//     }
INCLUDE_ASM(Func_0201EDF8);

// Draw the flags.timer count (two digits) at x, y; a sound plays when it changes.
void DrawTimerCount(s16 x, s16 y)
{
    u32 mosaic = gMain.mosaic != 0;
    u8 digit;

    if (gGameState.flags.timer != gGameState.timer)
        SoundPlay(0xC - SOUND_ID_SHIFT, 0);
    digit = gGameState.flags.timer / 10;
    if (digit != 0)
        DrawSprite(gSpriteTable[10], 0, x, y, 0, mosaic, 0, 0, 0, digit + 1, 0);
    digit = gGameState.flags.timer % 10;
    DrawSprite(gSpriteTable[10], 0, x + 8, y, 0, mosaic, 0, 0, 0, digit + 1, 0);
}

void Func_0201F068(void)
{
    struct Main *main = &gMain;
    u8 notType3 = gGameState.effect[0] != 3;
    u8 price;
    u16 x;
    u16 y;

    if (gGameState.unk12B != 6)
        Func_0201ED08();
    if (gGameState.unk12B == 2)
    {
        if (gGameState.unk1FA == 0)
        {
            if (gGameState.unk132 == 0)
            {
                if (gGameState.effect[0] == 13)
                    price = gGameState.items[5].price;
                else
                    price = gGameState.items[gGameState.effect[0]].price;
                SendItemUse(gGameState.effect[0], price);
                gGameState.unk12B = 5;
            }
            else
            {
                gGameState.unk132--;
            }
        }
    }
    else if (gGameState.unk12B == 5)
    {
        if (gGameState.flags.timer != 0)
        {
            gGameState.unk120 = 0;
            gGameState.unk12B = 6;
        }
    }
    else if (gGameState.unk12B == 6)
    {
        u8 i = FindMarker(1);

        Func_0201EDF8(gGameState.markers[i].x, gGameState.markers[i].y, notType3);
        if (gGameState.flags.timer == 0)
        {
            gGameState.unk120 = 1;
            gGameState.unk1C8 = 120;
            gGameState.unk12B = 7;
            gGameState.unk032 = gGameState.markers[i].x - main->bgOfs[3][0];
            gGameState.unk034 = gGameState.markers[i].y - main->bgOfs[3][1];
            if (gGameState.unk1DE == 5)
                GamePlaySound(0x30, 1);
            else if (!notType3)
                GamePlaySound(0x2E, 1);
            else
                GamePlaySound(0x2F, 1);
        }
        else
        {
            if (!notType3)
            {
                x = gGameState.markers[i].x - main->bgOfs[3][0] + 4;
                y = gGameState.markers[i].y - main->bgOfs[3][1] - 36;
            }
            else
            {
                x = gGameState.markers[i].x - main->bgOfs[3][0] + 4;
                y = gGameState.markers[i].y - main->bgOfs[3][1] - 16;
            }
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 7)
    {
        if (gGameState.unk1C8 == 0)
        {
            gGameState.unk120 = 0;
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
    }
}

void Func_0201F240(void)
{
    Func_0201ECC4();
    gGameState.unk130 = gSprite_0202C940[0].duration;
    gGameState.unk131 = 0;
    GamePlaySound(0x25, 1);
}

void Func_0201F274(void)
{
    gGameState.unk1C8 = 120;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

void Func_0201F2A8(void)
{
    if (gGameState.unk1C8 == 0)
        Func_0201EBAC();
}

void Func_0201F2C4(void)
{
    Func_0201ECC4();
    if (gGameState.effect[1] == 13)
    {
        gGameState.effect[0] = gGameState.effect[1];
        GamePlaySound(0x29, 1);
    }
    else if (gGameState.effect[1] == 12)
    {
        GamePlaySound(0x2A, 1);
    }
    else
    {
        GamePlaySound(0x2B, 1);
    }
}

INCLUDE_ASM(Func_0201F308);

INCLUDE_ASM(Func_0201F420);

INCLUDE_ASM(Func_0201F460);

INCLUDE_ASM(Func_0201F878);

INCLUDE_ASM(Func_0201F8C4);

INCLUDE_ASM(Func_0201FA2C);

INCLUDE_ASM(Func_0201FCA0);

INCLUDE_ASM(Func_0201FD04);

INCLUDE_ASM(Func_0201FEEC);

INCLUDE_ASM(Func_0201FF14);

INCLUDE_ASM(Func_02020150);

INCLUDE_ASM(Func_02020190);

INCLUDE_ASM(Func_02020298);

INCLUDE_ASM(Func_020202E8);

INCLUDE_ASM(Func_020203EC);

INCLUDE_ASM(MainMode2);

INCLUDE_ASM(Func_020204D0);

INCLUDE_ASM(Func_02020504);

INCLUDE_ASM(Func_02020538);

INCLUDE_ASM(ObjReset);

INCLUDE_ASM(Func_02020598);

INCLUDE_ASM(Func_020205D4);

INCLUDE_ASM(Func_020205F8);

INCLUDE_ASM(ObjResetActive);

INCLUDE_ASM(Func_02020668);

INCLUDE_ASM(Func_020207D4);

INCLUDE_ASM(Func_0202081C);

INCLUDE_ASM(BuildWaveScrollTable);

INCLUDE_ASM(Func_020208FC);

INCLUDE_ASM(Func_020209A0);

INCLUDE_ASM(Func_020209E0);

INCLUDE_ASM(Func_02020BEC);

INCLUDE_ASM(Func_02020C10);
