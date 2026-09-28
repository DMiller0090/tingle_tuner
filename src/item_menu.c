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
        DrawSprite(gSlotCursorSprite, (gMain.frameCount >> 4) & 1,
                   gMain.bgOfs[1][0] + (s16)(col * 24 + 24), row * 40 + 20, 0, mosaic, 0, 0, 0,
                   0, 0);
    else
        DrawSprite(gSlotCursorSprite, (gMain.frameCount >> 4) & 1,
                   gMain.bgOfs[1][0] + (s16)(col * 16 + 0x27), 0x6C, 0, mosaic, 0, 0, 0, 0, 0);
}

// Add an item's icon as a 16x16 sprite to gOamBuffer.
void DrawItemSprite(u8 item, u8 x, u8 y, u8 mosaic)
{
    struct OamData *oam = (struct OamData *)&gOamBuffer[gMain.oamCount];

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
    gMain.oamCount++;
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

    gGameState.selectedItem = item;
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
// bits until inMessage is clear, state 3 waits 300 frames or a key.
void ChestCounterMain(void)
{
    if (gGameState.chestState == 3)
    {
        if (--gGameState.chestTimer == 0 || gMain.newKeys != 0)
        {
            DrawChestRow();
            SoundPlay(0x2A - SOUND_ID_SHIFT, 0);
            gGameState.menuOpen = 0;
            gGameState.msgOpen = 0;
            gGameState.chestState = 0;
            gGameState.active = 1;
            gGameState.chestTimer = 0;
        }
    }
    else if (gGameState.chestState == 2)
    {
        if (++gGameState.chestTimer == 0x20)
            SoundPlay(0x12 - SOUND_ID_SHIFT, 0);
        if (gGameState.chestTimer & 0x20)
        {
            if (!(gGameState.chestTimer & 0x1F))
                SoundPlay(0x25 - SOUND_ID_SHIFT, 0);
            gGameState.flags.dungeonChests |= gGameState.newChests;
        }
        else
        {
            gGameState.flags.dungeonChests &= ~gGameState.newChests;
        }
        DrawChestRow();
        if (gGameState.inMessage == 0 && (gGameState.chestTimer & 0x20))
        {
            gGameState.chestTimer = 300;
            gGameState.chestState++;
        }
    }
}

// Item menu: move the cursor over the 5 x 3 slots with the d-pad (with key repeat); B picks
// the item under it, and the cursor sprite slides there.
// GZLE01_ud adds a message test: L toggles it, then Up/Down step the message ID by 1,
// Left/Right by 10 and R by 100 (wrapping in 0-0x437), shown as four digits at the rupee
// counter (see docs/layout.md).
void ItemMenuUpdate(void)
{
    s32 col = (u8)(gGameState.menuCursor % 5);
    s32 row = (u8)(gGameState.menuCursor / 5);
    u16 keys = gMain.heldKeys;
    u8 item;
    struct ItemSlot *entry;
#ifdef VERSION_GZLE01_ud
    u16 *map;
    s16 id;
    s32 n;
    s32 t;
#endif

#ifdef VERSION_GZLJ01
    if (gGameState.chestState != 0)
#else
    if (gGameState.chestState > 1)
#endif
    {
        ChestCounterMain();
        return;
    }
#ifdef VERSION_GZLE01_ud
    map = (u16 *)0x06006464;
    if (gMain.newKeys & 0x200)
    {
        gGameState.msgTest ^= 1;
        if (gGameState.msgTest)
        {
            MsgStartSilent(0);
            gGameState.msgId = 0;
        }
    }
    if (gGameState.msgTest)
    {
        if (gMain.newKeys & keys & 0x1F0)
        {
            gGameState.menuKeyRepeat = 16;
            keys = gMain.newKeys;
        }
        else if (CountDownU8(&gGameState.menuKeyRepeat))
        {
            keys = 0;
        }
        else if (keys & 0xF0)
        {
            gGameState.menuKeyRepeat = 4;
        }
        id = gGameState.msgId;
        if (keys & 0x40)
        {
            if (++id > 0x437)
                id = 0;
        }
        else if (keys & 0x80)
        {
            if (--id < 0)
                id = 0x437;
        }
        else if (keys & 0x20)
        {
            id += 10;
            if (id > 0x437)
                id -= 0x438;
        }
        else if (keys & 0x10)
        {
            id -= 10;
            if (id < 0)
                id += 0x438;
        }
        else if (gMain.newKeys & 0x100)
        {
            id += 100;
            if (id > 0x437)
                id -= 0x438;
        }
        n = id;
        MsgStartSilent(n);
        map[0] = n / 1000 * 2 + 0xB030;
        map[0x20] = n / 1000 * 2 + 0xB031;
        map++;
        t = (s16)(n / 100);
        map[0] = t % 10 * 2 + 0xB030;
        map[0x20] = t % 10 * 2 + 0xB031;
        map++;
        t = (s16)(n / 10);
        map[0] = t % 10 * 2 + 0xB030;
        map[0x20] = t % 10 * 2 + 0xB031;
        map++;
        map[0] = n % 10 * 2 + 0xB030;
        map[0x20] = n % 10 * 2 + 0xB031;
        return;
    }
#endif
    if (gGameState.itemFlying)
    {
        gGameState.menuCursorX += gGameState.menuCursorDX;
        gGameState.menuCursorY += gGameState.menuCursorDY;
        if (gGameState.menuCursorX == 0x9000)
        {
            gGameState.itemFlying = 0;
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
            gGameState.msgOpen = 0;
    }
    else
    {
        if (gMain.newKeys & 2)
        {
            if (item == 0)
            {
                gGameState.unk1DF = 1;
            }
            else if (item != gGameState.selectedItem)
            {
                gGameState.menuCursorX = ((gGameState.menuCursor % 5) * 24 + 20) << 8;
                gGameState.menuCursorY = ((gGameState.menuCursor / 5) * 40 + 20) << 8;
                gGameState.menuCursorDX = (0x9000 - gGameState.menuCursorX) >> 4;
                gGameState.menuCursorDY = (0x8800 - gGameState.menuCursorY) >> 4;
                gGameState.itemFlying = 1;
                SoundPlay(0x14 - SOUND_ID_SHIFT, 0);
            }
            else
            {
                SoundPlay(0x1C - SOUND_ID_SHIFT, 0);
            }
        }
        entry = &gGameState.items[item];
        if (item == 0)
            gGameState.msgOpen = 0;
        else
            MsgStartSilent(entry->descMsg);
    }
    DrawSlotCursor(gGameState.menuCursor);
}

// Set up the item menu: reset the item table, adjust prices and the dungeon item, and draw
// the ten slots.
bool32 ItemMenuInit(void)
{
    s32 i;

    CpuSet(gShopItemsInit, gGameState.items, 0x30);
    if (gGameState.balloonDiscount)
    {
        gGameState.items[3].price = 20;
        gGameState.items[4].price = 30;
        gGameState.items[5].price = 30;
    }
    if (gGameState.tingDiscount)
    {
        gGameState.items[8].price = 10;
        gGameState.items[9].price = 20;
        gGameState.items[10].price = 40;
    }
    if (gGameState.flags.dungeonNo == 0)
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x1000;
        gGameState.items[11].objPalette = 9;
        gGameState.items[11].descMsg = 7;
        gGameState.onSea = 1;
    }
    else
    {
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xFFF) | 0x8000;
        gGameState.items[11].objPalette = 8;
        gGameState.items[11].descMsg = 6;
        gGameState.onSea = 0;
    }
    i = 0;
    do
    {
        if (i == 6 && !gGameState.shopUnlocked)
        {
            SetSlotItem(6, 11);
            i = 7;
        }
        else if (!gGameState.flags.hasMagic && (i == 4 || i > 7))
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
    if (gGameState.selectedItem == 0)
        gGameState.selectedItem = 2;
    SetSelectedItem(gGameState.selectedItem);
    UpdateHudPanels();
    DrawChestRow();
    gGameState.menuOpen = 0;
    gGameState.menuCursor = 0;
    gGameState.itemFlying = 0;
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
// used right now. FindMarker's result goes through a u8 local: indexing with the call directly
// puts the index first in the address sum.
bool32 UseItem(u8 item)
{
    struct ItemSlot *entry = &gGameState.items[item];
    u8 price;
    u8 m;

    if (item == 0)
        return FALSE;
    if (!gMain.linked)
        return TRUE;
    m = FindMarker(1);
    if (gGameState.markers[m].param != 0
     && ((item >= 3 && item <= 5) || item == 8 || item == 9 || item == 10 || item == 16))
    {
        if (item == 16)
            ShowMessage(0x1F, 1);
        else
            ShowMessage(0xE, 1);
        return FALSE;
    }
    if (item == 6)
    {
        ShowMessage(0xD, 1);
    }
    else if (item == 11)
    {
#ifdef VERSION_GZLJ01
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0x88;
#else
        gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0x8A;
#endif
        SetSelectedItem(gGameState.selectedItem);
        StartEffect11();
    }
    else if (item == 1)
    {
        if (gMain.mapImageMode == 0)
        {
            if (MarkerDistance(1) <= 24)
            {
                ShowMessage(0xC, 1);
            }
            else if (gGameState.icons[2].type == 0)
            {
                u8 i = FindMarker(3);

                gGameState.icons[2].type = 3;
                gGameState.icons[2].x = gGameState.markers[i].x;
                gGameState.icons[2].y = gGameState.markers[i].y;
                SoundPlay(0xB - SOUND_ID_SHIFT, 0);
            }
            else
            {
                ShowMessage(0x1A6, 1);
            }
        }
        else
        {
            ShowMessage(0xE, 1);
        }
    }
    else if (item >= 3 && item <= 4)
    {
        if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
            goto no_money;
        SendItemUse(item == 3 ? 0x11 : 0x12, 0);
    }
    else if (item >= 8 && item <= 10)
    {
        if (gGameState.rupees - gGameState.rupeeDelta < entry->price)
        {
            ShowMessage(0xF, 1);
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
        }
        else
        {
            gGameState.effect[0] = item;
            StartTingEffect();
        }
    }
    else
    {
        if (item == 5 && gGameState.rupees - gGameState.rupeeDelta < entry->price)
        {
        no_money:
            ShowMessage(0xF, 1);
            SoundPlay(0x1A - SOUND_ID_SHIFT, 0);
            return FALSE;
        }
        if (item == 16 || item == 5)
            price = 0;
        else
            price = entry->price;
        SendItemUse(item, price);
    }
    return TRUE;
}


void RedrawHudItem(void)
{
    if (gGameState.hudRightSlide == 0)
    {
        if (gGameState.hudChartLabel == 0)
        {
            SetSelectedItem(gGameState.selectedItem);
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

// The left panel is one tile wider in the US versions.
#ifdef VERSION_GZLJ01
#define HUD_LEFT_WIDTH 4
#else
#define HUD_LEFT_WIDTH 5
#endif

// Slide the two HUD panels at the top of the screen in and out (hudLeftHidden/hudLeftSlide:
// the left one, the A button's label; hudRightHidden/hudRightSlide: the right one with the
// item on B and its price) depending on the game's state, and draw them.
void UpdateHudPanels(void)
{
    u16 *map;
    u16 tile;
    s32 i;

    if (gGameState.chartOpen != 0)
        gGameState.aButtonAction = 2;
    else if (gGameState.selection != 0)
        gGameState.aButtonAction = 0;
    else
        gGameState.aButtonAction = 1;

    if (gGameState.hudLeftHidden == 0)
    {
        if (gMain.modeState != 2 || !gGameState.itemsEnabled
         || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.active == 0)
         || gGameState.menuOpen != 0
         || (gGameState.flags.dungeonNo == 0 && gGameState.chartOpen == 0 && gGameState.flags.busy != 0
             && gGameState.active == 0))
            gGameState.hudLeftHidden = 1;
    }
    else if (gGameState.itemsEnabled && gGameState.menuOpen == 0 && gMain.modeState == 2)
    {
        if (((gGameState.flags.dungeonNo != 0 || gGameState.chartOpen == 0) && gGameState.flags.busy == 0
             && gGameState.active != 0)
         || (gGameState.flags.dungeonNo == 0 && gGameState.chartOpen != 0))
            gGameState.hudLeftHidden = 0;
    }

    if (gGameState.hudRightHidden == 0)
    {
        if (gGameState.menuOpen == 0)
        {
            if (gMain.modeState != 2 || (!gGameState.itemsEnabled && gGameState.selectedItem != 11)
             || (gGameState.flags.dungeonNo != 0 && gGameState.flags.busy != 0 && gGameState.active == 0)
             || (gGameState.flags.dungeonNo == 0 && gGameState.chartOpen == 0 && gGameState.flags.busy != 0
                 && gGameState.active == 0)
             || gGameState.holdState != 0)
                gGameState.hudRightHidden = 1;
        }
    }
    else if (gGameState.itemsEnabled && gMain.modeState == 2)
    {
        if ((gGameState.flags.dungeonNo != 0 && gGameState.flags.busy == 0 && gGameState.active != 0
             && gGameState.holdState == 0)
         || ((gGameState.flags.dungeonNo == 0 && gGameState.chartOpen == 0 && gGameState.flags.busy == 0
              && gGameState.active != 0 && gGameState.holdState == 0)
             || (gGameState.flags.dungeonNo == 0 && gGameState.chartOpen != 0)
             || gGameState.menuOpen != 0))
            gGameState.hudRightHidden = 0;
    }

    if (gGameState.hudLeftHidden != 0)
    {
        if (gGameState.hudLeftSlide <= HUD_LEFT_WIDTH - 1)
        {
            gGameState.hudLeftSlide++;
            map = (u16 *)0x06006440 + (0x18 + HUD_LEFT_WIDTH - gGameState.hudLeftSlide);
            tile = 0xE01B;
            for (i = 0; i < gGameState.hudLeftSlide; i++)
            {
                map[0] = tile;
                map[0x20] = tile | 0x800;
                tile ^= 0x400;
                map++;
            }
        }
        else
        {
            gGameState.hudLeftSlide = HUD_LEFT_WIDTH;
        }
    }
    else
    {
        tile = gHudPanelTiles[gGameState.aButtonAction];
        map = (u16 *)0x06006470;
        if (gGameState.hudLeftSlide != 0)
        {
            gGameState.hudLeftSlide--;
            for (i = HUD_LEFT_WIDTH; i != gGameState.hudLeftSlide; i--)
            {
                map[0] = tile;
                map[0x20] = tile + 0x10;
                tile++;
                map++;
            }
            tile = 0xE01B;
            for (; i != 0; i--)
            {
                map[0] = tile;
                map[0x20] = tile | 0x800;
                tile ^= 0x400;
                map++;
            }
        }
        else
        {
            for (i = 0; i < HUD_LEFT_WIDTH; i++)
            {
                map[0] = tile;
                map[0x20] = tile + 0x10;
                tile++;
                map++;
            }
        }
    }

    if (gGameState.hudRightHidden != 0)
    {
        if (gGameState.hudRightSlide <= 3)
        {
            gGameState.hudRightSlide++;
            map = (u16 *)0x06006440 + (0x16 - gGameState.hudRightSlide);
            tile = 0xE01B;
            for (i = 0; i < gGameState.hudRightSlide; i++)
            {
                map[0] = tile;
                map[0x20] = tile | 0x800;
                tile ^= 0x400;
                map++;
            }
        }
        else
        {
            gGameState.hudRightSlide = 4;
        }
    }
    else if (gGameState.hudRightSlide != 0)
    {
        gGameState.hudRightSlide--;
        if (gGameState.hudChartLabel == 0)
        {
            u8 n = gGameState.hudRightSlide;

            // The price digits and item tile reuse i (the original keeps them in i's register).
            map = (u16 *)0x06006440 + (0x15 - n);
            switch (n)
            {
            case 0:
                i = (u8)(gGameState.items[gGameState.selectedItem].price % 10) * 2;
                map[0] = i + 0x4030;
                map[0x20] = i + 0x4031;
                break;
            case 1:
                i = (u8)(gGameState.items[gGameState.selectedItem].price / 10) * 2;
                if (i != 0)
                {
                    map[0] = i + 0x4030;
                    map[0x20] = i + 0x4031;
                }
                else
                {
                    map[0] = 0xD09F;
                    map[0x20] = 0xD09F;
                }
                break;
            case 2:
                i = gGameState.items[gGameState.selectedItem].tile;
                map[0] = i + 1;
                map[0x20] = i + 1 + 0x10;
                break;
            case 3:
                i = gGameState.items[gGameState.selectedItem].tile;
                map[0] = i;
                map[0x20] = i + 0x10;
                break;
            }
            map++;
            tile = 0xE01B;
            for (i = gGameState.hudRightSlide; i != 0; i--)
            {
                map[0] = tile;
                map[0x20] = tile | 0x800;
                tile ^= 0x400;
                map++;
            }
        }
        else
        {
            map = (u16 *)0x06006464;
            tile = 0xB080;
            for (i = 4; i != gGameState.hudRightSlide; i--)
            {
                map[0] = tile;
                map[0x20] = tile + 0x10;
                tile++;
                map++;
            }
            tile = 0xE01B;
            for (; i != 0; i--)
            {
                map[0] = tile;
                map[0x20] = tile | 0x800;
                tile ^= 0x400;
                map++;
            }
        }
    }
}
