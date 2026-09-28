#ifndef GUARD_GAME_H
#define GUARD_GAME_H

#include "global.h"
#include "gba_com.h"

// One map marker, as slot 3 sends them (8 bytes each after a count byte and the GC's
// agbIsActive() byte). The GC fills them in dMap_c::setGbaPoint_ocean/_dungeon (still asm in
// the TWW decomp): marker 0 is the player (type 1), marker 1 the Tingle Tuner cursor (type 3;
// a copy of the player's position when there is no cursor point), then up to 14 more. Types
// (the GBA's numbers; dMap_c::getTypeAgbGcFromTypeAcs): 1 the player, 2 an enemy, 3 the
// cursor, 4 the ship, 17 a door, 19 a friend (companion); 5, 12, 13 and 16 are hidden
// (gSpriteTable[7]) until examined, 7 sends a switch record.
struct MapMarker
{
    /*0x0*/ s16 x;       // map pixels; on the sea 256 per chart square (the high byte is its column)
    /*0x2*/ s16 y;
    /*0x4*/ u8 angle;    // the point's angle >> 8 (DrawSprite rotation; >> 5 is a direction)
    /*0x5*/ u8 type:6;
            u8 highlight:2;  // enemies (type 2) only: draw the ring (gMarkerHighlightSprite), 1 or 2
    /*0x6*/ u8 param;    // per type: player/friend: the character (0: Link; icon under the
                         //   marker, the status icon); types 5/16: its bit in revealedType5/16
    /*0x7*/ u8 gbaName;  // the GC's gbaName: which message ShowSelectionMsg shows (0: none)
};

// A sprite drawn over the map image (GameState.icons); type picks its update function and
// sprite (1: follows the player marker, 2: cursor, 3: blinking).
struct MapIcon
{
    /*0x0*/ s16 x;      // 12.4 map pixels
    /*0x2*/ s16 y;
    /*0x4*/ u8 type;    // 0: none
    /*0x5*/ u8 frame;
    /*0x6*/ u8 timer;
    /*0x7*/ u8 unk7;
};

// One item (gGameState.items, indexed by item number; 0 is none). ItemMenuInit copies all
// twelve from gShopItemsInit.
struct ItemSlot
{
    /*0x0*/ u16 tile;        // BG map entry of the icon's top-left tile (2x2 tiles, palette on top)
    /*0x2*/ u16 objTile;     // OBJ tile of the icon as a sprite (DrawItemSprite)
    /*0x4*/ s16 descMsg;     // message shown while the menu cursor is on it (MsgStartSilent)
    /*0x6*/ u8 price;        // rupees (0, 10-80); DrawItemCount shows it
    /*0x7*/ u8 objPalette;
};

// Game state in IWRAM (0x030050E0 in every version), 0x240 bytes up to gGbaComDataStatus.
// The struct body is generated from this list by tools/genstruct.py.
// genstruct GameState 0x240
// 0x000 struct GbaComFlags flags size=12  // last slot 9 packet from the GC
// 0x00C struct MapImage* mapImage  // the slot 2 map image (LoadMapImage)
// 0x010 u32 revealedType5  // marker type 5 examined, bit per MapMarker.param (else drawn hidden)
// 0x014 s16 mapX  // top left of the map view in the map image (pixels); BG3 view + 8 when streamed (ScrollMap)
// 0x016 s16 mapY
// 0x018 u8 followStill  // cleared by UpdateMap; set when the followed marker stood still for 2 frames (unread)
// 0x01E s16 followLastX  // followed marker's last x/y (FollowMarkerStreamedMap)
// 0x020 s16 followLastY
// 0x022 u16 unk022  // set to 240 when a target is placed (MapIconCursor)
// 0x024 u16 targetScale  // pulse of the target compass sprite (UpdateTargetCompass)
// 0x019 u8 followStillCount  // frames to followStill; cleared when rToggle turns on (R)
// 0x026 u16 revealedType16  // marker type 16 examined, bit per MapMarker.param
// 0x028 u16 seaAnimTimer[4] size=8  // per quadrant of the view (UpdateMap)
// 0x030 u16 squareMsgTimer  // frames until squareMsgPending names the player's sea chart square
// 0x032 s16 spriteX  // title sprite x (mode 0); the item effects' sprite position
// 0x034 s16 spriteY  // title sprite y
// 0x036 s16 sprite2X  // Effect12's second sprite
// 0x038 s16 sprite2Y
// 0x03A u16 switchMsg  // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
// 0x03C u8 switchNo  // 0xFF: none
// 0x03D u8 switchStageNo
// 0x03E u8 switchRoomNo
// 0x03F u8 switchType  // the GC switches on type - 1
// 0x040 u8 switchNo2
// 0x042 u8 unk042
// 0x044 struct MapIcon icons[4] size=0x20  // drawn over the map by UpdateMapIcons
// 0x064 struct MapMarker markers[16] size=0x80  // from slot 3 (Mode1RecvMarkers)
// 0x0E4 u8 markerCount
// 0x0E5 u8 unk0E5
// 0x0E8 u8 seaTileSet[4]  // gSeaSquareTileSets of each quadrant's square
// 0x0EC u8 seaAnimFrame[4]  // index into gSeaAnimFrames
// 0x0F0 u8 seaSquare[4]  // sea chart square of each quadrant; in a dungeon, the room in each BG map block at 0x0600C000
// 0x0F4 u8 visitedSquares[8] size=8  // slot 11: one bit per sea chart square (7x7) Link has arrived at (GC dComIfGs_isSaveArriveGridForAgb)
// 0x0FC u8 aButtonAction  // what A does, and the left HUD panel label: 0 examine the selection, 1 item 16 (look at the cursor), 2 sea chart open
// 0x0FD u8 hudChartLabel  // the right HUD panel shows the chart label instead of the item (chart open)
// 0x0FF u8 windArrowTimer  // frames left in the frame of the wind arrow (gWindArrowSprite, while flags.showWind)
// 0x100 u8 windArrowFrame  // its frame (0-3)
// 0x101 u8 targetBlinkOn
// 0x102 u8 targetDir  // direction (0-7) last announced by UpdateTargetCompass; 0xFF: none
// 0x103 u8 targetBlinkTimer
// 0x104 u8 active  // the Tuner takes input: the GC's agbIsActive() (gcActive), or 1 when nothing is up; cleared while the GC is busy
// 0x105 u8 placingTarget  // the sea chart cursor (icons[1]) is placing a target (A in the chart)
// 0x106 u8 squareMsgPending  // Link entered another sea chart square: name it after squareMsgTimer
// 0x107 u8 targetNear  // the near sound has played
// 0x108 u8 playerBlinkTimer  // DrawMapMarkers's timers: count up, flip the frame byte after them (player 20, enemy 10, cursor 30 frames)
// 0x109 u8 playerFrame
// 0x10A u8 enemyBlinkTimer
// 0x10B u8 enemyFrame
// 0x10C u8 cursorBlinkTimer
// 0x10D u8 cursorFrame
// 0x10E u8 mapLoaded  // a map image is loaded (0: none yet, e.g. after the GC relinks in a room with no map)
// 0x10F u8 type9Timer  // DrawMapMarkers: frame timers and frames of the gSpriteTable[9]/[13]/[16]/[19] animations
// 0x110 u8 type9Frame
// 0x113 u8 type13Timer
// 0x114 u8 type13Frame
// 0x115 u8 type16Timer
// 0x116 u8 type16Frame
// 0x117 u8 friendTimer
// 0x118 u8 friendFrame
// 0x119 u8 selectedMarker  // index into markers of the selection (UpdateSelection); 0: none
// 0x11A u8 selection  // 0 nothing under the cursor, 1 something selectable, 2 examined (its message is up)
// 0x11B u8 selectedType  // the selected marker's type
// 0x11C u8 revealedType12  // marker type 12 examined
// 0x11D u8 revealedType13  // marker type 13 examined
// 0x11E u8 placeTargetAtCursor  // choice message 0x1A7 picked: Mode1State2 puts the target on the cursor
// 0x11F u8 mapScrolling  // L pressed to scroll the map (UpdateMap)
// 0x120 u8 viewLocked  // the streamed map view stops following the marker (item effects)
// 0x121 u8 holdState  // the cursor held by the GC (mail type 12): 1 gCursorHeldSprite loops, 2 gCursorReleaseSprite plays out, then a switch record
// 0x122 u8 holdMsgIndex  // item effect 16 while holding: message 0x60 + this (up to 3)
// 0x123 u8 holdTimer
// 0x124 u8 holdFrame
// 0x125 u8 char2Timer  // frame timers and frames of the character icons 2-4 (MapMarker.param of players and friends)
// 0x126 u8 char2Frame
// 0x127 u8 char4Timer
// 0x128 u8 char4Frame
// 0x129 u8 char3Timer
// 0x12A u8 char3Frame
// 0x12B u8 effectStep  // item effect step (the balloon/shield, Effect5, Effect8, Effect12)
// 0x12C u8 introTimer  // StartEffectIntro animation (gEffectIntroSprite): frame timer and frame
// 0x12D u8 introFrame
// 0x12E u8 balloonTimer  // DrawBalloonShield: balloon and shield frame timers and frames
// 0x12F u8 balloonFrame
// 0x130 u8 shieldTimer
// 0x131 u8 shieldFrame
// 0x132 u8 effectDelay  // frames before the next effect step
// 0x133 u8 gcActive  // slot 3 byte 1: the GC's dMap_c::agbIsActive()
// 0x134 u8 scrolling  // the view is still catching up with the followed marker
// 0x135 u8 selectedSquare  // sea chart square under the cursor + 1 (UpdateSelection), 0: none
// 0x136 u8 awayFromType20  // the cursor has left the type 20 marker (it is only selectable after that)
// #ifdef VERSION_PAL size=8
// 0x138 s16 pendingMsg  // PAL (fields from here on are 8 bytes higher): ExamineSelection saves the selection's message, type and param for after reactionTimer, so moving the cursor meanwhile doesn't change what is shown and revealed
// 0x13A u8 pendingType
// 0x13B u8 pendingParam
// 0x13C u8 pendingReveal  // set with them; Mode1State2 reveals pendingType/Param when the message closes
// #endif
// 0x138 bits u8 unk138_0:2 appeared:1 shopUnlocked:1 balloonDiscount:1 tingDiscount:1 unk138_6:2  // sent to the GC as GbaFlg.bits: appeared shows "Tingle appeared" (daAgb field_0x67a); the others set the GC's event flags (flags.shopUnlocked etc.)
// 0x13C u8 msgOpen  // a message is up or opening (the box slides in); 0 closes it
// 0x13D s8 msgBoxPos  // message box slide, steps toward 10 while msgOpen is set, else 0
// 0x13E u16 msgWaitTimer  // frames at a full page; past 0x12B the page turns by itself (counts only while flags.gcInput)
// 0x140 u16 msgLoopSound  // loop sound of the message (message code F6 20-22), 0xFFFF: none
// 0x142 s16 msgId  // the message shown (MsgStart), 0xFFFF: none
// 0x144 const u8* msgPtr  // message text (in the EWRAM message block at 0x02000000); US Mode2State0 sets it too
// 0x148 u8 textX  // text cursor, pixels
// 0x149 u8 textY
// 0x14A u8 textColor  // gTextColorMasks index (message code FC)
// 0x14B u8 msgStatus  // last MsgStep result (0 go on, 1 after F6 or while opening, 2 wait, 3 end)
// 0x14C u8 textDelay  // frames per character (textWait reload)
// 0x14D u8 textWait  // frames before the next character (message code F9)
// 0x14E u8 choiceIndex  // choice options seen so far (message codes F7/F8)
// 0x14F u8 choiceCursor  // choice menu cursor, 0xFF: none
// 0x150 u8 choiceY  // text y of the first option
// 0x151 u8 choiceMenu  // 1: two choices, 2: three
// 0x152 u8 choiceCursorTimer  // choice cursor sprite (gChoiceCursorSprite) frame timer and frame
// 0x153 u8 choiceCursorFrame
// 0x156 u8 reactionTimer  // ExamineSelection's thinking pause before the selection's message (loop sound 6)
// 0x157 u8 glyphWidth
// 0x158 u8 textFast  // message codes F3 0D/0E: draw the text without waiting
// 0x159 u8 charLoopSound  // the last character was JP 0xBC / US 0x3F: MsgStep keeps loop sound 0 playing
// 0x15A u8 textBig  // text drawn 2x (message code F3 0F)
// 0x15C u8 effect[4] size=4  // slot 7: item use result
// 0x160 struct ItemSlot items[12] size=0x60
// 0x1C0 u16 menuCursorX  // item menu cursor position (8.8), moving toward the slot
// 0x1C2 u16 menuCursorY
// 0x1C4 u16 menuCursorDX
// 0x1C6 u16 menuCursorDY
// 0x1C8 u16 effectTimer  // item effect timer, counts down in UpdateItemEffect (held while the GC menu is open)
// 0x1CA u16 chestTimer  // chest counter timer (ChestCounterMain)
// 0x1CC u8 menuOpen  // the item menu is open (START)
// 0x1CD u8 menuCursor  // item menu slot (5 x 3; row 2 is the dungeon chest row)
// 0x1CE u8 menuKeyRepeat  // frames to the next d-pad repeat
// 0x1CF u8 slotItems[13] size=13  // item shown in each menu slot (SetSlotItem); rows 0-1 index items
// 0x1E0 u8 chestState  // dungeon chest counter state (0 off, 1 start, 2 blink, 3 wait)
// 0x1DE u8 selectedItem  // item on B (the right HUD panel)
// 0x1DF u8 pickedEmptySlot  // B on an empty menu slot (a d-pad press clears it); never read
// 0x1E1 u8 newChests  // chest bits that changed
// 0x1E2 u8 dialogStep  // item effect step (Effect7, Effect11)
// 0x1E3 u8 chestCount  // chests opened
// 0x1E4 u8 savedEffect  // Effect8's effect id while the GC answers request 0x14 (answer 20 restores it)
// 0x1E5 bits u8 itemFlying:1 itemsEnabled:1 effectActive:1 onSea:1 menuSliding:1 msgTest:1 unk1E5_6:2  // itemFlying: the picked icon flies to the B panel; itemsEnabled: no item effect runs (B/A/R work); effectActive: UpdateItemEffect runs effect[0]; onSea: item 11's sea look (ItemMenuInit); menuSliding: BG1 (the menu) still sliding; msgTest: GZLE01_ud message test
// 0x1E8 u16 selectWinX  // select window edge x (0xF8 closed, 0xFFF8 fully across)
// 0x1EA s16 selectWinY  // select window edge y
// 0x1EE u8 selectWinStep  // UpdateSelectWindow step (0: closed)
// 0x1F4 u8 statusIcon  // DrawStatusIcon: which icon
// 0x1F5 u8 enemyHighlighted  // an enemy marker (type 2) has its highlight set
// 0x1F8 s16 nextMsg  // message shown after the current one (switch type 15), 0xFFFF: none
// 0x1FB u8 selectWinOpen  // the Select window is up
// 0x1FA u8 inMessage  // ShowMessage ran and the box has not closed yet; MainLoop tests inMessage/selectWinOpen as one u16
// 0x1FC s16 rupees  // shown on screen; counts toward flags.rupees
// 0x1FE s16 rupeeDelta  // flags.rupees - rupees
// 0x202 u16 tingleTimer  // Tingle animation: frames left in the current frame
// 0x204 u16 tingleScaleWait  // animation 3: frames between scale bounces
// 0x206 u16 tingleScale  // Tingle's DrawSprite scale (0: none)
// 0x208 u16 tinglePulse  // animation 11: scale of the pulsing sprite at (0xCC, 0x0E)
// 0x20A u16 waveAmplitude  // intro wave (Mode1State1), counts down to 0
// 0x20E u16 wavePhase  // 8.8; the high byte indexes gSinTable
// 0x210 u16 timer  // flags.timer
// 0x212 u8 playerName[8]  // slot 10
// 0x21A u8 playerNameIndex  // next letter of playerName for message code 0xFB
// 0x21C u8 unk21C
// 0x21D u8 hudLeftSlide  // HUD panels (UpdateHudPanels): tiles covered on the left/right panel
// 0x21E u8 hudRightSlide
// 0x21F u8 hudLeftHidden  // the left/right HUD panel slides out
// 0x220 u8 hudRightHidden
// 0x221 u8 fadeTimer  // frames to the next mosaic/brightness step
// 0x222 u8 rToggle  // R toggles it (Mode1State2); nothing reads it
// 0x223 u8 unk223
// 0x224 u8 chartOpen  // the sea chart overlay is open (L/B, UpdateChartOverlay)
// 0x225 u8 shake  // bit 1 shakes the BGs by a line in VBlankIntr
// 0x226 u8 tingleAnim  // Tingle's animation (gTingleAnims index)
// 0x227 u8 tingleAnimPart  // sprite of the animation
// 0x228 u8 tingleFrame  // frame of that sprite
// 0x229 u8 tingleAngle  // Tingle's DrawSprite angle; never set
// 0x22A u8 tingleScaleDir  // scale animations: 0 shrinking, 1 growing
// 0x22B u8 animateMarkers  // Tingle is shown: DrawMapMarkers runs the animation timers
// 0x22C u8 tinglePulseGrow  // tinglePulse is growing
// 0x22D u8 scrollTableNext
// 0x22E u8 scrollTable  // which per-line scroll table the HBlank DMA reads
// 0x230 u8 life  // flags.life as last seen
// 0x231 u8 lowLifeWarned  // the low-life sound has played
// 0x232 u8 chimed  // the clock sound for this hour has played
// 0x233 u8 dungeonChests  // flags.dungeonChests
// 0x235 u8 lastMapImageMode  // gMain.mapImageMode at the last fade out (unread)
// 0x236 u8 isFree  // flags.isFree
// 0x237 u8 markerDelay  // frames Mode1State4 waits after a map image or control packet before taking markers
// 0x238 u8 hasMagic  // flags.hasMagic as last seen
// 0x239 u8 hasMagicChanged  // flags.hasMagic changed: show message 0x10 and the magic items
// 0x23B u8 flagsReceived
// 0x23C u8 afterLinkLost  // set by the link-lost screen: the next intro leaves BG3 off in a dungeon
// end genstruct
struct GameState
{
    /*0x000*/ struct GbaComFlags flags;       // last slot 9 packet from the GC
    /*0x00C*/ struct MapImage* mapImage;      // the slot 2 map image (LoadMapImage)
    /*0x010*/ u32 revealedType5;              // marker type 5 examined, bit per MapMarker.param (else drawn hidden)
    /*0x014*/ s16 mapX;                       // top left of the map view in the map image (pixels); BG3 view + 8 when streamed (ScrollMap)
    /*0x016*/ s16 mapY;
    /*0x018*/ u8 followStill;                 // cleared by UpdateMap; set when the followed marker stood still for 2 frames (unread)
    /*0x019*/ u8 followStillCount;            // frames to followStill; cleared when rToggle turns on (R)
    /*0x01A*/ u8 filler01A[0x01E - 0x01A];
    /*0x01E*/ s16 followLastX;                // followed marker's last x/y (FollowMarkerStreamedMap)
    /*0x020*/ s16 followLastY;
    /*0x022*/ u16 unk022;                     // set to 240 when a target is placed (MapIconCursor)
    /*0x024*/ u16 targetScale;                // pulse of the target compass sprite (UpdateTargetCompass)
    /*0x026*/ u16 revealedType16;             // marker type 16 examined, bit per MapMarker.param
    /*0x028*/ u16 seaAnimTimer[4];            // per quadrant of the view (UpdateMap)
    /*0x030*/ u16 squareMsgTimer;             // frames until squareMsgPending names the player's sea chart square
    /*0x032*/ s16 spriteX;                    // title sprite x (mode 0); the item effects' sprite position
    /*0x034*/ s16 spriteY;                    // title sprite y
    /*0x036*/ s16 sprite2X;                   // Effect12's second sprite
    /*0x038*/ s16 sprite2Y;
    /*0x03A*/ u16 switchMsg;                  // switch record sent in slot 8 (daAgb_Switch, daAgb_c::SwitchOn)
    /*0x03C*/ u8 switchNo;                    // 0xFF: none
    /*0x03D*/ u8 switchStageNo;
    /*0x03E*/ u8 switchRoomNo;
    /*0x03F*/ u8 switchType;                  // the GC switches on type - 1
    /*0x040*/ u8 switchNo2;
    /*0x041*/ u8 filler041[0x042 - 0x041];
    /*0x042*/ u8 unk042;
    /*0x043*/ u8 filler043[0x044 - 0x043];
    /*0x044*/ struct MapIcon icons[4];        // drawn over the map by UpdateMapIcons
    /*0x064*/ struct MapMarker markers[16];   // from slot 3 (Mode1RecvMarkers)
    /*0x0E4*/ u8 markerCount;
    /*0x0E5*/ u8 unk0E5;
    /*0x0E6*/ u8 filler0E6[0x0E8 - 0x0E6];
    /*0x0E8*/ u8 seaTileSet[4];               // gSeaSquareTileSets of each quadrant's square
    /*0x0EC*/ u8 seaAnimFrame[4];             // index into gSeaAnimFrames
    /*0x0F0*/ u8 seaSquare[4];                // sea chart square of each quadrant; in a dungeon, the room in each BG map block at 0x0600C000
    /*0x0F4*/ u8 visitedSquares[8];           // slot 11: one bit per sea chart square (7x7) Link has arrived at (GC dComIfGs_isSaveArriveGridForAgb)
    /*0x0FC*/ u8 aButtonAction;               // what A does, and the left HUD panel label: 0 examine the selection, 1 item 16 (look at the cursor), 2 sea chart open
    /*0x0FD*/ u8 hudChartLabel;               // the right HUD panel shows the chart label instead of the item (chart open)
    /*0x0FE*/ u8 filler0FE[0x0FF - 0x0FE];
    /*0x0FF*/ u8 windArrowTimer;              // frames left in the frame of the wind arrow (gWindArrowSprite, while flags.showWind)
    /*0x100*/ u8 windArrowFrame;              // its frame (0-3)
    /*0x101*/ u8 targetBlinkOn;
    /*0x102*/ u8 targetDir;                   // direction (0-7) last announced by UpdateTargetCompass; 0xFF: none
    /*0x103*/ u8 targetBlinkTimer;
    /*0x104*/ u8 active;                      // the Tuner takes input: the GC's agbIsActive() (gcActive), or 1 when nothing is up; cleared while the GC is busy
    /*0x105*/ u8 placingTarget;               // the sea chart cursor (icons[1]) is placing a target (A in the chart)
    /*0x106*/ u8 squareMsgPending;            // Link entered another sea chart square: name it after squareMsgTimer
    /*0x107*/ u8 targetNear;                  // the near sound has played
    /*0x108*/ u8 playerBlinkTimer;            // DrawMapMarkers's timers: count up, flip the frame byte after them (player 20, enemy 10, cursor 30 frames)
    /*0x109*/ u8 playerFrame;
    /*0x10A*/ u8 enemyBlinkTimer;
    /*0x10B*/ u8 enemyFrame;
    /*0x10C*/ u8 cursorBlinkTimer;
    /*0x10D*/ u8 cursorFrame;
    /*0x10E*/ u8 mapLoaded;                   // a map image is loaded (0: none yet, e.g. after the GC relinks in a room with no map)
    /*0x10F*/ u8 type9Timer;                  // DrawMapMarkers: frame timers and frames of the gSpriteTable[9]/[13]/[16]/[19] animations
    /*0x110*/ u8 type9Frame;
    /*0x111*/ u8 filler111[0x113 - 0x111];
    /*0x113*/ u8 type13Timer;
    /*0x114*/ u8 type13Frame;
    /*0x115*/ u8 type16Timer;
    /*0x116*/ u8 type16Frame;
    /*0x117*/ u8 friendTimer;
    /*0x118*/ u8 friendFrame;
    /*0x119*/ u8 selectedMarker;              // index into markers of the selection (UpdateSelection); 0: none
    /*0x11A*/ u8 selection;                   // 0 nothing under the cursor, 1 something selectable, 2 examined (its message is up)
    /*0x11B*/ u8 selectedType;                // the selected marker's type
    /*0x11C*/ u8 revealedType12;              // marker type 12 examined
    /*0x11D*/ u8 revealedType13;              // marker type 13 examined
    /*0x11E*/ u8 placeTargetAtCursor;         // choice message 0x1A7 picked: Mode1State2 puts the target on the cursor
    /*0x11F*/ u8 mapScrolling;                // L pressed to scroll the map (UpdateMap)
    /*0x120*/ u8 viewLocked;                  // the streamed map view stops following the marker (item effects)
    /*0x121*/ u8 holdState;                   // the cursor held by the GC (mail type 12): 1 gCursorHeldSprite loops, 2 gCursorReleaseSprite plays out, then a switch record
    /*0x122*/ u8 holdMsgIndex;                // item effect 16 while holding: message 0x60 + this (up to 3)
    /*0x123*/ u8 holdTimer;
    /*0x124*/ u8 holdFrame;
    /*0x125*/ u8 char2Timer;                  // frame timers and frames of the character icons 2-4 (MapMarker.param of players and friends)
    /*0x126*/ u8 char2Frame;
    /*0x127*/ u8 char4Timer;
    /*0x128*/ u8 char4Frame;
    /*0x129*/ u8 char3Timer;
    /*0x12A*/ u8 char3Frame;
    /*0x12B*/ u8 effectStep;                  // item effect step (the balloon/shield, Effect5, Effect8, Effect12)
    /*0x12C*/ u8 introTimer;                  // StartEffectIntro animation (gEffectIntroSprite): frame timer and frame
    /*0x12D*/ u8 introFrame;
    /*0x12E*/ u8 balloonTimer;                // DrawBalloonShield: balloon and shield frame timers and frames
    /*0x12F*/ u8 balloonFrame;
    /*0x130*/ u8 shieldTimer;
    /*0x131*/ u8 shieldFrame;
    /*0x132*/ u8 effectDelay;                 // frames before the next effect step
    /*0x133*/ u8 gcActive;                    // slot 3 byte 1: the GC's dMap_c::agbIsActive()
    /*0x134*/ u8 scrolling;                   // the view is still catching up with the followed marker
    /*0x135*/ u8 selectedSquare;              // sea chart square under the cursor + 1 (UpdateSelection), 0: none
    /*0x136*/ u8 awayFromType20;              // the cursor has left the type 20 marker (it is only selectable after that)
    /*0x137*/ u8 filler137[0x138 - 0x137];
#ifdef VERSION_PAL
    /*0x138*/ s16 pendingMsg;                 // PAL (fields from here on are 8 bytes higher): ExamineSelection saves the selection's message, type and param for after reactionTimer, so moving the cursor meanwhile doesn't change what is shown and revealed
    /*0x13A*/ u8 pendingType;
    /*0x13B*/ u8 pendingParam;
    /*0x13C*/ u8 pendingReveal;               // set with them; Mode1State2 reveals pendingType/Param when the message closes
    /*0x13D*/ u8 filler13D[0x140 - 0x13D];
#endif
    /*0x138*/ u8 unk138_0:2;                  // sent to the GC as GbaFlg.bits: appeared shows "Tingle appeared" (daAgb field_0x67a); the others set the GC's event flags (flags.shopUnlocked etc.)
              u8 appeared:1;
              u8 shopUnlocked:1;
              u8 balloonDiscount:1;
              u8 tingDiscount:1;
              u8 unk138_6:2;
    /*0x139*/ u8 filler139[0x13C - 0x139];
    /*0x13C*/ u8 msgOpen;                     // a message is up or opening (the box slides in); 0 closes it
    /*0x13D*/ s8 msgBoxPos;                   // message box slide, steps toward 10 while msgOpen is set, else 0
    /*0x13E*/ u16 msgWaitTimer;               // frames at a full page; past 0x12B the page turns by itself (counts only while flags.gcInput)
    /*0x140*/ u16 msgLoopSound;               // loop sound of the message (message code F6 20-22), 0xFFFF: none
    /*0x142*/ s16 msgId;                      // the message shown (MsgStart), 0xFFFF: none
    /*0x144*/ const u8* msgPtr;               // message text (in the EWRAM message block at 0x02000000); US Mode2State0 sets it too
    /*0x148*/ u8 textX;                       // text cursor, pixels
    /*0x149*/ u8 textY;
    /*0x14A*/ u8 textColor;                   // gTextColorMasks index (message code FC)
    /*0x14B*/ u8 msgStatus;                   // last MsgStep result (0 go on, 1 after F6 or while opening, 2 wait, 3 end)
    /*0x14C*/ u8 textDelay;                   // frames per character (textWait reload)
    /*0x14D*/ u8 textWait;                    // frames before the next character (message code F9)
    /*0x14E*/ u8 choiceIndex;                 // choice options seen so far (message codes F7/F8)
    /*0x14F*/ u8 choiceCursor;                // choice menu cursor, 0xFF: none
    /*0x150*/ u8 choiceY;                     // text y of the first option
    /*0x151*/ u8 choiceMenu;                  // 1: two choices, 2: three
    /*0x152*/ u8 choiceCursorTimer;           // choice cursor sprite (gChoiceCursorSprite) frame timer and frame
    /*0x153*/ u8 choiceCursorFrame;
    /*0x154*/ u8 filler154[0x156 - 0x154];
    /*0x156*/ u8 reactionTimer;               // ExamineSelection's thinking pause before the selection's message (loop sound 6)
    /*0x157*/ u8 glyphWidth;
    /*0x158*/ u8 textFast;                    // message codes F3 0D/0E: draw the text without waiting
    /*0x159*/ u8 charLoopSound;               // the last character was JP 0xBC / US 0x3F: MsgStep keeps loop sound 0 playing
    /*0x15A*/ u8 textBig;                     // text drawn 2x (message code F3 0F)
    /*0x15B*/ u8 filler15B[0x15C - 0x15B];
    /*0x15C*/ u8 effect[4];                   // slot 7: item use result
    /*0x160*/ struct ItemSlot items[12];
    /*0x1C0*/ u16 menuCursorX;                // item menu cursor position (8.8), moving toward the slot
    /*0x1C2*/ u16 menuCursorY;
    /*0x1C4*/ u16 menuCursorDX;
    /*0x1C6*/ u16 menuCursorDY;
    /*0x1C8*/ u16 effectTimer;                // item effect timer, counts down in UpdateItemEffect (held while the GC menu is open)
    /*0x1CA*/ u16 chestTimer;                 // chest counter timer (ChestCounterMain)
    /*0x1CC*/ u8 menuOpen;                    // the item menu is open (START)
    /*0x1CD*/ u8 menuCursor;                  // item menu slot (5 x 3; row 2 is the dungeon chest row)
    /*0x1CE*/ u8 menuKeyRepeat;               // frames to the next d-pad repeat
    /*0x1CF*/ u8 slotItems[13];               // item shown in each menu slot (SetSlotItem); rows 0-1 index items
    /*0x1DC*/ u8 filler1DC[0x1DE - 0x1DC];
    /*0x1DE*/ u8 selectedItem;                // item on B (the right HUD panel)
    /*0x1DF*/ u8 pickedEmptySlot;             // B on an empty menu slot (a d-pad press clears it); never read
    /*0x1E0*/ u8 chestState;                  // dungeon chest counter state (0 off, 1 start, 2 blink, 3 wait)
    /*0x1E1*/ u8 newChests;                   // chest bits that changed
    /*0x1E2*/ u8 dialogStep;                  // item effect step (Effect7, Effect11)
    /*0x1E3*/ u8 chestCount;                  // chests opened
    /*0x1E4*/ u8 savedEffect;                 // Effect8's effect id while the GC answers request 0x14 (answer 20 restores it)
    /*0x1E5*/ u8 itemFlying:1;                // itemFlying: the picked icon flies to the B panel; itemsEnabled: no item effect runs (B/A/R work); effectActive: UpdateItemEffect runs effect[0]; onSea: item 11's sea look (ItemMenuInit); menuSliding: BG1 (the menu) still sliding; msgTest: GZLE01_ud message test
              u8 itemsEnabled:1;
              u8 effectActive:1;
              u8 onSea:1;
              u8 menuSliding:1;
              u8 msgTest:1;
              u8 unk1E5_6:2;
    /*0x1E6*/ u8 filler1E6[0x1E8 - 0x1E6];
    /*0x1E8*/ u16 selectWinX;                 // select window edge x (0xF8 closed, 0xFFF8 fully across)
    /*0x1EA*/ s16 selectWinY;                 // select window edge y
    /*0x1EC*/ u8 filler1EC[0x1EE - 0x1EC];
    /*0x1EE*/ u8 selectWinStep;               // UpdateSelectWindow step (0: closed)
    /*0x1EF*/ u8 filler1EF[0x1F4 - 0x1EF];
    /*0x1F4*/ u8 statusIcon;                  // DrawStatusIcon: which icon
    /*0x1F5*/ u8 enemyHighlighted;            // an enemy marker (type 2) has its highlight set
    /*0x1F6*/ u8 filler1F6[0x1F8 - 0x1F6];
    /*0x1F8*/ s16 nextMsg;                    // message shown after the current one (switch type 15), 0xFFFF: none
    /*0x1FA*/ u8 inMessage;                   // ShowMessage ran and the box has not closed yet; MainLoop tests inMessage/selectWinOpen as one u16
    /*0x1FB*/ u8 selectWinOpen;               // the Select window is up
    /*0x1FC*/ s16 rupees;                     // shown on screen; counts toward flags.rupees
    /*0x1FE*/ s16 rupeeDelta;                 // flags.rupees - rupees
    /*0x200*/ u8 filler200[0x202 - 0x200];
    /*0x202*/ u16 tingleTimer;                // Tingle animation: frames left in the current frame
    /*0x204*/ u16 tingleScaleWait;            // animation 3: frames between scale bounces
    /*0x206*/ u16 tingleScale;                // Tingle's DrawSprite scale (0: none)
    /*0x208*/ u16 tinglePulse;                // animation 11: scale of the pulsing sprite at (0xCC, 0x0E)
    /*0x20A*/ u16 waveAmplitude;              // intro wave (Mode1State1), counts down to 0
    /*0x20C*/ u8 filler20C[0x20E - 0x20C];
    /*0x20E*/ u16 wavePhase;                  // 8.8; the high byte indexes gSinTable
    /*0x210*/ u16 timer;                      // flags.timer
    /*0x212*/ u8 playerName[8];               // slot 10
    /*0x21A*/ u8 playerNameIndex;             // next letter of playerName for message code 0xFB
    /*0x21B*/ u8 filler21B[0x21C - 0x21B];
    /*0x21C*/ u8 unk21C;
    /*0x21D*/ u8 hudLeftSlide;                // HUD panels (UpdateHudPanels): tiles covered on the left/right panel
    /*0x21E*/ u8 hudRightSlide;
    /*0x21F*/ u8 hudLeftHidden;               // the left/right HUD panel slides out
    /*0x220*/ u8 hudRightHidden;
    /*0x221*/ u8 fadeTimer;                   // frames to the next mosaic/brightness step
    /*0x222*/ u8 rToggle;                     // R toggles it (Mode1State2); nothing reads it
    /*0x223*/ u8 unk223;
    /*0x224*/ u8 chartOpen;                   // the sea chart overlay is open (L/B, UpdateChartOverlay)
    /*0x225*/ u8 shake;                       // bit 1 shakes the BGs by a line in VBlankIntr
    /*0x226*/ u8 tingleAnim;                  // Tingle's animation (gTingleAnims index)
    /*0x227*/ u8 tingleAnimPart;              // sprite of the animation
    /*0x228*/ u8 tingleFrame;                 // frame of that sprite
    /*0x229*/ u8 tingleAngle;                 // Tingle's DrawSprite angle; never set
    /*0x22A*/ u8 tingleScaleDir;              // scale animations: 0 shrinking, 1 growing
    /*0x22B*/ u8 animateMarkers;              // Tingle is shown: DrawMapMarkers runs the animation timers
    /*0x22C*/ u8 tinglePulseGrow;             // tinglePulse is growing
    /*0x22D*/ u8 scrollTableNext;
    /*0x22E*/ u8 scrollTable;                 // which per-line scroll table the HBlank DMA reads
    /*0x22F*/ u8 filler22F[0x230 - 0x22F];
    /*0x230*/ u8 life;                        // flags.life as last seen
    /*0x231*/ u8 lowLifeWarned;               // the low-life sound has played
    /*0x232*/ u8 chimed;                      // the clock sound for this hour has played
    /*0x233*/ u8 dungeonChests;               // flags.dungeonChests
    /*0x234*/ u8 filler234[0x235 - 0x234];
    /*0x235*/ u8 lastMapImageMode;            // gMain.mapImageMode at the last fade out (unread)
    /*0x236*/ u8 isFree;                      // flags.isFree
    /*0x237*/ u8 markerDelay;                 // frames Mode1State4 waits after a map image or control packet before taking markers
    /*0x238*/ u8 hasMagic;                    // flags.hasMagic as last seen
    /*0x239*/ u8 hasMagicChanged;             // flags.hasMagic changed: show message 0x10 and the magic items
    /*0x23A*/ u8 filler23A[0x23B - 0x23A];
    /*0x23B*/ u8 flagsReceived;
    /*0x23C*/ u8 afterLinkLost;               // set by the link-lost screen: the next intro leaves BG3 off in a dungeon
    /*0x23D*/ u8 filler23D[0x240 - 0x23D];
};

extern struct GameState gGameState;

// EWRAM below the program: per-line BG0HOFS tables that VBlankIntr's HBlank DMA reads
// (gGameState.scrollTable picks one).
extern u16 gBgScrollTables[][160];
// The same tables by constant address, as the game code addresses them (see GBACOM_BUF_MARKERS).
#define BG_SCROLL_TABLE_0 ((u16 *)(WORK_TOP - 0x4C0))
#define BG_SCROLL_TABLE_1 ((u16 *)(WORK_TOP - 0x380))

// Graphics in .rodata (LZ77-compressed except the palettes).
extern const u8 gMode1BgTiles[];
extern const u16 gMode1BgPalette[0x100];
extern const u8 gMode1BgMap[];
extern const u8 gMode1SeaMap[];
extern const u8 gMode2BgMap[];
extern const u8 gMode1ObjTiles[];
extern const u16 gMode1ObjPalette[0x100];
extern const u8 gWaitSprite[];
extern const u8 gTitleSprite[];
extern const u8 *const gSpriteTable[];

void Mode1VBlank(void);
void DrawFloorNumber(void);
// Shows message id (0-0x437) with Tingle's voice; arg goes to gGameState.tingleAnim (his animation).
// The definition narrows id to u16, but callers see an s16 parameter (they sign-extend).
bool32 ShowMessage(s16 id, u8 arg);
bool32 InitMapView(void);
void UpdateMapView(void);
bool32 StepToward(s16 *pos, s16 target, s16 speed);
void UpdateSelectWindow(void);
// sprite points to struct SpriteFrame[]; scale is 8.8 (0: no affine unless angle is set).
// Callers see s16 x and y and a wider mosaic (DrawQuestMarkers passes its 0/1 flag without
// the copy agbcc makes for a u8 argument).
void DrawSprite(const void *sprite, u8 frame, s16 x, s16 y, u8 angle, u32 mosaic, u16 scale, s16 ofsX,
                s16 ofsY, u8 tileOfs, u8 palette);
// sprite points to struct SpriteFrame[]; palette 0 keeps each part's palette, n uses palette n - 1
void DrawSpriteAt(const void *sprite, u8 frame, s16 x, s16 y, u8 mosaic, u8 palette, u8 priority);
bool32 LoadMapImage(void);  // returns nothing (falls off the end)
bool32 ItemMenuInit(void);
void ResetSelectWindow(void);
bool32 MsgReset(void);
void BuildWaveScrollTable(u16 *table, u8 phase, u16 amplitude);

// 16 objects in IWRAM (0x03005650).
struct Obj
{
    /*0x00*/ s32 x;        // 8.8
    /*0x04*/ s32 y;        // 8.8
    /*0x08*/ s32 z;        // 8.8 height, <= 0; drawn at y + z
    /*0x0C*/ s16 vx;
    /*0x0E*/ s16 vy;
    /*0x10*/ s16 vz;
    /*0x12*/ u16 unk12;    // set to 2 by ObjBurstInit, never read
    /*0x14*/ u16 unk14;    // only cleared
    /*0x16*/ u16 scale;    // DrawSprite scale (8.8)
    /*0x18*/ s16 ofsX;     // DrawSprite rotation centre offset
    /*0x1A*/ s16 ofsY;
    /*0x1C*/ u16 piece;    // ObjBurst: which of the five pieces
    /*0x1E*/ u8 angle;     // DrawSprite angle
    /*0x1F*/ u8 active;    // 0: free, else 1 + index into the type's state table
    /*0x20*/ u8 type;      // index into gObjFuncs
};

extern struct Obj gObjs[16];

// 0x03005640, just before gObjs: the title screen's blend ramp.
struct Mode0Work
{
    /*0x0*/ u8 blend;      // BLDALPHA EVA (EVB = 16 - blend)
    /*0x1*/ u8 timer;      // frames to the next step
    /*0x2*/ u8 fadingOut;  // 0: blend counts up to 16, 1: down to 1
    /*0x3*/ u8 linkTimeout;  // frames left to receive the message (slot 0)
    /*0x4*/ u8 filler4[0xC];
};

extern struct Mode0Work gMode0Work;

// One entry of the slot 14 data: the point of one sea chart square (its island), which the
// GC's dMap_c::mapAGBSendIslandData builds from the stage's fmap check points
// (mFmapChkPntData); unused entries are {0, 0, 0, 0xFF}.
struct SeaChartEntry
{
    /*0x0*/ u8 x;       // pixels within its square
    /*0x1*/ u8 y;
    /*0x2*/ u8 big;     // UpdateSelection: within 32 pixels (on an 8-pixel grid) rather than 8
    /*0x3*/ u8 square;  // sea chart square (row * 7 + column); its name message is 0x14E + square
};

// Slot 14 data (IWRAM 0x03002FB0), received once at boot by Mode0State3: one entry per
// sea chart square.
extern struct SeaChartEntry gChartPoints[49];

#ifndef VERSION_GZLJ01
void DrawString(const u8 *str, u8 a1, u8 a2, u8 a3);
#endif

void ObjReset(struct Obj *obj);
void ObjResetActive(void);  // ObjReset on every active entry
void UpdateMap(void);
void UpdateMsgAndTingle(void);
#ifdef VERSION_PAL
s32 ShowSelectionMsg(void);
#else
void ShowSelectionMsg(void);
#endif
void MsgClose(void);
bool32 UseItem(u8 item);
void UpdateChartOverlay(void);
void ExamineSelection(void);
u8 FindMarker(u8 type);
void ItemMenuUpdate(void);
void DrawStatusIcon(void);
s32 UpdateSelection(void);
void PlayMsgSound(u8 id);
void StartTingleAnim(void);
void SetSlotItem(u8 a, u8 b);
void UpdateHudPanels(void);
void EndItemEffect(void);
void StartBombBlast(void);
void StartBalloonEffect(void);
void StartShieldEffect(void);
void StartLookEffect(void);
void StartRandomEffect(void);
void StartShopEffect(void);
void StartBombCountdown(void);
void UpdateItemEffect(void);
extern const u8 gCursorHeldSprite[];
extern const u8 gCursorReleaseSprite[];

// ---- GZLJ01 02018884-0201A224: sprite.c, start of map.c ----

s16 EaseTowardMinMax(s16 *pos, s16 target, s16 div, s16 maxStep, s16 minStep);
void EaseTowardMax(s16 *pos, s16 target, s16 div, s16 maxStep);
bool32 StepTowardS8(s8 *pos, s8 target, s8 speed);
bool32 StepTowardAngle(s16 *pos, s16 target, s16 speed);
u8 CountDownU8(u8 *timer);
u16 CountDownU16(u16 *timer);

void CopyMapColumn(s16 x, s16 y, const u16 *src);
void CopyMapRow(s16 y, s16 x, const u16 *src);
void ScrollMap(struct GameState *state);
void DrawSpriteParts(const void *parts, s16 x, s16 y, u8 mosaic, u8 priority);
void FollowMarkerFixedMap(void);
void FollowMarkerStreamedMap(void);
void DrawMapMarkers(void);
void ObjUpdateAll(void);
void Mode1SendSwitch(u8 soundArg);


// A hardware OAM entry, as DrawSprite and DrawItemSprite write it into gOamBuffer.
struct OamData
{
    /*0x0*/ u32 y:8;
            u32 affineMode:2;
            u32 objMode:2;
            u32 mosaic:1;
            u32 bpp:1;
            u32 shape:2;
    /*0x2*/ u32 x:9;
            u32 matrixNum:3;
            u32 hFlip:1;
            u32 vFlip:1;
            u32 size:2;
    /*0x4*/ u16 tileNum:10;
            u16 priority:2;
            u16 paletteNum:4;
    /*0x6*/ u16 affineParam;
};

// One OBJ of a sprite frame: OAM attributes relative to the sprite's position.
struct SpritePart
{
    /*0x0*/ u8 y;
    /*0x1*/ u8 affineMode:2;
            u8 objMode:2;
            u8 mosaic:1;
            u8 bpp:1;
            u8 shape:2;
    /*0x2*/ s8 x;
    /*0x3*/ u8 xHi:1;
            u8 matrixNum:3;
            u8 hFlip:1;
            u8 vFlip:1;
            u8 size:2;
    /*0x4*/ u16 tileNum:10;
            u16 priority:2;
            u16 paletteNum:4;
    /*0x6*/ u16 end;  // 0xFFFF: past the last part
};

// What DrawSprite's sprite argument points to (indexed by frame).
struct SpriteFrame
{
    /*0x0*/ const struct SpritePart *parts;
    /*0x4*/ u8 duration;  // frames
    /*0x5*/ u8 unk5[3];
};

// sin(i * 2pi / 256) in 8.8 fixed point; 320 entries, so cos(i) = (gSinTable + 64)[i].
extern const s16 gSinTable[];

// Sprites that only the .rodata tables point at (gSpriteTable, gMapIconSprites, gTingleAnims).
extern const u8 gMarkerType1Sprite[];
extern const u8 gMarkerType3Sprite[];
extern const u8 gMarkerType2Sprite[];
extern const u8 gMarkerType19Sprite[];
extern const u8 gMarkerType17Sprite[];
extern const u8 gMarkerType18Sprite[];
extern const u8 gMarkerType4Sprite[];
extern const u8 gMarkerType11Sprite[];
extern const u8 gMarkerType10Sprite[];
extern const u8 gMarkerType8Sprite[];
extern const u8 gMarkerType5Sprite[];
extern const u8 gMarkerType7Sprite[];
extern const u8 gMarkerType9Sprite[];
extern const u8 gMarkerType6Sprite[];
extern const u8 gMarkerType12Sprite[];
extern const u8 gMarkerType13Sprite[];
extern const u8 gMarkerType21Sprite[];
extern const u8 gMarkerType15Sprite[];
extern const u8 gMarkerType14Sprite[];
extern const u8 gMapCursorIconSprite[];
extern const u8 gFollowIconSprite[];
extern const u8 gMarkerType16Sprite[];
extern const u8 gMarkerType20Sprite[];
extern const u8 gSprite_0202C788[];
extern const u8 gSprite_0202C7C8[];
extern const u8 gSprite_0202C850[];
extern const u8 gSelectionIconSprite[];

// Sprites (struct SpriteFrame[]) and tables in .rodata used by DrawMapMarkers.
extern const struct SpriteFrame gSprite_0202BD80[];
extern const struct SpriteFrame gMarkerHighlightSprite[];
extern const struct SpriteFrame gChoiceCursorSprite[];
extern const struct SpriteFrame gTargetSprite[];
extern const struct SpriteFrame gCharIcon0[];
extern const struct SpriteFrame gCharIcon1[];
extern const struct SpriteFrame gCharIcon2[];
extern const struct SpriteFrame gCharIcon4[];
extern const struct SpriteFrame gCharIcon3[];
extern const struct SpriteFrame gFriendIcon0[];
extern const struct SpriteFrame gCursorSelectSprite[];
extern const s16 gMarkerDirOfsX[8];
extern const s16 gMarkerDirOfsY[8];

// ---- GZLJ01 0201A224-0201AFCC: map.c ----

// The map image the GC sends in slot 2, in the message buffer (gGbaComMessageBuf). Its two
// data pointers arrive as offsets from the header; LoadMapImage turns them into addresses.
struct MapImage
{
    /*0x00*/ u16 width;       // pixels
    /*0x02*/ u16 height;
    /*0x04*/ u8 filler4[2];
    /*0x06*/ s8 floor;        // dungeon floor (< 0: basement); 0: none
    /*0x07*/ u8 filler7;
    /*0x08*/ u8 *tiles;
    /*0x0C*/ s32 tilesSize;   // bytes
    /*0x10*/ u8 filler10[0x20];
    /*0x30*/ u8 mapWidth;     // tiles per map row
    /*0x31*/ u8 filler31[3];
    /*0x34*/ u16 *map;
};

#define GBACOM_BUF_MESSAGE ((u8 *)(WORK_TOP - 0xA800))

u16 Sqrt(u32 x);
void MapIconFollowMarker(struct MapIcon *icon);
void InitMarkerIcon(void);
void MapIconBlink(struct MapIcon *icon);
void InitTargetIcon(void);
void InitCursorIcon(void);
void MapIconCursor(struct MapIcon *icon);
void UpdateMapIcons(void);
extern void (*const gMapIconFuncs[])(struct MapIcon *icon);
extern const u8 *const gMapIconSprites[];
extern const u8 gTargetBlinkPeriods[5];
extern const u8 gCompassArrowSprite[];
extern const u8 gMapScrollCursorSprite[];
extern const u16 gSeaQuadrantOffsets[4][2];
extern const u8 gSeaSquareTileSets[49];
extern const u8 gSeaAnimFrames[5];
extern const u8 gSeaAnimPeriods[3];
extern const u8 gSeaAnimTiles[0x2000];
void DarkenUnvisitedSquares(void);
void HighlightMapRooms(void);
u16 ArcTan2(s32 x, s32 y);

// ---- GZLJ01 0201AFCC-0201C660: end of map.c, start of message.c ----
void DrawMsgBoxRow(s8 row);
void RestoreMsgBoxRow(s8 row);
void TextClearCanvas(void);
void MsgNewPage(u8 playSound);
void TextBlitGlyph(u8 x, u8 y, u8 color, u8 rows, const u32 *src, u32 *dst);
void TextScaleGlyph(s32 count, const u8 *src);
void TextDrawSmallGlyph(u8 x, u8 y, u8 color, u16 ch);
void TextDrawGlyph(u8 x, u8 y, u8 color, u16 ch);
void DrawChoiceCursor(void);
// EWRAM below the program, after the link buffers: a 16x8 glyph scaled up by TextScaleGlyph.
#define TEXT_GLYPH_BUF ((u16 *)(WORK_TOP - 0x540))
// A 4bpp pixel pair doubled horizontally: 0xAB -> 0xAABB.
#define EXPAND_NIBBLES(b) (((((b) & 0xF0) + (((b) & 0xF0) >> 4)) << 8) + (((b) & 0x0F) + (((b) & 0x0F) << 4)))
extern const u32 gTextColorMasks[];
extern const u16 gMsgSoundIds[];
extern const s16 gFriendMsgIds[];
void DrawQuestMarkers(void);
// One OAM entry as bitfields (gOamBuffer holds 128 of them).
struct OamData16
{
    /*0x0*/ u16 y:8;
            u16 affine:1;
            u16 doubleSize:1;
            u16 objMode:2;
            u16 mosaic:1;
            u16 bpp:1;
            u16 shape:2;
    /*0x2*/ u16 x:9;
            u16 matrixNum:3;
            u16 hFlip:1;
            u16 vFlip:1;
            u16 size:2;
    /*0x4*/ u16 tileNum:10;
            u16 priority:2;
            u16 paletteNum:4;
    /*0x6*/ u16 affineParam;
};
extern const u8 gPearlMarkerPos[6];
extern const u8 gTriangleIslandPos[6];
extern const u8 gTempleMarkerPos[4];
extern const u16 gTowerChartTiles[2];
s32 ChoiceMenuUpdate(void);  // the definition returns u8
// Per question (msgId): the ids, then the sound per choice (0xFFFF: none).
extern const u16 gTwoChoiceIds[];
extern const u16 gTwoChoiceMsgIds[];  // [id index * 2 + choice]
extern const u16 gThreeChoiceIds[];
extern const u16 gThreeChoiceMsgIds[];  // [id index * 3 + choice]
void UpdateTargetCompass(void);
void RedrawHudItem(void);

// ---- GZLJ01 0201C660-0201DEAC: message.c, start of item_menu.c ----
// Text: glyphs are drawn at gGameState.textX/textY (pixels) in a window 0x60 wide and 0x50
// high; glyphWidth is the glyph width in pixels (6 or 8) and textBig doubles the size.
bool32 TextWrapLine(void);  // wraps the cursor to the next line; FALSE when the window is full
void DrawChestRow(void);
bool32 SendItemUse(u8 a, u8 b);
void DrawGlyphRun(u8 id);
bool32 DrawChar(u8 ch);
#ifdef VERSION_GZLJ01
#define GLYPH_DIGIT_0 0x76
#define DRAW_CHAR DrawChar
#else
#define GLYPH_DIGIT_0 0x35
bool32 UsDrawChar(u8 ch);
#define DRAW_CHAR UsDrawChar  // the US callers go through this wrapper
#endif
s32 MsgStep(void);
void MsgStart(s16 id);
void DrawItemIcon(u8 item, u16 *map);
void DrawItemCount(u8 item, u16 *map);
void SetSelectedItem(u8 item);

// gGameState.items ... slotItems (0x160-0x1DB) seen as one block: SetSlotItem addresses
// slotItems through a pointer to it (base + 0x6F), which is how the original loads it.
struct ItemMenu
{
    /*0x00*/ struct ItemSlot items[13];
    /*0x68*/ u16 effectTimer;  // gGameState.effectTimer
    /*0x6A*/ u16 chestTimer;   // gGameState.chestTimer
    /*0x6C*/ u8 menuOpen;      // gGameState.menuOpen
    /*0x6D*/ u8 filler6D[2];
    /*0x6F*/ u8 slotItems[13];
};

extern const u16 gMsgTingleAnims[];  // message code 0xF5: argument -> gGameState.tingleAnim
extern const u16 gGlyphRunStart[];   // DrawGlyphRun: first glyph of each run
extern const u8 gGlyphRunLength[];
extern const u8 gVoicedKanaGlyphs[];  // DrawChar: glyphs for codes 0xE0-0xE5 and 0xEB-
extern const u8 gDayNameChars[7 * 2];    // two glyphs per weekday (flags.dayOfWeek), JP only
extern void (*const gMsgCmdFuncs[])(void);  // message code 0xF3: MsgCmd00 ... MsgCmd12
void MsgCmd00(void);
void MsgCmd01(void);
void DrawWeekday(void);
void DrawHour(void);
void DrawMinute(void);
void MsgCmd05(void);
void MsgCmd06(void);
void MsgCmd07(void);
void MsgCmd08(void);
void MsgCmd09(void);
void MsgCmd0A(void);
void MsgCmd0B(void);
void MsgCmd0C(void);
void MsgCmd0D(void);
void MsgCmd0E(void);
void MsgCmd0F(void);
void MsgCmd10(void);
void MsgCmd11(void);
void MsgCmd12(void);
extern const u8 gSlotCursorSprite[];  // menu slot cursor (DrawSlotCursor)
extern const u8 gMsgNextArrowSprite[];  // "more text" arrow (MsgStep)

void Mode1StartFadeOut(void);
void DrawMsgBoxFrame(s8 pos);
// Tingle's animations (gGameState.tingleAnim), each a list of sprites (gGameState.tingleAnimPart picks one).
extern const u8 *const *const gTingleAnims[15];
extern const u8 gWindArrowSprite[];
extern const u8 gSprite_0202C650[];
extern const u8 gTingleSprite[];

// ---- GZLJ01 0201DEAC-0201F308: item_menu.c, start of effect.c ----

extern const struct ItemSlot gShopItemsInit[12];

#define GBACOM_BUF_ITEM ((u8 *)(WORK_TOP - 0x6C0))

// Animations (they overlap: each starts somewhere in one frame table).
extern const struct SpriteFrame gBalloonSprite[];
extern const struct SpriteFrame gShieldSprite[];
extern const struct SpriteFrame gEffectIntroSprite[];
extern const u8 gBombBlastSprite[];

void EffectBombBlast(void);
void StartEffectIntro(void);
void DrawEffectIntro(void);
void DrawBalloonShield(s16 x, s16 y, u8 kind);
void EffectWaitTimer(void);
// The definition takes a u16; callers see s16 (they sign-extend).
bool32 MsgStartSilent(s16 id);
void DrawSlotCursor(u8 cursor);
void DrawItemSprite(u8 item, u8 x, u8 y, u8 mosaic);
void ChestCounterMain(void);
extern const s16 gChestRowIds[5];
extern const u16 gHudPanelTiles[3];
u32 MarkerDistance(u8 arg);
void StartEffect11(void);
void StartTingEffect(void);
// The definition takes u16 x and y; callers see s16 (they sign-extend).

// ---- GZLJ01 0201F308-02020E28: effect.c ----

extern void (*const gMode2StateFuncs[])(void);
// Per object type: its function, called every frame while the object is active.
extern void (*const gObjFuncs[])(struct Obj *obj);

void ObjDecelX(struct Obj *obj, s16 decel);
void ObjDecelY(struct Obj *obj, s16 decel);
void ObjFall(struct Obj *obj, s16 gravity);
void ObjUpdateAll(void);
s32 ObjFindFree(void);
bool32 ObjSpawn(u8 type);
extern const u8 gSelectWindowEdgeSprite[];
extern const u8 gSprite_0202C848[];
void Mode2State0(void);
void Mode2State1(void);
// The message data (slot 0, msg_LZ.bin), unpacked to the start of EWRAM by Mode0State1.
#define MSG_DATA ((const u8 *)0x02000000)
// Object type 0 (ObjBurst): five pieces, per piece (Obj.piece) an offset and a velocity.
extern const s16 gObjBurstOfsX[5];
extern const s16 gObjBurstOfsY[5];
extern const s16 gObjBurstVelX[5];
extern const s16 gObjBurstVelY[5];
extern void (*const gObjBurstStateFuncs[])(struct Obj *obj);
extern const u8 gBurstPieceSprite[];
void ObjBurstInit(struct Obj *obj);
void ObjBurstFly(struct Obj *obj);
void ObjBurst(struct Obj *obj);
#ifndef VERSION_GZLJ01
extern const u8 gMode2UsString[];
#endif
// Per item effect (GameState.effect[0], slot 7), run every frame by UpdateItemEffect.
extern void (*const gItemEffectFuncs[18])(void);
void EffectBalloonShield(void);
void Effect5(void);
void Effect7(void);
void Effect8(void);
void Effect11(void);
void Effect12(void);
void Effect14(void);
void SelectWindowWipe(u8 open);
#define GBACOM_BUF_SHOP     ((u8 *)(WORK_TOP - 0x570))
#define GBACOM_BUF_ITEM_BUY ((u8 *)(WORK_TOP - 0x560))
// Shop purchase requests (slot 12), 3 bytes each.
extern const u8 gShopRequests[9];
extern const s16 gEffectBobOffsets[4];
extern const u8 gTingSprite[];
// The definition narrows x and y to u16; callers see s16 (they sign-extend).
void DrawTitleSpriteOnMap(s16 x, s16 y, u8 kind);
void StartEffectDrop(void);
void DrawTimerCount(s16 x, s16 y);
// The definition narrows x and y to u16; callers see s16 (they sign-extend).
extern const u8 gBombSprite[];
extern const u8 gStatusHighlightSprite[];
extern const u8 gStatusFrameSprite[];

#endif // GUARD_GAME_H
