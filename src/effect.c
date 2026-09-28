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

const s16 gEffectBobOffsets[4] =
{
    0, -1, 0, 1
};

void (*const gItemEffectFuncs[18])(void) =
{
    EffectWaitTimer,
    EffectWaitTimer,
    EffectBombBlast,
    EffectBalloonShield,
    EffectBalloonShield,
    Effect5,
    EffectWaitTimer,
    Effect7,
    Effect8,
    Effect8,
    Effect8,
    Effect11,
    Effect12,
    EffectBalloonShield,
    Effect14,
    EffectBombBlast,
    EffectWaitTimer,
    EffectBombBlast
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

// Back to no item effect: items usable again, UpdateItemEffect idle, the view follows.
void EndItemEffect(void)
{
    gGameState.itemsEnabled = 1;
    gGameState.effectActive = 0;
    gGameState.viewLocked = 0;
}

// Item effects 2, 15 and 21 (a Tingle Bomb went off): 180 frames of shaking, the burst pieces
// and Tingle's animation 5.
void StartBombBlast(void)
{
    gGameState.effectTimer = 180;
    gGameState.tingleFrame = 0;
    gGameState.tingleAnim = 5;
    gGameState.tingleAnimPart = 0;
    ObjSpawn(0);
}

// Item effects 2, 15 and 17: shake the screen while effectTimer runs (held while the GC's
// menu is open) and draw the blast at the cursor.
void EffectBombBlast(void)
{
    u8 mosaic = gMain.mosaic != 0;
    u8 i;
    u8 unk;

    if (gGameState.flags.busy & 1)
        gGameState.effectTimer++;
    if (gGameState.effectTimer == 0)
    {
        EndItemEffect();
        return;
    }
    i = FindMarker(3);
    gGameState.shake = gGameState.effectTimer;
    unk = gGameState.selectWinStep;
    if (unk == 0)
        DrawSpriteParts(gBombBlastSprite, gGameState.markers[i].x - gMain.bgOfs[3][0],
                      gGameState.markers[i].y - gMain.bgOfs[3][1], mosaic, unk);
}

// Start the animation that opens items 3-5 and 8-10 (gEffectIntroSprite, mid-map).
void StartEffectIntro(void)
{
    gGameState.introFrame = 0;
    gGameState.viewLocked = 1;
    gGameState.spriteY = gGameState.spriteX = 0x44;
    gGameState.introTimer = gEffectIntroSprite[0].duration;
    gGameState.effectStep = 1;
}

// Step and draw it; after its last frame go to effectStep 2 and turn the message page.
void DrawEffectIntro(void)
{
    u32 mosaic = gMain.mosaic != 0;
    const struct SpriteFrame *anim;

    if (gGameState.introTimer == 0)
    {
        if (gGameState.introFrame < 22)
        {
            gGameState.introFrame++;
        }
        else if (gGameState.effectStep < 2)
        {
            gGameState.effectStep = 2;
            gGameState.effectDelay = 10;
            gGameState.msgWaitTimer = 300;
        }
        gGameState.introTimer = gEffectIntroSprite[gGameState.introFrame].duration;
        anim = gEffectIntroSprite;
    }
    else
    {
        gGameState.introTimer--;
        anim = gEffectIntroSprite;
    }
    DrawSpriteAt(anim, gGameState.introFrame, gGameState.spriteX, gGameState.spriteY, mosaic, 0, 1);
}

// GC answer 17 to item 3's request 0x11: effect 3, the balloon.
void StartBalloonEffect(void)
{
    StartEffectIntro();
    gGameState.balloonTimer = gBalloonSprite[0].duration;
    gGameState.balloonFrame = 0;
    ShowMessage(0x23, 1);
}

// Draw the item effect sprite (kind 0: gBalloonSprite, else gShieldSprite) at a map
// position. The scroll is subtracted as an s16, which keeps the arithmetic in int (x and y
// get sign-extended after the scroll load).
void DrawBalloonShield(s16 x, s16 y, u8 kind)
{
    u32 mosaic = gMain.mosaic != 0;
    const struct SpriteFrame *anim;
    u8 flip;

    if (kind == 0)
    {
        if (gGameState.balloonTimer == 0)
        {
            gGameState.balloonFrame ^= 1;
            gGameState.balloonTimer = gBalloonSprite[gGameState.balloonFrame].duration;
            anim = gBalloonSprite;
        }
        else
        {
            gGameState.balloonTimer--;
            anim = gBalloonSprite;
        }
        if (gGameState.effect[0] == 12)
            flip = 16;
        else
            flip = 0;
        DrawSpriteAt(anim, gGameState.balloonFrame, x - gMain.bgOfs[3][0] + 3,
                     y - gMain.bgOfs[3][1] - 24, mosaic, flip, 1);
    }
    else
    {
        if (gGameState.shieldTimer == 0)
        {
            gGameState.shieldFrame ^= 1;
            gGameState.shieldTimer = gShieldSprite[gGameState.shieldFrame].duration;
            anim = gShieldSprite;
        }
        else
        {
            gGameState.shieldTimer--;
            anim = gShieldSprite;
        }
        if (gGameState.effect[0] == 12)
            flip = 8;
        else
            flip = 0;
        if (gGameState.shieldFrame == 0)
            DrawSpriteAt(anim, gGameState.shieldFrame, x - gMain.bgOfs[3][0] - 4,
                         y - gMain.bgOfs[3][1], mosaic, flip, 1);
        else
            DrawSpriteAt(anim, gGameState.shieldFrame, x - gMain.bgOfs[3][0] + 4,
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

// Item effects 3, 4 and 13: after the intro send the item use, then show flags.timer (the
// GC's remaining time) over the player until it runs out.
void EffectBalloonShield(void)
{
    struct Main *main = &gMain;
    u8 notType3 = gGameState.effect[0] != 3;
    u8 price;
    u16 x;
    u16 y;

    if (gGameState.effectStep != 6)
        DrawEffectIntro();
    if (gGameState.effectStep == 2)
    {
        if (gGameState.inMessage == 0)
        {
            if (gGameState.effectDelay == 0)
            {
                if (gGameState.effect[0] == 13)
                    price = gGameState.items[5].price;
                else
                    price = gGameState.items[gGameState.effect[0]].price;
                SendItemUse(gGameState.effect[0], price);
                gGameState.effectStep = 5;
            }
            else
            {
                gGameState.effectDelay--;
            }
        }
    }
    else if (gGameState.effectStep == 5)
    {
        if (gGameState.flags.timer != 0)
        {
            gGameState.viewLocked = 0;
            gGameState.effectStep = 6;
        }
    }
    else if (gGameState.effectStep == 6)
    {
        u8 i = FindMarker(1);

        DrawBalloonShield(gGameState.markers[i].x, gGameState.markers[i].y, notType3);
        if (gGameState.flags.timer == 0)
        {
            gGameState.viewLocked = 1;
            gGameState.effectTimer = 120;
            gGameState.effectStep = 7;
            gGameState.spriteX = gGameState.markers[i].x - main->bgOfs[3][0];
            gGameState.spriteY = gGameState.markers[i].y - main->bgOfs[3][1];
            if (gGameState.selectedItem == 5)
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
    else if (gGameState.effectStep == 7)
    {
        if (gGameState.effectTimer == 0)
        {
            gGameState.viewLocked = 0;
            gGameState.effectStep = 0;
            EndItemEffect();
        }
    }
}

// GC answer 18 to item 4's request 0x12: effect 4, the shield.
void StartShieldEffect(void)
{
    StartEffectIntro();
    gGameState.shieldTimer = gShieldSprite[0].duration;
    gGameState.shieldFrame = 0;
    ShowMessage(0x25, 1);
}

// GC answer 16 to item 16 (A with nothing selected; GC MODE_LOOK_ATTENTION): Tingle's
// animation 5 for 120 frames.
void StartLookEffect(void)
{
    gGameState.effectTimer = 120;
    gGameState.tingleFrame = 0;
    gGameState.tingleAnim = 5;
    gGameState.tingleAnimPart = 0;
}

// Item effects 0, 1, 6 and 16: end when effectTimer runs out.
void EffectWaitTimer(void)
{
    if (gGameState.effectTimer == 0)
        EndItemEffect();
}

// Effect 5 (item 5): the GC picked a result (effect[1]); a message per result, and 13
// (the long shield) takes over at once.
void StartRandomEffect(void)
{
    StartEffectIntro();
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
    DrawEffectIntro();
    if (gGameState.effectStep == 2)
    {
        if (gGameState.inMessage == 0)
        {
            if (gGameState.effectDelay == 0)
            {
                if (gGameState.effect[1] == 10)
                {
                    ShowMessage(0x2C, 1);
                    gGameState.effect[0] = gGameState.effect[1];
                    gGameState.viewLocked = 0;
                    gGameState.effectDelay = 8;
                    gGameState.effectStep = 3;
                }
                else if (gGameState.effect[1] == 0x13)
                {
                    SendItemUse(0x13, gGameState.items[5].price);
                    gGameState.effectStep = 5;
                }
                else
                {
                    gGameState.effect[0] = gGameState.effect[1];
                    StartEffectDrop();
                    gGameState.effectStep = 3;
                }
            }
            else
            {
                gGameState.effectDelay--;
            }
        }
    }
    else if (gGameState.effectStep == 5)
    {
    }
    else if (gGameState.effectStep == 6)
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            gGameState.viewLocked = 0;
            ShowMessage(0x30, 1);
            EndItemEffect();
        }
    }
}

// Effect 7 (item 7, the shop): a random greeting.
void StartShopEffect(void)
{
    if (gMain.random & 1)
        ShowMessage(0x3A, 14);
    else
        ShowMessage(0x3B, 14);
    gGameState.dialogStep = 0;
}

// Item effect 7, the shop: gGameState.dialogStep steps through choosing an item, sending the
// purchase request (slot 12) and waiting for the GC's answer (slot 13).
void Effect7(void)
{
    u16 item;
    u8 what, result;

    if (gGameState.flags.cursorInZone)
    {
        EndItemEffect();
        return;
    }
    if (gGameState.dialogStep == 0)
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
        {
            gGameState.msgOpen = 0;
            if ((*(u32 *)gGameState.effect & 0xFFFFFF00) == 0)
            {
                ShowMessage(0x3C, 14);
                gGameState.dialogStep = 4;
            }
            else if (gGameState.effect[1])
            {
                ShowMessage(0x40, 14);
                gGameState.dialogStep = 1;
            }
            else if (gGameState.effect[2])
            {
                ShowMessage(0x3F, 14);
                gGameState.dialogStep = 1;
            }
            else if (gGameState.effect[3])
            {
                ShowMessage(0x3E, 14);
                gGameState.dialogStep = 1;
            }
            else
            {
                ShowMessage(0x3C, 14);
                gGameState.dialogStep = 4;
            }
        }
    }
    else if (gGameState.dialogStep == 1)
    {
        if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.msgId == 0x3E)
            {
                if (gGameState.choiceCursor == 1)
                    gGameState.dialogStep = 5;
                else
                    gGameState.dialogStep = 2;
            }
            else if (gGameState.msgId == 0x3F || gGameState.msgId == 0x40)
            {
                if (gGameState.choiceCursor == 2)
                    gGameState.dialogStep = 5;
                else
                    gGameState.dialogStep = 2;
            }
        }
    }
    else if (gGameState.dialogStep == 2)
    {
        item = gGameState.msgId;
        if (item >= 0x41 && item <= 0x45)
        {
            if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && (gMain.newKeys & 1))
            {
                if (gGameState.choiceCursor == 0)
                {
                    if (gGameState.msgId == 0x41)
                        what = 0;
                    else if (gGameState.msgId == 0x43)
                        what = 1;
                    else
                        what = 2;
                    GBACOM_BUF_SHOP[0] = gShopRequests[what * 3];
                    GBACOM_BUF_SHOP[1] = gShopRequests[what * 3 + 1];
                    GBACOM_BUF_SHOP[2] = gShopRequests[what * 3 + 2];
                    GbaComSendDataSet(GBACOM_BUF_SHOP, 4, GBACOM_SLOT_SHOP, *(u32 *)GBACOM_BUF_SHOP);
                    gGameState.dialogStep = 3;
                    gMain.newKeys ^= 1;
                }
                else
                {
                    gGameState.msgOpen = 0;
                    ShowMessage(0x3D, 1);
                    gGameState.dialogStep = 5;
                }
            }
        }
    }
    else if (gGameState.dialogStep == 3)
    {
        if (GbaComRecvStatusCheck(GBACOM_SLOT_ITEM_BUY))
        {
            what = GBACOM_BUF_ITEM_BUY[0];
            result = GBACOM_BUF_ITEM_BUY[1];
            GbaComDataStatusReset(GBACOM_SLOT_ITEM_BUY);
            if (result == 0)
            {
                gGameState.msgOpen = 0;
                ShowMessage(0x48, 14);
                gGameState.dialogStep = 4;
                gGameState.choiceMenu = 0;
                gGameState.choiceY = 0;
                gGameState.choiceCursor = 0xFF;
                gGameState.choiceIndex = 0;
            }
            else if (result == 1)
            {
                gMain.newKeys |= 1;
                gGameState.dialogStep = 4;
            }
            else if (result == 2)
            {
                gGameState.msgOpen = 0;
                gGameState.dialogStep = 4;
                gGameState.choiceMenu = 0;
                gGameState.choiceY = 0;
                gGameState.choiceCursor = 0xFF;
                gGameState.choiceIndex = 0;
                if (what == 0)
                    ShowMessage(0x44, 14);
                else if (what == 1)
                    ShowMessage(0x42, 14);
                else
                    ShowMessage(0x49, 14);
            }
            else
            {
                gGameState.msgOpen = 0;
                ShowMessage(0x46, 14);
                gGameState.choiceMenu = 0;
                gGameState.choiceY = 0;
                gGameState.choiceCursor = 0xFF;
                gGameState.choiceIndex = 0;
                gGameState.dialogStep = 4;
            }
        }
        else
        {
            gMain.newKeys &= 0xFF3E;
        }
    }
    else if (gGameState.dialogStep == 4)
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
        {
            gGameState.msgOpen = 0;
            ShowMessage(0x3D, 1);
            gGameState.dialogStep = 5;
        }
    }
    else
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
            EndItemEffect();
    }
}

// Items 8-10 (the Tings): the effect intro, then Effect8.
void StartTingEffect(void)
{
    StartEffectIntro();
    gGameState.itemsEnabled = 0;
    gGameState.effectActive = 1;
    gGameState.balloonTimer = gBalloonSprite[0].duration;
    gGameState.balloonFrame = 0;
    ShowMessage(0x27, 1);
}

// Draws the title sprite at (x, y) on BG3's map, bobbing; kind 1 adds gTingSprite with a
// palette per effect, kind 2 flips it and adds gBalloonSprite at (sprite2X, sprite2Y). One
// variable holds the flip flags and then the palette.
void DrawTitleSpriteOnMap(s16 x, s16 y, u8 kind)
{
    s16 bob = gEffectBobOffsets[(gMain.frameCount & 0x30) >> 4];
    u8 mosaic = gMain.mosaic != 0;
    u8 flags = 0;

    if (kind == 2)
        flags = 8;
    DrawSpriteAt(gTitleSprite, 0, x - gMain.bgOfs[3][0] + bob,
                 y - gMain.bgOfs[3][1], mosaic, flags, 1);
    if (kind == 1)
    {
        if (gGameState.effect[0] == 8)
            flags = 8;
        else if (gGameState.effect[0] == 9)
            flags = 15;
        else
            flags = 4;
        DrawSprite(gTingSprite, 0, x - gMain.bgOfs[3][0] + (s16)(bob + 11),
                   y - gMain.bgOfs[3][1] + 26, 0xC0, mosaic, 0, 0, 0, 0, flags);
    }
    else if (kind == 2)
    {
        flags = 16;
        DrawSpriteAt(gBalloonSprite, 0, gGameState.sprite2X - gMain.bgOfs[3][0] + bob,
                     gGameState.sprite2Y - gMain.bgOfs[3][1], mosaic, flags, 1);
    }
}

// Item effects 8-10: the title sprite drops in over the type 1 marker and sends the item use.
void Effect8(void)
{
    struct Main *main = &gMain;
    s32 m = FindMarker(1);
    u8 idx = gGameState.effect[0] - 8;
    u16 x, y;
    u8 arg;

    if (gGameState.effectStep <= 3)
        DrawEffectIntro();
    if (gGameState.effectStep == 2)
    {
        if (gGameState.inMessage == 0)
        {
            if (gGameState.effectDelay == 0)
            {
                // FAKEMATCH: without x (found with decomp-permuter), cse stores effectDelay's
                // register (known to be 0 here) instead of a fresh 0.
                x = 0;
                gGameState.viewLocked = 0;
                gGameState.effectDelay = 8;
                gGameState.effectStep = 3;
                gGameState.flags.isFree = 0;
                gGameState.savedEffect = gGameState.effect[0];
                SendItemUse(0x14, x);
            }
            else
            {
                gGameState.effectDelay--;
            }
        }
    }
    else if (gGameState.effectStep == 3)
    {
        if (gGameState.effectDelay == 0)
        {
            if (gGameState.scrolling == 0)
            {
                gGameState.spriteY = main->bgOfs[3][1] - 0x30;
                gGameState.effectStep = 4;
            }
        }
        else
        {
            gGameState.effectDelay--;
            gGameState.flags.isFree = 0;
        }
    }
    else if (gGameState.effectStep == 4)
    {
        if (main->frameCount & 1)
            gGameState.spriteY++;
        if (gGameState.markers[m].y - gGameState.spriteY <= 0x18)
        {
            gGameState.effectDelay = (idx + 2) * 60;
            gGameState.spriteY = gGameState.markers[m].y - 0x18;
            gGameState.effectStep = 6;
            ShowMessage(0x28, 1);
        }
        DrawTitleSpriteOnMap(gGameState.markers[m].x - 0x18, gGameState.spriteY, 1);
    }
    else if (gGameState.effectStep == 6)
    {
        x = gGameState.markers[m].x - 0x18;
        y = gGameState.markers[m].y - 0x18;
        DrawTitleSpriteOnMap(x, y, 1);
        if (gGameState.effectDelay == 0)
        {
            if (gGameState.selectedItem == 5)
                arg = gGameState.items[5].price;
            else
                arg = gGameState.items[idx + 8].price;
            SendItemUse(idx + 8, arg);
            gGameState.spriteY = y;
            gGameState.effectStep = 5;
        }
        else
        {
            SoundPlayLoop(1, 1, 0);
            if (--gGameState.effectDelay == 10)
                gGameState.msgWaitTimer = 300;
        }
    }
    else if (gGameState.effectStep == 5)
    {
        x = gGameState.markers[m].x - 0x18;
        y = gGameState.markers[m].y - 0x18;
        DrawTitleSpriteOnMap(x, y, 1);
    }
    else if (gGameState.effectStep == 7)
    {
        gGameState.spriteY--;
        DrawTitleSpriteOnMap(gGameState.markers[m].x - 0x18, gGameState.spriteY, 0);
        if (gGameState.spriteY < main->bgOfs[3][1] - 0x30)
        {
            gGameState.effectStep = 0;
            if (gGameState.selectedItem == 5)
                ShowMessage(0x30, 1);
            else
                ShowMessage(gGameState.effect[0] + 0x10, 1);
            EndItemEffect();
        }
    }
}

// Item 11 (the slot 6 item until the shop is unlocked): a choice dialogue, one for the sea
// and one for dungeons (Effect11).
void StartEffect11(void)
{
    if (!gGameState.onSea)
        ShowMessage(0xD2, 1);
    else
        ShowMessage(0xDB, 1);
    gGameState.effect[0] = 11;
    gGameState.effectActive = 1;
    gGameState.itemsEnabled = 0;
    gGameState.dialogStep = 0;
}

// Item effect 11.
void Effect11(void)
{
    if (gGameState.dialogStep == 0)
    {
        if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.choiceCursor == 0)
                gGameState.dialogStep = 2;
            else if (gGameState.choiceCursor == 1)
                gGameState.dialogStep = 1;
            else
                gGameState.dialogStep = 4;
        }
    }
    else if (gGameState.dialogStep == 1)
    {
        if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && (gMain.newKeys & 1))
            gGameState.dialogStep = 2;
    }
    else if (gGameState.dialogStep == 2)
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
        {
            gMain.newKeys &= ~(gMain.newKeys & 0x30F);
            ShowMessage(0xD8, 1);
            gGameState.dialogStep = 3;
        }
    }
    else if (gGameState.dialogStep == 3)
    {
        if ((gGameState.msgStatus == 1 || gGameState.msgStatus == 3) && (gMain.newKeys & 1))
        {
            if (gGameState.choiceCursor == 1)
            {
                gMain.newKeys ^= 1;
                if (!gGameState.onSea)
                    ShowMessage(0xD4, 1);
                else
                    ShowMessage(0xDD, 1);
                gGameState.dialogStep = 1;
            }
            else
            {
                gGameState.dialogStep = 4;
            }
        }
    }
    else
    {
        if ((gGameState.msgStatus == 3 && (gMain.newKeys & 0x30F)) || gGameState.msgWaitTimer > 0x12A)
        {
            gGameState.items[11].tile = (gGameState.items[11].tile & 0xF000) + 0xAE;
            SetSelectedItem(gGameState.selectedItem);
            EndItemEffect();
        }
    }
}

// Effect5 with a result for Effect12: go straight to its drop.
void StartEffectDrop(void)
{
    gGameState.effectDelay = 8;
    gGameState.viewLocked = 0;
    gGameState.effectStep = 4;
}

// Item effect 12.
void Effect12(void)
{
    struct Main *main = &gMain;
    s32 m = FindMarker(1);
    u16 x, y;

    if (gGameState.effectStep == 3)
    {
        if (gGameState.effectDelay == 0)
        {
            if (gGameState.scrolling == 0)
            {
                gGameState.sprite2Y = gGameState.spriteY = main->bgOfs[3][1] - 0x30;
                gGameState.effectStep = 4;
            }
        }
        else
        {
            gGameState.effectDelay--;
        }
    }
    else if (gGameState.effectStep == 4)
    {
        if (main->frameCount & 1)
        {
            gGameState.spriteY++;
            gGameState.sprite2Y++;
        }
        if (gGameState.markers[m].y - gGameState.spriteY <= 0x18)
        {
            gGameState.spriteY = gGameState.markers[m].y - 0x18;
            gGameState.sprite2Y = gGameState.markers[m].y - 0x18;
            SendItemUse(gGameState.effect[0], gGameState.items[5].price);
            gGameState.effectStep = 5;
        }
        x = gGameState.markers[m].x - 0x15;
        gGameState.sprite2X = gGameState.markers[m].x + 0x11;
        DrawTitleSpriteOnMap(x, gGameState.spriteY, 2);
    }
    else if (gGameState.effectStep == 5)
    {
        DrawBalloonShield(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        DrawBalloonShield(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (gGameState.flags.timer)
        {
            gGameState.effectTimer = 120;
            gGameState.effectStep = 6;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.effectStep == 6)
    {
        DrawBalloonShield(gGameState.markers[m].x, gGameState.markers[m].y, 1);
        DrawBalloonShield(gGameState.markers[m].x, gGameState.markers[m].y, 0);
        if (!gGameState.flags.timer)
        {
            gGameState.spriteX = gGameState.markers[m].x - 0x15;
            gGameState.sprite2X = gGameState.markers[m].x + 0x11;
            gGameState.spriteY = gGameState.markers[m].y - 0x18;
            gGameState.sprite2Y = gGameState.markers[m].y - 0x18;
            gGameState.effectStep = 7;
        }
        else
        {
            x = gGameState.markers[m].x - main->bgOfs[3][0] + 4;
            y = gGameState.markers[m].y - main->bgOfs[3][1] - 0x24;
            DrawTimerCount(x, y);
        }
    }
    else if (gGameState.effectStep == 7)
    {
        gGameState.spriteY--;
        gGameState.sprite2Y--;
        gGameState.spriteX = gGameState.markers[m].x - 0x15;
        gGameState.sprite2X = gGameState.markers[m].x + 0x11;
        DrawTitleSpriteOnMap(gGameState.spriteX, gGameState.spriteY, 2);
        if (gGameState.spriteY < main->bgOfs[3][1] - 0x30)
        {
            ShowMessage(0x30, 1);
            gGameState.effectStep = 0;
            EndItemEffect();
        }
        if (gGameState.sprite2Y < main->bgOfs[3][1] - 0x30)
            gGameState.sprite2Y = main->bgOfs[3][1] - 0x30;
    }
}

// GC answer 14 to item 2 (the cursor sat on Link: the GC frees it and drops the bomb
// later): a 300-frame countdown (Effect14).
void StartBombCountdown(void)
{
    ShowMessage(0x50, 1);
    gGameState.effectTimer = 300;
    gGameState.tingleFrame = 0;
    gGameState.tingleAnim = 5;
    gGameState.tingleAnimPart = 0;
}

// Item effect 14: a countdown (effectTimer) that ticks faster near the end, drawn at the type 3
// marker; at 0 it switches to effect 2.
void Effect14(void)
{
    u8 mosaic = gMain.mosaic != 0;
    s32 m;

    if (gGameState.flags.busy & 1)
        gGameState.effectTimer++;
    if (gGameState.selectWinStep == 0)
    {
        gGameState.msgWaitTimer = 0;
        if (gGameState.effectTimer > 179)
        {
            if ((u16)(gGameState.effectTimer % 30) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.effectTimer > 59)
        {
            if ((u16)(gGameState.effectTimer % 20) == 0)
                SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        else if (gGameState.effectTimer == 0)
        {
            gGameState.msgWaitTimer = 300;
            gGameState.effect[0] = 2;
            StartBombBlast();
        }
        else if ((u16)(gGameState.effectTimer % 10) == 0)
        {
            SoundPlay(0xC - SOUND_ID_SHIFT, 0);
        }
        m = FindMarker(3);
        DrawSpriteParts(gBombSprite, gGameState.markers[m].x - gMain.bgOfs[3][0],
                      gGameState.markers[m].y - gMain.bgOfs[3][1], mosaic, 0);
    }
}

// Run the current item effect (effect[0], from slot 7) while one is active.
void UpdateItemEffect(void)
{
    if (gGameState.effectActive)
    {
        if (!gGameState.itemsEnabled)
        {
            if (gGameState.effectTimer != 0)
                gGameState.effectTimer--;
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
    gGameState.textColor = 6;
    gGameState.msgStatus = 0;
    gGameState.glyphWidth = 6;
    gGameState.textBig = 0;
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
    gGameState.afterLinkLost = 1;
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
    obj->scale = 0;
    obj->ofsX = 0;
    obj->ofsY = 0;
    obj->piece = 0;
    obj->angle = 0;
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
    s16 ofsX = gObjBurstOfsX[obj->piece];
    s16 ofsY = gObjBurstOfsY[obj->piece];
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
    obj->ofsY = obj->ofsX = 0;
    obj->vx = gObjBurstVelX[obj->piece];
    obj->vy = gObjBurstVelY[obj->piece];
    obj->vz = -0x100;
    obj->angle = 0;
    obj->unk12 = 2;
    obj->scale = 0x180;
    obj->active++;

    for (j = 1; j <= 4; j++)
    {
        idx = ObjFindFree();
        if (idx != -1)
        {
            // BUG: indexes from obj, not gObjs (the same when obj is gObjs[0])
            obj[idx].piece = j;
            ofsX = gObjBurstOfsX[obj[idx].piece];
            ofsY = gObjBurstOfsY[obj[idx].piece];
            obj[idx].type = 0;
            obj[idx].active = 1;
            obj[idx].x = (gGameState.markers[i].x << 8) + ofsX;
            obj[idx].y = (gGameState.markers[i].y << 8) + ofsY;
            obj[idx].ofsY = obj[idx].ofsX = 0;
            obj[idx].vx = gObjBurstVelX[obj[idx].piece];
            obj[idx].vy = gObjBurstVelY[obj[idx].piece];
            obj[idx].vz = -0x100;
            obj[idx].angle = 0;
            obj[idx].unk12 = 2;
            obj[idx].scale = 0x180;
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
    obj->angle -= 16;
    if (obj->vz < 0)
        obj->scale = 0x80 - obj->vz;
    else
        obj->scale = obj->vz + 0x80;
}

// Object type 0: pieces flying out of the type 3 marker (spawned by StartBombBlast).
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
        DrawSprite(gBurstPieceSprite, 0, x, y, obj->angle, mosaic, obj->scale, obj->ofsX,
                   obj->ofsY, 0, 0);
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
    gGameState.selectWinX = gGameState.selectWinX + step;
    if (gGameState.selectWinX >= 0xE8 && gGameState.selectWinX <= 0xF8)
    {
        gMain.win0h = 0xEF;
    }
    else if (gGameState.selectWinX + 8 == 0)
    {
        gMain.win0h = 0;
        gMain.winin = 0;
    }
    else
    {
        gMain.win0h = gGameState.selectWinX + 8;
    }
    gMain.win0v = 0x9F;
    gMain.winout = 0;
    gMain.dispcnt |= 0x2000;
}

void SelectWindowDrawEdge(void)
{
    DrawSprite(gSelectWindowEdgeSprite, 0, gGameState.selectWinX, gGameState.selectWinY, 0, 0, 0, 0, 0, 0, 0);
}

// Select (in mode 1 state 2, when nothing else is up) wipes a window across the screen
// (SelectWindowWipe), flips BG0 to its other half behind it and wipes it back; Select again (or
// leaving state 2) reverses it. gGameState.selectWinStep is the step.
void UpdateSelectWindow(void)
{
    switch (gGameState.selectWinStep)
    {
    case 0:
        if (gMain.modeState == 2 && gGameState.inMessage == 0 && gGameState.menuOpen == 0
         && !gGameState.menuSliding && (gMain.newKeys & 4))
        {
            gGameState.selectWinOpen = 1;
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.selectWinStep++;
        }
        break;
    case 1:
        SelectWindowWipe(0);
        if (gGameState.selectWinX == 0xFFF8)
        {
            gGameState.selectWinStep++;
            gMain.bgOfs[0][0] = 0x100;
        }
        SelectWindowDrawEdge();
        break;
    case 2:
        SelectWindowWipe(1);
        if (gGameState.selectWinX == 0xF8)
        {
            gGameState.selectWinStep++;
            gMain.dispcnt ^= 0x2000;
        }
        SelectWindowDrawEdge();
        break;
    case 3:
        // Two branches with the same body (cross-jumped), not one ||.
        if (gMain.modeState != 2)
        {
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.selectWinStep++;
        }
        else if (gMain.newKeys & 4)
        {
            SoundPlay(0x27 - SOUND_ID_SHIFT, 0);
            gGameState.selectWinStep++;
        }
        break;
    case 4:
        SelectWindowWipe(0);
        if (gGameState.selectWinX == 0xFFF8)
        {
            gGameState.selectWinStep++;
            gMain.bgOfs[0][0] = 0;
        }
        SelectWindowDrawEdge();
        break;
    case 5:
        SelectWindowWipe(1);
        if (gGameState.selectWinX == 0xF8)
        {
            gGameState.selectWinStep = 0;
            gGameState.selectWinOpen = 0;
            gGameState.active = 1;
            gMain.dispcnt &= 0xDFFF;
        }
        SelectWindowDrawEdge();
        break;
    }
}

void ResetSelectWindow(void)
{
    gGameState.selectWinX = 0xF8;
    gGameState.selectWinY = 0x50;
    gGameState.selectWinStep = 0;
}

// The status icon at (232, 128) and the blinking frame around it; gGameState.statusIcon picks the
// icon.
void DrawStatusIcon(void)
{
    u8 blink = (gMain.frameCount >> 3) % 2;
    s32 m = FindMarker(1);
    u8 type;

    if (gGameState.selectWinOpen | gGameState.inMessage | gGameState.chartOpen | gGameState.menuOpen)
        return;
    if (gGameState.holdState != 0 || gGameState.markers[m].param == 4)
    {
        gGameState.statusIcon = 0;
    }
    else if (gGameState.flags.miniboss)
    {
        if (gGameState.flags.isFree)
            gGameState.statusIcon = 1;
        else
            gGameState.statusIcon = 0;
    }
    else if (gGameState.flags.isFree)
    {
        if (gGameState.enemyHighlighted)
        {
            SoundPlayLoop(3, 3, 0);
            gGameState.statusIcon = 2;
        }
        else
        {
            gGameState.statusIcon = 1;
        }
    }
    else if (gGameState.flags.followTarget)
    {
        SoundPlayLoop(4, 4, 0);
        gGameState.statusIcon = 1;
    }
    else if (gGameState.enemyHighlighted)
    {
        SoundPlayLoop(3, 3, 0);
        gGameState.statusIcon = 2;
    }
    else
    {
        gGameState.statusIcon = 0;
    }

    if (gGameState.statusIcon == 1)
    {
        type = gGameState.markers[FindMarker(1)].param;
        if (type)
            gGameState.statusIcon = type + 2;
    }

    if (gGameState.statusIcon == 0)
        return;
    if (gGameState.statusIcon == 1)
        DrawSprite(gCharIcon0, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.statusIcon == 2)
        DrawSprite(gStatusHighlightSprite, 0, 0xE8, 0x80, 0, 0, 0, 0, 0, 0, 0);
    else if (gGameState.statusIcon == 3)
        DrawSpriteAt(gCharIcon1, 0, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.statusIcon == 4)
        DrawSpriteAt(gCharIcon2, gGameState.char2Frame, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.statusIcon == 5)
        DrawSpriteAt(gCharIcon3, gGameState.char3Frame, 0xE8, 0x80, 0, 0, 0);
    else if (gGameState.statusIcon == 6)
        DrawSpriteAt(gCharIcon4, gGameState.char4Frame, 0xE8, 0x80, 0, 0, 0);
    DrawSprite(gStatusFrameSprite, 0, 0xE0, blink + 0x80, 0, 0, 0, 0, 0, 0, 0);
}
