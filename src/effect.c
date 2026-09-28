#include "global.h"
#include "gba/syscall.h"
#include "game.h"
#include "gba_com.h"
#include "main.h"
#include "sound.h"

// Game code object 5 (GZLJ01 0201EBAC-02020E28): the item effects, mode 2 (link lost), the
// object system, the wave scroll, the Select window and the status icon. See docs/layout.md.

const u8 gShopRequests[9] =
{
    0x2, 0x1, 0x14, 0x1, 0xA, 0x28, 0x0, 0xA, 0x32
};

const s16 gBobOffsets_020244DA[4] =
{
    0, -1, 0, 1
};

void (*const gItemEffectFuncs[18])(void) =
{
    Func_0201F2A8,
    Func_0201F2A8,
    Func_0201EC10,
    Func_0201F068,
    Func_0201F068,
    Effect5,
    Func_0201F2A8,
    Effect7,
    Effect8,
    Effect8,
    Effect8,
    Effect11,
    Effect12,
    Func_0201F068,
    Effect14,
    Func_0201EC10,
    Func_0201F2A8,
    Func_0201EC10
};

void (*const gMode2StateFuncs[2])(void) =
{
    Mode2State0,
    Mode2State1
};

void (*const gObjFuncs[1])(struct Obj *obj) =
{
    ObjBurst
};

const s16 gObjBurstOfsX[5] =
{
    0, 0, 0, 0, 0
};

const s16 gObjBurstOfsY[5] =
{
    0, 0, 0, 0, 0
};

const s16 gObjBurstVelX[5] =
{
    -32, -23, 23, 32, 0
};

const s16 gObjBurstVelY[5] =
{
    0, -23, -23, 0, -32
};

void (*const gObjBurstStateFuncs[2])(struct Obj *obj) =
{
    ObjBurstInit,
    ObjBurstFly
};

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
    ObjSpawn(0);
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
        DrawSpriteParts(gSprite_0202CC60, gGameState.markers[i].x - gMain.bgOfs[3][0],
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
    const struct SpriteFrame *anim;

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
    ShowMessage(0x23, 1);
}

// Draw the item effect sprite (kind 0: gSprite_0202C928, else gSprite_0202C940) at a map
// position. The scroll is subtracted as an s16, which keeps the arithmetic in int (x and y
// get sign-extended after the scroll load).
void Func_0201EDF8(s16 x, s16 y, u8 kind)
{
    u32 mosaic = gMain.mosaic != 0;
    const struct SpriteFrame *anim;
    u8 flip;

    if (kind == 0)
    {
        if (gGameState.unk12E == 0)
        {
            gGameState.unk12F ^= 1;
            gGameState.unk12E = gSprite_0202C928[gGameState.unk12F].duration;
            anim = gSprite_0202C928;
        }
        else
        {
            gGameState.unk12E--;
            anim = gSprite_0202C928;
        }
        if (gGameState.effect[0] == 12)
            flip = 16;
        else
            flip = 0;
        DrawSpriteAt(anim, gGameState.unk12F, x - gMain.bgOfs[3][0] + 3,
                     y - gMain.bgOfs[3][1] - 24, mosaic, flip, 1);
    }
    else
    {
        if (gGameState.unk130 == 0)
        {
            gGameState.unk131 ^= 1;
            gGameState.unk130 = gSprite_0202C940[gGameState.unk131].duration;
            anim = gSprite_0202C940;
        }
        else
        {
            gGameState.unk130--;
            anim = gSprite_0202C940;
        }
        if (gGameState.effect[0] == 12)
            flip = 8;
        else
            flip = 0;
        if (gGameState.unk131 == 0)
            DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] - 4,
                         y - gMain.bgOfs[3][1], mosaic, flip, 1);
        else
            DrawSpriteAt(anim, gGameState.unk131, x - gMain.bgOfs[3][0] + 4,
                         y - gMain.bgOfs[3][1], mosaic, flip, 1);
    }
}

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
                ShowMessage(0x30, 1);
            else if (!notType3)
                ShowMessage(0x2E, 1);
            else
                ShowMessage(0x2F, 1);
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
    ShowMessage(0x25, 1);
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
        ShowMessage(0x29, 1);
    }
    else if (gGameState.effect[1] == 12)
    {
        ShowMessage(0x2A, 1);
    }
    else
    {
        ShowMessage(0x2B, 1);
    }
}

// Item effect 5.
void Effect5(void)
{
    Func_0201ED08();
    if (gGameState.unk12B == 2)
    {
        if (gGameState.unk1FA == 0)
        {
            if (gGameState.unk132 == 0)
            {
                if (gGameState.effect[1] == 10)
                {
                    ShowMessage(0x2C, 1);
                    gGameState.effect[0] = gGameState.effect[1];
                    gGameState.unk120 = 0;
                    gGameState.unk132 = 8;
                    gGameState.unk12B = 3;
                }
                else if (gGameState.effect[1] == 0x13)
                {
                    SendItemUse(0x13, gGameState.items[5].price);
                    gGameState.unk12B = 5;
                }
                else
                {
                    gGameState.effect[0] = gGameState.effect[1];
                    Func_0201FEEC();
                    gGameState.unk12B = 3;
                }
            }
            else
            {
                gGameState.unk132--;
            }
        }
    }
    else if (gGameState.unk12B == 5)
    {
    }
    else if (gGameState.unk12B == 6)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            gGameState.unk120 = 0;
            ShowMessage(0x30, 1);
            Func_0201EBAC();
        }
    }
}

void Func_0201F420(void)
{
    if (gMain.random & 1)
        ShowMessage(0x3A, 14);
    else
        ShowMessage(0x3B, 14);
    gGameState.unk1E2 = 0;
}

// Item effect 7, the shop: gGameState.unk1E2 steps through choosing an item, sending the purchase request
// (slot 12) and waiting for the GC's answer (slot 13).
void Effect7(void)
{
    u16 item;
    u8 what, result;

    if (gGameState.flags.unk3_2)
    {
        Func_0201EBAC();
        return;
    }
    if (gGameState.unk1E2 == 0)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.unk13C = 0;
            if ((*(u32 *)gGameState.effect & 0xFFFFFF00) == 0)
            {
                ShowMessage(0x3C, 14);
                gGameState.unk1E2 = 4;
            }
            else if (gGameState.effect[1])
            {
                ShowMessage(0x40, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[2])
            {
                ShowMessage(0x3F, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[3])
            {
                ShowMessage(0x3E, 14);
                gGameState.unk1E2 = 1;
            }
            else
            {
                ShowMessage(0x3C, 14);
                gGameState.unk1E2 = 4;
            }
        }
    }
    else if (gGameState.unk1E2 == 1)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk142 == 0x3E)
            {
                if (gGameState.unk14F == 1)
                    gGameState.unk1E2 = 5;
                else
                    gGameState.unk1E2 = 2;
            }
            else if (gGameState.unk142 == 0x3F || gGameState.unk142 == 0x40)
            {
                if (gGameState.unk14F == 2)
                    gGameState.unk1E2 = 5;
                else
                    gGameState.unk1E2 = 2;
            }
        }
    }
    else if (gGameState.unk1E2 == 2)
    {
        item = gGameState.unk142;
        if (item >= 0x41 && item <= 0x45)
        {
            if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
            {
                if (gGameState.unk14F == 0)
                {
                    if (gGameState.unk142 == 0x41)
                        what = 0;
                    else if (gGameState.unk142 == 0x43)
                        what = 1;
                    else
                        what = 2;
                    GBACOM_BUF_SHOP[0] = gShopRequests[what * 3];
                    GBACOM_BUF_SHOP[1] = gShopRequests[what * 3 + 1];
                    GBACOM_BUF_SHOP[2] = gShopRequests[what * 3 + 2];
                    GbaComSendDataSet(GBACOM_BUF_SHOP, 4, GBACOM_SLOT_SHOP, *(u32 *)GBACOM_BUF_SHOP);
                    gGameState.unk1E2 = 3;
                    gMain.newKeys ^= 1;
                }
                else
                {
                    gGameState.unk13C = 0;
                    ShowMessage(0x3D, 1);
                    gGameState.unk1E2 = 5;
                }
            }
        }
    }
    else if (gGameState.unk1E2 == 3)
    {
        if (GbaComRecvStatusCheck(GBACOM_SLOT_ITEM_BUY))
        {
            what = GBACOM_BUF_ITEM_BUY[0];
            result = GBACOM_BUF_ITEM_BUY[1];
            GbaComDataStatusReset(GBACOM_SLOT_ITEM_BUY);
            if (result == 0)
            {
                gGameState.unk13C = 0;
                ShowMessage(0x48, 14);
                gGameState.unk1E2 = 4;
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
            }
            else if (result == 1)
            {
                gMain.newKeys |= 1;
                gGameState.unk1E2 = 4;
            }
            else if (result == 2)
            {
                gGameState.unk13C = 0;
                gGameState.unk1E2 = 4;
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                if (what == 0)
                    ShowMessage(0x44, 14);
                else if (what == 1)
                    ShowMessage(0x42, 14);
                else
                    ShowMessage(0x49, 14);
            }
            else
            {
                gGameState.unk13C = 0;
                ShowMessage(0x46, 14);
                gGameState.unk151 = 0;
                gGameState.unk150 = 0;
                gGameState.unk14F = 0xFF;
                gGameState.unk14E = 0;
                gGameState.unk1E2 = 4;
            }
        }
        else
        {
            gMain.newKeys &= 0xFF3E;
        }
    }
    else if (gGameState.unk1E2 == 4)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.unk13C = 0;
            ShowMessage(0x3D, 1);
            gGameState.unk1E2 = 5;
        }
    }
    else
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
            Func_0201EBAC();
    }
}

void Func_0201F878(void)
{
    Func_0201ECC4();
    gGameState.unk1E5_1 = 0;
    gGameState.unk1E5_2 = 1;
    gGameState.unk12E = gSprite_0202C928[0].duration;
    gGameState.unk12F = 0;
    ShowMessage(0x27, 1);
}

// Draws the title sprite at (x, y) on BG3's map, bobbing; kind 1 adds gSprite_0202C490 with a
// palette per effect, kind 2 flips it and adds gSprite_0202C928 at (unk036, unk038). One
// variable holds the flip flags and then the palette.
void Func_0201F8C4(s16 x, s16 y, u8 kind)
{
    s16 bob = gBobOffsets_020244DA[(gMain.frameCount & 0x30) >> 4];
    u8 mosaic = gMain.mosaic != 0;
    u8 flags = 0;

    if (kind == 2)
        flags = 8;
    DrawSpriteAt(gSprite_0202C938, 0, x - gMain.bgOfs[3][0] + bob,
                 y - gMain.bgOfs[3][1], mosaic, flags, 1);
    if (kind == 1)
    {
        if (gGameState.effect[0] == 8)
            flags = 8;
        else if (gGameState.effect[0] == 9)
            flags = 15;
        else
            flags = 4;
        DrawSprite(gSprite_0202C490, 0, x - gMain.bgOfs[3][0] + (s16)(bob + 11),
                   y - gMain.bgOfs[3][1] + 26, 0xC0, mosaic, 0, 0, 0, 0, flags);
    }
    else if (kind == 2)
    {
        flags = 16;
        DrawSpriteAt(gSprite_0202C928, 0, gGameState.unk036 - gMain.bgOfs[3][0] + bob,
                     gGameState.unk038 - gMain.bgOfs[3][1], mosaic, flags, 1);
    }
}

// Item effects 8-10: the title sprite drops in over the type 1 marker and sends the item use.
// Doesn't match by one instruction: in state 2 the original stores a fresh 0 to unk120 where this
// reuses the register holding unk132 (known to be 0). cse does that: byte stores go through
// store_bit_field as (and mem 0), which folds to 0 and finds unk132's register in the jump
// equivalence. Tried: !x, <1, u32/s8 casts, a local or pointer for unk132, != with swapped arms,
// switch, goto, volatile/bitfield/cast stores to unk120, storing unk1FA or unk132*0, the unk1FA
// test written other ways, a temp reassigned to 8 before the store. The cse1 dump shows the
// store as (subreg:QI (reg:SI unk132)): the jump equivalence is SImode and cse's narrower-mode
// constant lookup finds it; any u8 field store of 0 after `== 0` does this (a two-field test
// file gives `strb r0` from the tested reg). Also tried (2026-09-28): while/for/do-while/switch/
// goto forms, the test as & 0xFF, << 24, u16/s16 casts, + 1 == 1, == unk1FA, (x == 0) == 1;
// storing idx >> 8, effect[0] >> 8, & 0x100 (cse or combine doesn't fold them to a fresh 0);
// `s32 z = unk132 == 0; if (z)` does give a fresh `movs #0` but keeps a real scc sequence:
//     void Effect8(void)
//     {
//         struct Main *main = &gMain;
//         s32 m = FindMarker(1);
//         u8 idx = gGameState.effect[0] - 8;
//         u16 x, y;
//         u8 arg;
//
//         if (gGameState.unk12B <= 3)
//             Func_0201ED08();
//         if (gGameState.unk12B == 2)
//         {
//             if (gGameState.unk1FA == 0)
//             {
//                 if (gGameState.unk132 == 0)
//                 {
//                     gGameState.unk120 = 0;
//                     gGameState.unk132 = 8;
//                     gGameState.unk12B = 3;
//                     gGameState.flags.isFree = 0;
//                     gGameState.unk1E4 = gGameState.effect[0];
//                     SendItemUse(0x14, 0);
//                 }
//                 else
//                 {
//                     gGameState.unk132--;
//                 }
//             }
//         }
//         else if (gGameState.unk12B == 3)
//         {
//             if (gGameState.unk132 == 0)
//             {
//                 if (gGameState.scrolling == 0)
//                 {
//                     gGameState.unk034 = main->bgOfs[3][1] - 0x30;
//                     gGameState.unk12B = 4;
//                 }
//             }
//             else
//             {
//                 gGameState.unk132--;
//                 gGameState.flags.isFree = 0;
//             }
//         }
//         else if (gGameState.unk12B == 4)
//         {
//             if (main->frameCount & 1)
//                 gGameState.unk034++;
//             if (gGameState.markers[m].y - gGameState.unk034 <= 0x18)
//             {
//                 gGameState.unk132 = (idx + 2) * 60;
//                 gGameState.unk034 = gGameState.markers[m].y - 0x18;
//                 gGameState.unk12B = 6;
//                 ShowMessage(0x28, 1);
//             }
//             Func_0201F8C4(gGameState.markers[m].x - 0x18, gGameState.unk034, 1);
//         }
//         else if (gGameState.unk12B == 6)
//         {
//             x = gGameState.markers[m].x - 0x18;
//             y = gGameState.markers[m].y - 0x18;
//             Func_0201F8C4(x, y, 1);
//             if (gGameState.unk132 == 0)
//             {
//                 if (gGameState.unk1DE == 5)
//                     arg = gGameState.items[5].price;
//                 else
//                     arg = gGameState.items[idx + 8].price;
//                 SendItemUse(idx + 8, arg);
//                 gGameState.unk034 = y;
//                 gGameState.unk12B = 5;
//             }
//             else
//             {
//                 SoundPlayLoop(1, 1, 0);
//                 if (--gGameState.unk132 == 10)
//                     gGameState.unk13E = 300;
//             }
//         }
//         else if (gGameState.unk12B == 5)
//         {
//             x = gGameState.markers[m].x - 0x18;
//             y = gGameState.markers[m].y - 0x18;
//             Func_0201F8C4(x, y, 1);
//         }
//         else if (gGameState.unk12B == 7)
//         {
//             gGameState.unk034--;
//             Func_0201F8C4(gGameState.markers[m].x - 0x18, gGameState.unk034, 0);
//             if (gGameState.unk034 < main->bgOfs[3][1] - 0x30)
//             {
//                 gGameState.unk12B = 0;
//                 if (gGameState.unk1DE == 5)
//                     ShowMessage(0x30, 1);
//                 else
//                     ShowMessage(gGameState.effect[0] + 0x10, 1);
//                 Func_0201EBAC();
//             }
//         }
//     }
//
INCLUDE_ASM(Effect8);

void Func_0201FCA0(void)
{
    if (!gGameState.unk1E5_3)
        ShowMessage(0xD2, 1);
    else
        ShowMessage(0xDB, 1);
    gGameState.effect[0] = 11;
    gGameState.unk1E5_2 = 1;
    gGameState.unk1E5_1 = 0;
    gGameState.unk1E2 = 0;
}

// Item effect 11.
void Effect11(void)
{
    if (gGameState.unk1E2 == 0)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk14F == 0)
                gGameState.unk1E2 = 2;
            else if (gGameState.unk14F == 1)
                gGameState.unk1E2 = 1;
            else
                gGameState.unk1E2 = 4;
        }
    }
    else if (gGameState.unk1E2 == 1)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
            gGameState.unk1E2 = 2;
    }
    else if (gGameState.unk1E2 == 2)
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            ShowMessage(0xD8, 1);
            gGameState.unk1E2 = 3;
        }
    }
    else if (gGameState.unk1E2 == 3)
    {
        if ((gGameState.unk14B == 1 || gGameState.unk14B == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.unk14F == 1)
            {
                gMain.newKeys ^= 1;
                if (!gGameState.unk1E5_3)
                    ShowMessage(0xD4, 1);
                else
                    ShowMessage(0xDD, 1);
                gGameState.unk1E2 = 1;
            }
            else
            {
                gGameState.unk1E2 = 4;
            }
        }
    }
    else
    {
        if ((gGameState.unk14B == 3 && (gMain.newKeys & 0x30F)) || gGameState.unk13E > 0x12A)
        {
            gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0xAE;
            SetSelectedItem(gGameState.unk1DE);
            Func_0201EBAC();
        }
    }
}

void Func_0201FEEC(void)
{
    gGameState.unk132 = 8;
    gGameState.unk120 = 0;
    gGameState.unk12B = 4;
}

// Item effect 12.
void Effect12(void)
{
    struct Main *main = &gMain;
    s32 m = FindMarker(1);
    u16 x, y;

    if (gGameState.unk12B == 3)
    {
        if (gGameState.unk132 == 0)
        {
            if (gGameState.scrolling == 0)
            {
                gGameState.unk038 = gGameState.unk034 = main->bgOfs[3][1] - 0x30;
                gGameState.unk12B = 4;
            }
        }
        else
        {
            gGameState.unk132--;
        }
    }
    else if (gGameState.unk12B == 4)
    {
        if (main->frameCount & 1)
        {
            gGameState.unk034++;
            gGameState.unk038++;
        }
        if (gGameState.markers[m].y - gGameState.unk034 <= 0x18)
        {
            gGameState.unk034 = gGameState.markers[m].y - 0x18;
            gGameState.unk038 = gGameState.markers[m].y - 0x18;
            SendItemUse(gGameState.effect[0], gGameState.items[5].price);
            gGameState.unk12B = 5;
        }
        x = gGameState.markers[m].x - 0x15;
        gGameState.unk036 = gGameState.markers[m].x + 0x11;
        Func_0201F8C4(x, gGameState.unk034, 2);
    }
    else if (gGameState.unk12B == 5)
    {
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (gGameState.flags.timer)
        {
            gGameState.unk1C8 = 120;
            gGameState.unk12B = 6;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 6)
    {
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        Func_0201EDF8(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (!gGameState.flags.timer)
        {
            gGameState.unk032 = gGameState.markers[m].x - 0x15;
            gGameState.unk036 = gGameState.markers[m].x + 0x11;
            gGameState.unk034 = gGameState.markers[m].y - 0x18;
            gGameState.unk038 = gGameState.markers[m].y - 0x18;
            gGameState.unk12B = 7;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.unk12B == 7)
    {
        gGameState.unk034--;
        gGameState.unk038--;
        gGameState.unk032 = gGameState.markers[m].x - 0x15;
        gGameState.unk036 = gGameState.markers[m].x + 0x11;
        Func_0201F8C4(gGameState.unk032, gGameState.unk034, 2);
        if (gGameState.unk034 < main->bgOfs[3][1] - 0x30)
        {
            ShowMessage(0x30, 1);
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
        if (gGameState.unk038 < main->bgOfs[3][1] - 0x30)
            gGameState.unk038 = main->bgOfs[3][1] - 0x30;
    }
}

void Func_02020150(void)
{
    ShowMessage(0x50, 1);
    gGameState.unk1C8 = 300;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

// Item effect 14: a countdown (unk1C8) that ticks faster near the end, drawn at the type 3
// marker; at 0 it switches to effect 2.
void Effect14(void)
{
    u8 mosaic = gMain.mosaic != 0;
    s32 m;

    if (gGameState.flags.busy & 1)
        gGameState.unk1C8++;
    if (gGameState.unk1EE == 0)
    {
        gGameState.unk13E = 0;
        if (gGameState.unk1C8 > 179)
        {
            if ((u16)(gGameState.unk1C8 % 30) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.unk1C8 > 59)
        {
            if ((u16)(gGameState.unk1C8 % 20) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.unk1C8 == 0)
        {
            gGameState.unk13E = 300;
            gGameState.effect[0] = 2;
            Func_0201EBD4();
        }
        else if ((u16)(gGameState.unk1C8 % 10) == 0)
        {
            SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        m = FindMarker(3);
        DrawSpriteParts(gSprite_0202CC50, gGameState.markers[m].x - gMain.bgOfs[3][0],
                      gGameState.markers[m].y - gMain.bgOfs[3][1], mosaic, 0);
    }
}

// Run the current item effect (effect[0], from slot 7) while one is active.
void UpdateItemEffect(void)
{
    if (gGameState.unk1E5_2)
    {
        if (!gGameState.unk1E5_1)
        {
            if (gGameState.unk1C8 != 0)
                gGameState.unk1C8--;
            gItemEffectFuncs[gGameState.effect[0]]();
        }
    }
}

// Link lost: load the title graphics with the "link lost" BG map and fade in (state 1).
void Mode2State0(void)
{
    u8 blend;

    REG_DISPCNT = 0x80;  // forced blank
    gMain.dispcnt = 0x1100;  // BG0 and OBJ
    gMain.bgcnt[0] = 0x4C08;
    LZ77UnCompVram(gMode1BgTiles, (void *)0x06008000);
    LZ77UnCompVram(gMode2BgMap, (void *)0x06006000);
#ifndef VERSION_GZLJ01
    TextClearCanvas();
    gGameState.msgPtr = MSG_DATA + *(u16 *)(MSG_DATA + *(u16 *)MSG_DATA + 0x3E);
    gGameState.unk14A = 6;
    gGameState.unk14B = 0;
    gGameState.glyphWidth = 6;
    gGameState.unk15A = 0;
    DrawString(gMode2UsString, 0, 0, 8);
#endif
    CpuCopy16(gMode1BgPalette, (void *)0x05000000, sizeof(gMode1BgPalette));
    LZ77UnCompVram(gMode1ObjTiles, (void *)0x06010000);
    CpuCopy16(gMode1ObjPalette, (void *)0x05000200, sizeof(gMode1ObjPalette));
    gMain.bgOfs[0][0] = 0;
    gMain.bgOfs[0][1] = 0;
    gMain.hblankScroll = 0;
    gMode0Work.blend = blend = 0;
    gMain.bldcnt = 0x2071;  // BG0 over BG0-3, OBJ and backdrop
    gMain.bldalpha = ((16 - blend) << 8) | blend;
    gGameState.flagsReceived = 0;
    gGameState.unk23C = 1;
    gMode0Work.timer = 1;
    gMode0Work.fadingOut = 0;
    gMain.modeState++;
}

// Pulse the blend (between 6 and 16, holding 3 s at full) until the link is back, then
// restart mode 1.
void Mode2State1(void)
{
    SoundPlayLoop(2, 2, 0);
    if (--gMode0Work.timer == 0)
    {
        gMode0Work.timer = 2;
        if (gMode0Work.fadingOut == 0)
        {
            gMode0Work.blend++;
            if (gMode0Work.blend == 16)
            {
                gMode0Work.timer = 180;
                gMode0Work.fadingOut ^= 1;
            }
        }
        else
        {
            gMode0Work.blend--;
            if (gMode0Work.blend == 6)
                gMode0Work.fadingOut ^= 1;
        }
        gMain.bldalpha = ((16 - gMode0Work.blend) << 8) | gMode0Work.blend;
    }
    if (gMain.linked)
    {
        gMain.mode = 1;
        gMain.modeState = 0;
    }
    DrawSpriteAt(gSprite_0202C848, 0, 0xD0, 0x80, 0, 0, 0);
}

void MainMode2(void)
{
    gMode2StateFuncs[gMain.modeState]();
}

// Slow vx toward 0 by decel (it can overshoot) and move x.
void ObjDecelX(struct Obj *obj, s16 decel)
{
    if (obj->vx >= 0)
        decel = -decel;
    if (obj->vx != 0)
        obj->vx += decel;
    obj->x += obj->vx;
}

// Slow vy toward 0 by decel (it can overshoot) and move y.
void ObjDecelY(struct Obj *obj, s16 decel)
{
    if (obj->vy >= 0)
        decel = -decel;
    if (obj->vy != 0)
        obj->vy += decel;
    obj->y += obj->vy;
}

// While in the air (z < 0 or moving), accelerate vz by gravity and land at z = 0.
void ObjFall(struct Obj *obj, s16 gravity)
{
    if (obj->vz | obj->z)
    {
        obj->vz += gravity;
        obj->z += obj->vz;
        if (obj->z >= 0)
        {
            obj->vz = 0;
            obj->z = 0;
        }
    }
}

void ObjReset(struct Obj *obj)
{
    obj->x = 0;
    obj->y = 0;
    obj->z = 0;
    obj->vx = 0;
    obj->vy = 0;
    obj->vz = 0;
    obj->unk12 = 0;
    obj->unk14 = 0;
    obj->unk16 = 0;
    obj->unk18 = 0;
    obj->unk1A = 0;
    obj->unk1C = 0;
    obj->unk1E = 0;
    obj->active = 0;
    obj->type = 0;
}

// Run every active object's type function.
void ObjUpdateAll(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            gObjFuncs[gObjs[i].type](&gObjs[i]);
    }
}

// Index of the first free object, or -1.
s32 ObjFindFree(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (!gObjs[i].active)
            return i;
    }
    return -1;
}

// Start a free object of the given type (its function sees active = 1 and sets it up).
bool32 ObjSpawn(u8 type)
{
    s32 i = ObjFindFree();

    if (i != -1)
    {
        gObjs[i].type = type;
        gObjs[i].active = 1;
        gObjFuncs[gObjs[i].type](&gObjs[i]);
        return TRUE;
    }
    return FALSE;
}

void ObjResetActive(void)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (gObjs[i].active)
            ObjReset(&gObjs[i]);
    }
}

// Place this piece and up to four more at the first type 3 marker, flying out in a fan.
void ObjBurstInit(struct Obj *obj)
{
    s16 ofsX = gObjBurstOfsX[obj->unk1C];
    s16 ofsY = gObjBurstOfsY[obj->unk1C];
    s32 i = 0;
    s32 idx;
    s32 j;

    do
    {
        if (gGameState.markers[i].type == 3)
            break;
        i++;
    } while (i < gGameState.markerCount);

    obj->x = (gGameState.markers[i].x << 8) + ofsX;
    obj->y = (gGameState.markers[i].y << 8) + ofsY;
    obj->unk1A = obj->unk18 = 0;
    obj->vx = gObjBurstVelX[obj->unk1C];
    obj->vy = gObjBurstVelY[obj->unk1C];
    obj->vz = -0x100;
    obj->unk1E = 0;
    obj->unk12 = 2;
    obj->unk16 = 0x180;
    obj->active++;

    for (j = 1; j <= 4; j++)
    {
        idx = ObjFindFree();
        if (idx != -1)
        {
            // BUG: indexes from obj, not gObjs (the same when obj is gObjs[0])
            obj[idx].unk1C = j;
            ofsX = gObjBurstOfsX[obj[idx].unk1C];
            ofsY = gObjBurstOfsY[obj[idx].unk1C];
            obj[idx].type = 0;
            obj[idx].active = 1;
            obj[idx].x = (gGameState.markers[i].x << 8) + ofsX;
            obj[idx].y = (gGameState.markers[i].y << 8) + ofsY;
            obj[idx].unk1A = obj[idx].unk18 = 0;
            obj[idx].vx = gObjBurstVelX[obj[idx].unk1C];
            obj[idx].vy = gObjBurstVelY[obj[idx].unk1C];
            obj[idx].vz = -0x100;
            obj[idx].unk1E = 0;
            obj[idx].unk12 = 2;
            obj[idx].unk16 = 0x180;
            obj[idx].active = 2;
        }
    }
}

// Fly, spinning, and scale with the vertical speed; vanish on landing.
void ObjBurstFly(struct Obj *obj)
{
    ObjDecelX(obj, 0);
    ObjDecelY(obj, 0);
    ObjFall(obj, 5);
    if (obj->z == 0)
        ObjReset(obj);
    obj->unk1E -= 16;
    if (obj->vz < 0)
        obj->unk16 = 0x80 - obj->vz;
    else
        obj->unk16 = obj->vz + 0x80;
}

// Object type 0: pieces flying out of the type 3 marker (spawned by Func_0201EBD4).
void ObjBurst(struct Obj *obj)
{
    u8 mosaic = (gMain.bgcnt[0] >> 6) & 1;
    s32 x, y;

    if (!(gGameState.flags.busy & 1))
        gObjBurstStateFuncs[obj->active - 1](obj);
    if (obj->active)
    {
        x = (obj->x >> 8) - (u16)gMain.bgOfs[3][0];  // the one unsigned read of the scroll
        y = ((obj->y + obj->z) >> 8) - (u16)gMain.bgOfs[3][1];
        DrawSprite(gSprite_0202C150, 0, x, y, obj->unk1E, mosaic, obj->unk16, obj->unk18,
                   obj->unk1A, 0, 0);
    }
}

// Fill a per-line BG0HOFS table: BG0's scroll plus a sine wave of the given amplitude (8.8),
// advancing the phase by 4 per line.
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude)
{
    s32 i;
    const s16 *sin;
    u16 hofs;

    i = 160;
    sin = gSinTable;
    hofs = *gMain.bgOfs[0];
    for (; i != 0; i--)
    {
        *table = hofs + ((sin[phase] * amplitude) >> 8);
        phase += 4;
        table++;
    }
}

void SelectWindowWipe(u8 open)
{
    s16 step;

    if (open)
        step = 16;
    else
        step = -16;
    gMain.winin = 0x1F;
    gGameState.unk1E8 = gGameState.unk1E8 + step;
    if (gGameState.unk1E8 >= 0xE8 && gGameState.unk1E8 <= 0xF8)
    {
        gMain.win0h = 0xEF;
    }
    else if (gGameState.unk1E8 + 8 == 0)
    {
        gMain.win0h = 0;
        gMain.winin = 0;
    }
    else
    {
        gMain.win0h = gGameState.unk1E8 + 8;
    }
    gMain.win0v = 0x9F;
    gMain.winout = 0;
    gMain.dispcnt |= 0x2000;
}

void SelectWindowDrawEdge(void)
{
    DrawSprite(gSprite_0202BD78, 0, gGameState.unk1E8, gGameState.unk1EA, 0, 0, 0, 0, 0, 0, 0);
}

// Select (in mode 1 state 2, when nothing else is up) wipes a window across the screen
// (SelectWindowWipe), flips BG0 to its other half behind it and wipes it back; Select again (or
// leaving state 2) reverses it. gGameState.unk1EE is the step.
void UpdateSelectWindow(void)
{
    switch (gGameState.unk1EE)
    {
    case 0:
        if (gMain.modeState == 2 && gGameState.unk1FA == 0 && gGameState.unk1CC == 0
         && !gGameState.unk1E5_4 && (gMain.newKeys & 4))
        {
            gGameState.unk1FB = 1;
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.unk1EE++;
        }
        break;
    case 1:
        SelectWindowWipe(0);
        if (gGameState.unk1E8 == 0xFFF8)
        {
            gGameState.unk1EE++;
            gMain.bgOfs[0][0] = 0x100;
        }
        SelectWindowDrawEdge();
        break;
    case 2:
        SelectWindowWipe(1);
        if (gGameState.unk1E8 == 0xF8)
        {
            gGameState.unk1EE++;
            gMain.dispcnt ^= 0x2000;
        }
        SelectWindowDrawEdge();
        break;
    case 3:
        // Two branches with the same body (cross-jumped), not one ||.
        if (gMain.modeState != 2)
        {
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.unk1EE++;
        }
        else if (gMain.newKeys & 4)
        {
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.unk1EE++;
        }
        break;
    case 4:
        SelectWindowWipe(0);
        if (gGameState.unk1E8 == 0xFFF8)
        {
            gGameState.unk1EE++;
            gMain.bgOfs[0][0] = 0;
        }
        SelectWindowDrawEdge();
        break;
    case 5:
        SelectWindowWipe(1);
        if (gGameState.unk1E8 == 0xF8)
        {
            gGameState.unk1EE = 0;
            gGameState.unk1FB = 0;
            gGameState.unk104 = 1;
            gMain.dispcnt &= 0xDFFF;
        }
        SelectWindowDrawEdge();
        break;
    }
}

void ResetSelectWindow(void)
{
    gGameState.unk1E8 = 0xF8;
    gGameState.unk1EA = 0x50;
    gGameState.unk1EE = 0;
}

// The status icon at (232, 128) and the blinking frame around it; gGameState.unk1F4 picks the
// icon.
void DrawStatusIcon(void)
{
    u8 blink = (gMain.frameCount >> 3) % 2;
    s32 m = FindMarker(1);
    u8 type;

    if (gGameState.unk1FB | gGameState.unk1FA | gGameState.unk224 | gGameState.unk1CC)
        return;
    if (gGameState.unk121 != 0 || gGameState.markers[m].unk6 == 4)
    {
        gGameState.unk1F4 = 0;
    }
    else if (gGameState.flags.unk8_7)
    {
        if (gGameState.flags.isFree)
            gGameState.unk1F4 = 1;
        else
            gGameState.unk1F4 = 0;
    }
    else if (gGameState.flags.isFree)
    {
        if (gGameState.unk1F5)
        {
            SoundPlayLoop(3, 3, 0);
            gGameState.unk1F4 = 2;
        }
        else
        {
            gGameState.unk1F4 = 1;
        }
    }
    else if (gGameState.flags.followTarget)
    {
        SoundPlayLoop(4, 4, 0);
        gGameState.unk1F4 = 1;
    }
    else if (gGameState.unk1F5)
    {
        SoundPlayLoop(3, 3, 0);
        gGameState.unk1F4 = 2;
    }
    else
    {
        gGameState.unk1F4 = 0;
    }

    if (gGameState.unk1F4 == 1)
    {
        type = gGameState.markers[FindMarker(1)].unk6;
        if (type)
            gGameState.unk1F4 = type + 2;
    }

    if (gGameState.unk1F4 == 0)
        return;
    if (gGameState.unk1F4 == 1)
        DrawSprite(gSprite_0202C420, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.unk1F4 == 2)
        DrawSprite(gSprite_0202C428, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.unk1F4 == 3)
        DrawSpriteAt(gSprite_0202C440, 0, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 4)
        DrawSpriteAt(gSprite_0202C448, gGameState.unk126, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 5)
        DrawSpriteAt(gSprite_0202C4A8, gGameState.unk12A, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.unk1F4 == 6)
        DrawSpriteAt(gSprite_0202C498, gGameState.unk128, 0xE8, 0x80, 0, 0, 0);
    DrawSprite(gSprite_0202C430, 0, 0xE0, blink + 0x80, 0, 0, 0, 0, 0, 0, 0);
}
