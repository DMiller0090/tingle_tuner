#include "global.h"
#include "main.h"
#include "game.h"
#include "sound.h"
#include "gba_com.h"
#include "gba/syscall.h"

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

INCLUDE_ASM(Func_0201DEAC);

INCLUDE_ASM(Func_0201E190);

INCLUDE_ASM(Func_0201E2FC);

INCLUDE_ASM(SendItemUse);

INCLUDE_ASM(Func_0201E384);

INCLUDE_ASM(Func_0201E584);

INCLUDE_ASM(Func_0201E5E0);

INCLUDE_ASM(Func_0201EBAC);

INCLUDE_ASM(Func_0201EBD4);

INCLUDE_ASM(Func_0201EC10);

INCLUDE_ASM(Func_0201ECC4);

INCLUDE_ASM(Func_0201ED08);

INCLUDE_ASM(Func_0201EDC4);

INCLUDE_ASM(Func_0201EDF8);

INCLUDE_ASM(Func_0201EF98);

INCLUDE_ASM(Func_0201F068);

INCLUDE_ASM(Func_0201F240);

INCLUDE_ASM(Func_0201F274);

INCLUDE_ASM(Func_0201F2A8);

INCLUDE_ASM(Func_0201F2C4);

void Func_0201F308(void)
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
                    GamePlaySound(0x2C, 1);
                    gGameState.effect[0] = gGameState.effect[1];
                    gGameState.unk120 = 0;
                    gGameState.unk132 = 8;
                    gGameState.unk12B = 3;
                }
                else if (gGameState.effect[1] == 0x13)
                {
                    SendItemUse(0x13, gGameState.unk164[5].unk2);
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
            GamePlaySound(0x30, 1);
            Func_0201EBAC();
        }
    }
}

void Func_0201F420(void)
{
    if (gMain.random & 1)
        GamePlaySound(0x3A, 14);
    else
        GamePlaySound(0x3B, 14);
    gGameState.unk1E2 = 0;
}

// Item effect 7, the shop: gGameState.unk1E2 steps through choosing an item, sending the purchase request
// (slot 12) and waiting for the GC's answer (slot 13).
void EffectShop(void)
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
                GamePlaySound(0x3C, 14);
                gGameState.unk1E2 = 4;
            }
            else if (gGameState.effect[1])
            {
                GamePlaySound(0x40, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[2])
            {
                GamePlaySound(0x3F, 14);
                gGameState.unk1E2 = 1;
            }
            else if (gGameState.effect[3])
            {
                GamePlaySound(0x3E, 14);
                gGameState.unk1E2 = 1;
            }
            else
            {
                GamePlaySound(0x3C, 14);
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
                    GamePlaySound(0x3D, 1);
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
                GamePlaySound(0x48, 14);
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
                    GamePlaySound(0x44, 14);
                else if (what == 1)
                    GamePlaySound(0x42, 14);
                else
                    GamePlaySound(0x49, 14);
            }
            else
            {
                gGameState.unk13C = 0;
                GamePlaySound(0x46, 14);
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
            GamePlaySound(0x3D, 1);
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
    gGameState.unk12E = gSprite_0202C928[4];
    gGameState.unk12F = 0;
    GamePlaySound(0x27, 1);
}

// Draws the title sprite at (x, y) on BG3's map, bobbing; kind 1 adds gSprite_0202C490 with a
// palette per effect, kind 2 flips it and adds gSprite_0202C928 at (unk036, unk038).
// Doesn't match: the original keeps x, y and bob in registers (spilling copies of kind and
// mosaic instead) and loads BG3's scroll as a folded gMain+0x432 constant in the first and
// last calls; this keeps the address in a register:
//     s16 bob = gBobOffsets_020244DA[(gMain.frameCount & 0x30) >> 4];
//     u8 mosaic = gMain.mosaic != 0;
//     u8 flags = 0;
//     u8 palette;
//
//     if (kind == 2)
//         flags = 8;
//     DrawSpriteAt(gSprite_0202C938, 0, (s16)x - gMain.bgOfs[3][0] + bob,
//                  (s16)y - gMain.bgOfs[3][1], mosaic, flags, 1);
//     if (kind == 1)
//     {
//         if (gGameState.effect[0] == 8)
//             palette = 8;
//         else if (gGameState.effect[0] == 9)
//             palette = 15;
//         else
//             palette = 4;
//         DrawSprite(gSprite_0202C490, 0, (s16)x - gMain.bgOfs[3][0] + (bob + 11),
//                    (s16)y - gMain.bgOfs[3][1] + 26, 0xC0, mosaic, 0, 0, 0, 0, palette);
//     }
//     else if (kind == 2)
//     {
//         DrawSpriteAt(gSprite_0202C928, 0, gGameState.unk036 - gMain.bgOfs[3][0] + bob,
//                      gGameState.unk038 - gMain.bgOfs[3][1], mosaic, 16, 1);
//     }
INCLUDE_ASM(Func_0201F8C4);

// Item effects 8-10: the title sprite drops in over the type 1 marker and sends the item use.
// Doesn't match by one instruction: in state 2 the original stores a fresh 0 to unk120 where this
// reuses the register holding unk132 (known to be 0):
//     void Func_0201FA2C(void)
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
//                 if (gGameState.unk134 == 0)
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
//                 GamePlaySound(0x28, 1);
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
//                     arg = gGameState.unk164[5].unk2;
//                 else
//                     arg = gGameState.unk164[idx + 8].unk2;
//                 SendItemUse(idx + 8, arg);
//                 gGameState.unk034 = y;
//                 gGameState.unk12B = 5;
//             }
//             else
//             {
//                 Func_02021284(1, 1, 0);
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
//             if (gGameState.unk034 < (s16)main->bgOfs[3][1] - 0x30)
//             {
//                 gGameState.unk12B = 0;
//                 if (gGameState.unk1DE == 5)
//                     GamePlaySound(0x30, 1);
//                 else
//                     GamePlaySound(gGameState.effect[0] + 0x10, 1);
//                 Func_0201EBAC();
//             }
//         }
//     }
//
INCLUDE_ASM(Func_0201FA2C);

void Func_0201FCA0(void)
{
    if (!gGameState.unk1E5_3)
        GamePlaySound(0xD2, 1);
    else
        GamePlaySound(0xDB, 1);
    gGameState.effect[0] = 11;
    gGameState.unk1E5_2 = 1;
    gGameState.unk1E5_1 = 0;
    gGameState.unk1E2 = 0;
}

// Item effect 11.
void Func_0201FD04(void)
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
            GamePlaySound(0xD8, 1);
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
                    GamePlaySound(0xD4, 1);
                else
                    GamePlaySound(0xDD, 1);
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
            gGameState.unk164[10].unk4 = (gGameState.unk164[10].unk4 & 0xF000) + 0xAE;
            Func_0201DCEC(gGameState.unk1DE);
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
void Func_0201FF14(void)
{
    struct Main *main = &gMain;
    s32 m = FindMarker(1);
    u16 x, y;

    if (gGameState.unk12B == 3)
    {
        if (gGameState.unk132 == 0)
        {
            if (gGameState.unk134 == 0)
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
            SendItemUse(gGameState.effect[0], gGameState.unk164[5].unk2);
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
            Func_0201EF98(x, y);
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
            Func_0201EF98(x, y);
        }
    }
    else if (gGameState.unk12B == 7)
    {
        gGameState.unk034--;
        gGameState.unk038--;
        gGameState.unk032 = gGameState.markers[m].x - 0x15;
        gGameState.unk036 = gGameState.markers[m].x + 0x11;
        Func_0201F8C4(gGameState.unk032, gGameState.unk034, 2);
        if (gGameState.unk034 < (s16)main->bgOfs[3][1] - 0x30)
        {
            GamePlaySound(0x30, 1);
            gGameState.unk12B = 0;
            Func_0201EBAC();
        }
        if (gGameState.unk038 < (s16)main->bgOfs[3][1] - 0x30)
            gGameState.unk038 = main->bgOfs[3][1] - 0x30;
    }
}

void Func_02020150(void)
{
    GamePlaySound(0x50, 1);
    gGameState.unk1C8 = 300;
    gGameState.unk228 = 0;
    gGameState.unk226 = 5;
    gGameState.unk227 = 0;
}

INCLUDE_ASM(Func_02020190);

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
    Func_0201BAD8();
    gGameState.unk144 = MSG_DATA + *(u16 *)(MSG_DATA + *(u16 *)MSG_DATA + 0x3E);
    gGameState.unk14A = 6;
    gGameState.unk14B = 0;
    gGameState.unk157 = 6;
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
    Func_02021284(2, 2, 0);
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
        x = (obj->x >> 8) - gMain.bgOfs[3][0];
        y = ((obj->y + obj->z) >> 8) - gMain.bgOfs[3][1];
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

void Func_020208FC(u8 open)
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

void Func_020209A0(void)
{
    DrawSprite(gSprite_0202BD78, 0, gGameState.unk1E8, gGameState.unk1EA, 0, 0, 0, 0, 0, 0, 0);
}

INCLUDE_ASM(Func_020209E0);

void Func_02020BEC(void)
{
    gGameState.unk1E8 = 0xF8;
    gGameState.unk1EA = 0x50;
    gGameState.unk1EE = 0;
}

INCLUDE_ASM(Func_02020C10);
