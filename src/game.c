#include "global.h"
#include "gba/syscall.h"
#include "main.h"
#include "game.h"
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

void Func_0201A540(void)
{
    if (gMain.unk457 == 0)
    {
        Func_02019F5C();
    }
    else
    {
        Func_0201A0A8();
        Func_02019350(&gGameState);
    }
}

// Tile of the floor number box's edge.
#ifdef VERSION_GZLJ01
#define FLOOR_EDGE_TILE 0x6D
#else
#define FLOOR_EDGE_TILE 0x01
#endif

// Draw the dungeon floor number next to the map (blank outside dungeons or with no floor).
void Func_0201A570(void)
{
    u16 *map = (u16 *)0x06006074;
    u8 floor;

    if (gGameState.mapImage->floor == 0 || gGameState.flags.dungeonNo == 0)
    {
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
        map += 0x20;
        CpuFill16(0xE05B, map, 6);
    }
    else
    {
        map[0] = 0xE458;
        map[1] = 0xE800 + FLOOR_EDGE_TILE;
        map[2] = 0xE800 + FLOOR_EDGE_TILE;
        map[0x20] = 0xE458;
        map[0x40] = 0xE458;
        map[0x60] = 0xE458;
        map[0x61] = 0xE000 + FLOOR_EDGE_TILE;
        map[0x62] = 0xE000 + FLOOR_EDGE_TILE;
        floor = gGameState.mapImage->floor;
        if ((s8)floor < 0)
        {
            u8 n = ~floor * 2 + 2;  // -floor * 2

            map[0x21] = 0xE05C;
            map[0x41] = 0xE05D;
            map[0x22] = n + 0xE030;
            map[0x42] = n + 0xE031;
        }
        else
        {
            map[0x21] = gGameState.mapImage->floor * 2 + (s16)0xE030;
            map[0x41] = gGameState.mapImage->floor * 2 + (s16)0xE031;
            map[0x22] = 0xE05E;
            map[0x42] = 0xE05F;
        }
    }
}

// Point gGameState.mapImage at the received map image and relocate its data offsets (unless
// they are already addresses).
bool32 Func_0201A69C(void)
{
    gGameState.mapImage = (struct MapImage *)GBACOM_BUF_MESSAGE;
    if (*(u32 *)(GBACOM_BUF_MESSAGE + 8) <= 0x01FFFFFF)
    {
        gGameState.mapImage->tiles = (u8 *)((u32)gGameState.mapImage->tiles + (u32)gGameState.mapImage);
        gGameState.mapImage->map = (u16 *)((u32)gGameState.mapImage->map + (u32)gGameState.mapImage);
    }
    GbaComDataStatusReset(2);
}

// Set up the map view on the player's marker: in map mode, load the map image's tiles and
// draw the visible part of its map; otherwise just centre the view.
bool32 Func_0201A6D4(void)
{
    u8 i = FindMarker(3);
    u16 maxX, maxY;
    s16 x, y;
    s16 yEnd;
    u16 *map;

    if (i == 0xFF)
        i = FindMarker(1);
    if (gMain.unk457 != 0)
    {
        if (gGameState.unk10E == 0)
        {
            CpuFill16(0, GBACOM_BUF_MESSAGE, 0xA000);
            gGameState.mapX = gMain.bgOfs[3][0];
            gGameState.mapY = gMain.bgOfs[3][1];
        }
        else
        {
            CpuCopy16(gGameState.mapImage->tiles, (void *)0x06000000, gGameState.mapImage->tilesSize);
            maxX = gGameState.mapImage->width - 120;
            maxY = gGameState.mapImage->height - 120;
            gGameState.mapX = gGameState.markers[i].x - 60;
            gGameState.mapY = gGameState.markers[i].y - 60;
            if (gGameState.mapX < 0)
                gGameState.mapX = 0;
            if (gGameState.mapX > (s16)maxX)
                gGameState.mapX = maxX;
            if (gGameState.mapY < 0)
                gGameState.mapY = 0;
            if (gGameState.mapY > (s16)maxY)
                gGameState.mapY = maxY;
            gMain.bgOfs[3][0] = gGameState.mapX - 8;
            gMain.bgOfs[3][1] = gGameState.mapY - 8;
            Func_0201A570();
            x = gGameState.mapX;
            y = gGameState.mapY;
            yEnd = y + 120;
            map = gGameState.mapImage->map + ((y >> 3) * gGameState.mapImage->mapWidth + (x >> 3));
            CpuFill16(0x2000, (void *)0x06005800, 0x800);
            do
            {
                Func_020192D8(y, x, map);
                map += gGameState.mapImage->mapWidth;
                y += 8;
            } while (y <= yEnd);
        }
    }
    else
    {
        gGameState.mapX = gGameState.markers[i].x - 68;
        gGameState.mapY = gGameState.markers[i].y - 68;
    }
    return TRUE;
}

u8 FindMarker(u8 type)
{
    s32 i;

    for (i = 0; i < 16; i++)
    {
        if (type == gGameState.markers[i].type)
            return i;
    }
    return 0xFF;
}

// Distance from the type 1 marker to icons[2] (arg 0) or to the type 3 marker.
u32 Func_0201A8AC(u8 arg)
{
    u8 i = FindMarker(1);
    s32 dx, dy;

    if (arg == 0)
    {
        dx = gGameState.icons[2].x - gGameState.markers[i].x;
        dy = gGameState.icons[2].y - gGameState.markers[i].y;
    }
    else
    {
        u8 j = FindMarker(3);

        dx = gGameState.markers[j].x - gGameState.markers[i].x;
        dy = gGameState.markers[j].y - gGameState.markers[i].y;
    }
    dx *= dx;
    dy *= dy;
    dx += dy;
    return Sqrt(dx);
}

// The target compass: while a target (icons[2], type 3) is placed, point an arrow at it
// from the player, pulse it faster the closer the player gets, play the near and arrival
// sounds, and (once Link has the Wind Waker, flags.hasTact) announce its direction when
// Link is not facing it (flags.unk2 looks like his heading).
// Nearly matches: the logic and instruction selection are right, but registers differ from
// the start (the ArcTan2 operands land in r4 instead of r2, so the address temporaries shift
// and the targetNear stores of levels 2 and 4 are not cross-jumped):
// void Func_0201A944(void)
// {
//     u8 angle;
//     s32 x, y;
//     s32 level;
//
//     if (gGameState.unk1FA != 0)
//         return;
//     if (gGameState.unk224 != 0)
//         return;
//     if (gGameState.icons[2].type != 3)
//         return;
//     {
//         u8 i = FindMarker(1);
//
//         x = gGameState.icons[2].x - gGameState.markers[i].x;
//         y = gGameState.icons[2].y - gGameState.markers[i].y;
//         angle = ((u16)~ArcTan2(x >> 4, y >> 4) + 0x4001) >> 8;
//     }
//     x = gSinTable[angle];
//     x >>= 3;
//     y = (gSinTable + 64)[angle];
//     y >>= 3;
//     DrawSprite(gMapIconSprites[gGameState.icons[2].type], 0, (u16)(x + 0xB8), (u16)(y + 0x58), 0, 0, 0, 0, 0, 0, 0);
//     level = Func_0201A8AC(0);
//     if ((u32)level <= 0x18)
//     {
//         if (gGameState.unk224 == 0)
//         {
//             GamePlaySound(0x1A5, 1);
//             gGameState.icons[2].type = 0;
//             gGameState.icons[1].type = 0;
//             gGameState.unk105 = 0;
//         }
//         level = 0;
//         gGameState.targetNear = 0;
//     }
//     else if ((u32)level <= 0x7F)
//     {
//         if (gGameState.unk224 == 0 && gGameState.targetNear == 0)
//         {
//             GamePlaySound(0x1A4, 1);
//             gGameState.targetNear = 1;
//         }
//         level = 1;
//     }
//     else if ((u32)level <= 0x1FF)
//     {
//         level = 2;
//         gGameState.targetNear = 0;
//     }
//     else if ((u32)level <= 0x3FF)
//     {
//         level = 3;
//         gGameState.targetNear = 0;
//     }
//     else
//     {
//         level = 4;
//         gGameState.targetNear = 0;
//     }
//     if (gGameState.targetBlinkOn != 0)
//     {
//         if (--gGameState.targetBlinkTimer == 0)
//             gGameState.targetBlinkOn = 0;
//     }
//     else
//     {
//         if (++gGameState.targetBlinkTimer >= gTargetBlinkPeriods[level])
//         {
//             gGameState.targetBlinkTimer = gTargetBlinkPeriods[level];
//             gGameState.targetBlinkOn = 1;
//         }
//     }
//     gGameState.targetScale = (gGameState.targetBlinkTimer << 7) / gTargetBlinkPeriods[level] + 0x100;
//     DrawSprite(gSprite_0202C3F8, 0, 200, 72, angle + 0x80, 0, gGameState.targetScale, 0, 16, 0, 0);
//     if (gGameState.unk224 == 0 && gGameState.flags.hasTact && level > 1
//      && (u8)(gGameState.flags.unk2 - angle + 9) > 18)
//     {
//         angle = ~angle + 1;
//         if (((angle + 8) & 0x1F) <= 0x10)
//         {
//             u8 dir = (u8)(angle + 0x88) >> 5;
//
//             if (gGameState.targetDir != dir && gGameState.unk1FA == 0)
//             {
//                 GamePlaySound(0x19C + dir, 1);
//                 gGameState.targetDir = dir;
//             }
//         }
//         else
//         {
//             gGameState.targetDir = 0xFF;
//         }
//     }
// }
INCLUDE_ASM(Func_0201A944);

INCLUDE_ASM(Mode1VBlank);

// Icon type 1: follow the player's marker (type 3, else type 1).
void Func_0201AC94(struct MapIcon *icon)
{
    u8 i = FindMarker(3);

    if (i == 0xFF)
        i = FindMarker(1);
    icon->x = gGameState.markers[i].x;
    icon->y = gGameState.markers[i].y;
}

void Func_0201ACD0(void)
{
    gGameState.icons[0].type = 1;
    gGameState.icons[0].frame = 0;
    gGameState.icons[0].timer = 0;
    Func_0201AC94(&gGameState.icons[0]);
}

// Icon type 3: blink between frames 0 and 1.
void Func_0201ACF8(struct MapIcon *icon)
{
    if (icon->timer != 0)
    {
        icon->timer--;
    }
    else
    {
        const u8 *const *sprites;
        s32 offset;

        icon->frame ^= 1;
        sprites = gMapIconSprites;
        offset = icon->frame * 8;  // 8 bytes per frame; byte 4 is its duration
        icon->timer = (sprites[3] + offset)[4];
    }
}

void Func_0201AD24(void)
{
    gGameState.icons[2].type = 3;
    gGameState.icons[2].frame = 1;
    gGameState.icons[2].timer = 0;
    Func_0201ACF8(&gGameState.icons[2]);
}

void Func_0201AD4C(void)
{
    gGameState.icons[1].type = 2;
    if (gGameState.icons[2].type != 0)
    {
        gGameState.icons[1].x = gGameState.icons[2].x;
        gGameState.icons[1].y = gGameState.icons[2].y;
    }
    else
    {
        gGameState.icons[1].x = gGameState.icons[0].x;
        gGameState.icons[1].y = gGameState.icons[0].y;
    }
}

// Icon type 2: the cursor. While placing a target (unk105 == 1), move it a sea chart square
// (16 pixels) at a time with the D-pad and place the target (icons[2]) with A. Then name the
// square under the cursor if its unk0F4 bit is set.
void Func_0201AD90(struct MapIcon *icon)
{
    u16 keys;
    u16 pos;
    u8 square;
    s32 i;

    if (gGameState.unk105 == 1)
    {
        if ((gMain.newKeys & 0xF0) == 0)
        {
            keys = gMain.heldKeys;
            if (keys & 0xF0)
            {
                if (++icon->timer == 2)
                    icon->timer = 0;
            }
            else
            {
                icon->timer = 0;
            }
        }
        else
        {
            keys = gMain.newKeys;
            icon->timer = 0;
        }
        if (icon->timer == 0 && (keys & 0xF0))
        {
            pos = icon->y >> 4;
            if (keys & 0x40)
            {
                if (pos != 0)
                    icon->y -= 0x10;
            }
            else if (keys & 0x80)
            {
                if (pos != 0x6F)
                    icon->y += 0x10;
            }
            pos = icon->x >> 4;
            if (keys & 0x20)
            {
                if (pos != 0)
                    icon->x -= 0x10;
            }
            else if (keys & 0x10)
            {
                if (pos != 0x6F)
                    icon->x += 0x10;
            }
        }
        if (gMain.newKeys & 1)
        {
            gGameState.icons[2].type = 3;
            gGameState.icons[2].x = icon->x;
            gGameState.icons[2].y = icon->y;
            Func_0201ACD0();
            Func_0201AD24();
            gGameState.unk022 = 240;
            SoundPlay(10 - SOUND_ID_SHIFT, 0);
        }
    }
    square = (icon->x >> 8) + (icon->y >> 8) * 7;
    if ((gGameState.unk0F4[square >> 3] >> (square & 7)) & 1)
    {
        for (i = 0; i < 49; i++)
        {
            if (gSlot14Data[i].square == square)
            {
                if (square == 25 && !gGameState.flags.unkA_4)
                {
                    gGameState.unk13C = 0;
                }
                else
                {
                    if (square == 32 && gGameState.flags.unk5_6)
                        GamePlaySound(0x13E, 1);
                    else
                        GamePlaySound(0x14E + gSlot14Data[i].square, 1);
#ifndef VERSION_GZLJ01
                    gGameState.unk13E = 0;
#endif
                }
            }
        }
    }
    else
    {
        gGameState.unk13C = 0;
    }
}

// Update and draw the map icons.
void Func_0201AF48(void)
{
    u8 mosaic = gMain.mosaic != 0;
    s32 i;

    for (i = 0; i < 4; i++)
    {
        if (gGameState.icons[i].type != 0)
        {
            gMapIconFuncs[gGameState.icons[i].type](&gGameState.icons[i]);
            DrawSprite(gMapIconSprites[gGameState.icons[i].type], gGameState.icons[i].frame,
                       (gGameState.icons[i].x >> 4) + 12, (gGameState.icons[i].y >> 4) + 12, 0,
                       mosaic, 0, 0, 0, 0, 0);
        }
    }
}

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
