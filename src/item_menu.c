#include "global.h"
#include "gba/syscall.h"
#include "game.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

// Game code object 4 (GZLJ01 0201DA78-0201EBAC): the item menu, UseItem and the HUD panels.
// See docs/layout.md.

const struct ItemSlot gShopItemsInit[12] =
{
    {0x0000, 0x1DE, -1, 0, 15},
    {0xC0A6, 0x1DC, 0, 0, 14},
    {0xD0A0, 0x1D2, 1, 10, 4},
    {0x80CA, 0x1D4, 2, 30, 8},
    {0xC0AA, 0x1D6, 3, 40, 8},
    {0xC0C8, 0x1D8, 20, 40, 8},
    {0x10A2, 0x1DA, 4, 0, 3},
    {0xA0A8, 0x190, 8, 0, 3},
    {0x80A4, 0x1DE, 9, 20, 7},
    {0xC0A4, 0x1DE, 10, 40, 14},
    {0x10A4, 0x1DE, 11, 80, 3},
    {0x80AE, 0x192, 6, 0, 8}
};

const s16 gChestRowIds[5] =
{
    0x4A, 0x4B, 0x4C, 0x4E, 0x4D
};

#ifdef VERSION_GZLJ01
const u16 gHudPanelTiles[3] =
{
    0xB065, 0xB084, 0xB069
};
#else
const u16 gHudPanelTiles[3] =
{
    0xB065, 0xB084, 0xB06A
};
#endif

// Draw the blinking cursor on menu slot (rows 0-1: the item grid, row 2: the bottom row).
void DrawSlotCursor(u8 slot)
{
    // (row compares signed, so it is an int; the x sums only match in s16.)
    u8 col = slot % 5;
    s32 row = (u8)(slot / 5);
    bool32 mosaic = gMain.mosaic != 0;

    if (row <= 1)
        DrawSprite(gSprite_0202BDB8, (gMain.frameCount >> 4) & 1,
                   (s16)gMain.bgOfs[1][0] + (s16)(col * 24 + 24), row * 40 + 20, 0, mosaic, 0, 0, 0,
                   0, 0);
    else
        DrawSprite(gSprite_0202BDB8, (gMain.frameCount >> 4) & 1,
                   (s16)gMain.bgOfs[1][0] + (s16)(col * 16 + 0x27), 0x6C, 0, mosaic, 0, 0, 0, 0, 0);
}

// Add an item's icon as a 16x16 sprite to gOamBuffer.
void DrawItemSprite(u8 item, u8 x, u8 y, u8 mosaic)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.unk450];

    oam->y = y;
    oam->mosaic = mosaic;
    oam->shape = 0;
    oam->x = x;
    oam->hFlip = 0;
    oam->vFlip = 0;
    oam->size = 1;
    oam->tileNum = gGameState.items[item].objTile;
    oam->priority = 0;
    oam->paletteNum = gGameState.items[item].objPalette;
    gMain.unk450++;
}

// Draw an item's 2x2-tile icon at map (a blank tile for item 0).
void DrawItemIcon(u8 item, u16 *map)
{
    struct ItemSlot *slot = &gGameState.items[item];

    if (item == 0)
    {
        map[0] = 0xD09F;
        map[1] = 0xD09F;
        map[0x20] = 0xD09F;
        map[0x21] = 0xD09F;
    }
    else
    {
        map[0] = slot->tile;
        map[1] = slot->tile + 1;
        map[0x20] = slot->tile + 0x10;
        map[0x21] = slot->tile + 0x11;
    }
}

// Draw an item's count as two digits (two tile rows; blank tens digit below 10).
void DrawItemCount(u8 item, u16 *map)
{
    struct ItemSlot *slot = &gGameState.items[item];
    u8 count = slot->price;
    s32 tens = (u8)(count / 10) * 2;
    s32 ones = (u8)(count % 10) * 2;

    if (item == 0)
    {
        map[0] = 0;
        map[1] = 0;
        map[0x20] = 0;
        map[0x21] = 0;
    }
    else
    {
        if (tens != 0)
        {
            map[0] = tens + 0x4030;
            map[0x20] = tens + 0x4031;
        }
        else
        {
            map[0] = 0xD09F;
            map[0x20] = 0xD09F;
        }
        map[1] = ones + 0x4030;
        map[0x21] = ones + 0x4031;
    }
}

void SetSelectedItem(u8 item)
{
    u16 *map = (u16 *)0x06006464;

    gGameState.unk1DE = item;
    DrawItemIcon(item, map);
    map += 2;
    DrawItemCount(item, map);
}

// Put item in menu slot (5 per row, 3 tiles apart; count below the icon).
void SetSlotItem(u8 slot, u8 item)
{
    struct ItemMenu *menu = (struct ItemMenu *)gGameState.items;
    u8 col = slot % 5;
    u8 row = slot / 5;
    u16 *map = (u16 *)(0x06007000 + ((row * 40 + 8) >> 3) * 64 + ((col * 24 + 8) >> 2));

    menu->slotItems[slot] = item;
    DrawItemIcon(item, map);
    map += 0x40;
    DrawItemCount(item, map);
}

// The dungeon chest counter (MainMode1 starts it at state 1): state 2 blinks the new chest
// bits until unk1FA is clear, state 3 waits 300 frames or a key.
void ChestCounterMain(void)
{
    if (gGameState.unk1E0 == 3)
    {
        if (--gGameState.unk1CA == 0 || gMain.newKeys != 0)
        {
            DrawChestRow();
            SoundPlay(0x2A - SOUND_ID_SHIFT, 0);
            gGameState.unk1CC = 0;
            gGameState.unk13C = 0;
            gGameState.unk1E0 = 0;
            gGameState.unk104 = 1;
            gGameState.unk1CA = 0;
        }
    }
    else if (gGameState.unk1E0 == 2)
    {
        if (++gGameState.unk1CA == 0x20)
            SoundPlay(0x12 - SOUND_ID_SHIFT, 0);
        if (gGameState.unk1CA & 0x20)
        {
            if (!(gGameState.unk1CA & 0x1F))
                SoundPlay(0x25 - SOUND_ID_SHIFT, 0);
            gGameState.flags.dungeonChests |= gGameState.unk1E1;
        }
        else
        {
            gGameState.flags.dungeonChests &= ~gGameState.unk1E1;
        }
        DrawChestRow();
        if (gGameState.unk1FA == 0 && (gGameState.unk1CA & 0x20))
        {
            gGameState.unk1CA = 300;
            gGameState.unk1E0++;
        }
    }
}

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
    struct ItemSlot *entry;

#ifdef VERSION_GZLJ01
    if (gGameState.unk1E0 != 0)
#else
    if (gGameState.unk1E0 > 1)
#endif
    {
        ChestCounterMain();
        return;
    }
    if (gGameState.unk1E5_0)
    {
        gGameState.menuCursorX += gGameState.menuCursorDX;
        gGameState.menuCursorY += gGameState.menuCursorDY;
        if (gGameState.menuCursorX == 0x9000)
        {
            gGameState.unk1E5_0 = 0;
            SetSelectedItem(gGameState.slotItems[gGameState.menuCursor]);
            SoundPlay(0x15 - SOUND_ID_SHIFT, 0);
        }
        DrawItemSprite(gGameState.slotItems[gGameState.menuCursor], gGameState.menuCursorX >> 8,
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
    else if (CountDownU8(&gGameState.menuKeyRepeat))
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
    item = gGameState.slotItems[gGameState.menuCursor];
    if (row == 2)
    {
        if ((gGameState.flags.dungeonChests >> col) & 1)
            MsgStartSilent(gChestRowIds[col]);
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
            MsgStartSilent(entry->unk4);
    }
    DrawSlotCursor(gGameState.menuCursor);
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
        gGameState.items[11].objPalette = 9;
        gGameState.items[11].unk4 = 7;
        gGameState.unk1E5_3 = 1;
    }
    else
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x8000;
        gGameState.items[11].objPalette = 8;
        gGameState.items[11].unk4 = 6;
        gGameState.unk1E5_3 = 0;
    }
    i = 0;
    do
    {
        if (i == 6 && !gGameState.unk138_3)
        {
            SetSlotItem(6, 11);
            i = 7;
        }
        else if (!gGameState.flags.unkB_7 && (i == 4 || i > 7))
        {
            SetSlotItem(i, 0);
            i++;
        }
        else
        {
            SetSlotItem(i, i + 1);
            i++;
        }
    } while (i <= 9);
    if (gGameState.unk1DE == 0)
        gGameState.unk1DE = 2;
    SetSelectedItem(gGameState.unk1DE);
    UpdateHudPanels();
    DrawChestRow();
    gGameState.unk1CC = 0;
    gGameState.menuCursor = 0;
    gGameState.unk1E5_0 = 0;
    return TRUE;
}

// Draw the dungeon chest row of the item menu (a 2x2 tile icon per flags.dungeonChests bit).
// The icon's four tiles are written as constants (the compiler derives each from the last).
void DrawChestRow(void)
{
    u16 *top = (u16 *)0x06007306;
    s32 i;

    for (i = 0; i <= 4; i++)
    {
        if ((gGameState.flags.dungeonChests >> i) & 1)
        {
            top[0] = 0xB0C6;
            top[1] = 0xB0C7;
            top[0x20] = 0xB0D6;
            top[0x21] = 0xB0D7;
        }
        else
        {
            top[0] = 0xC0C2;
            top[1] = 0xC0C3;
            top[0x20] = 0xC0D2;
            top[0x21] = 0xC0D3;
        }
        top += 2;
    }
}

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
//         struct ItemSlot *entry = &gGameState.items[item];
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
//                 ShowMessage(0x1F, 1);
//             else
//                 ShowMessage(0xE, 1);
//             return FALSE;
//         }
//         if (item == 6)
//         {
//             ShowMessage(0xD, 1);
//         }
//         else if (item == 11)
//         {
//             gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0x88;
//             SetSelectedItem(gGameState.unk1DE);
//             Func_0201FCA0();
//         }
//         else if (item == 1)
//         {
//             if (gMain.unk457 == 0)
//             {
//                 if (MarkerDistance(1) <= 24)
//                 {
//                     ShowMessage(0xC, 1);
//                 }
//                 else if (gGameState.icons[2].type == 0)
//                 {
//                     u8 i = FindMarker(3);
//
//                     gGameState.icons[2].type = 3;
//                     gGameState.icons[2].x = gGameState.markers[i].x;
//                     gGameState.icons[2].y = gGameState.markers[i].y;
//                     SoundPlay(0xB - SOUND_ID_SHIFT, 0);
//                 }
//                 else
//                 {
//                     ShowMessage(0x1A6, 1);
//                 }
//             }
//             else
//             {
//                 ShowMessage(0xE, 1);
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
//                 ShowMessage(0xF, 1);
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
//                 ShowMessage(0xF, 1);
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
            SetSelectedItem(gGameState.unk1DE);
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
