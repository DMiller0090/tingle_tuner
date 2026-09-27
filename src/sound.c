#include "global.h"
#include "sound.h"
#include "gba/syscall.h"

// The sound driver (GZLJ01 02020E28-020237CC): the API the game calls, then the sequencer,
// PCM mixer front end, GB channels and the command queue. See docs/layout.md "Sound driver".

// gSoundTable/gSoundLoopTable entries: bits 0-15 SE index, 16-23 player, then these flags.
#define SOUND_PAN_X          (1 << 24)  // pan and volume from params->x
#define SOUND_LOW_PRIORITY   (1 << 25)
#define SOUND_HIGH_PRIORITY  (1 << 26)
#define SOUND_ADD_ARG        (1 << 27)  // add params->arg to the SE index
#define SOUND_NO_RESTART     (1 << 28)  // ignored while the same ID plays
#define SOUND_VOLUME_Y       (1 << 29)  // volume (also) from params->y

#define SOUND_API_PLAYERS 19  // players 0-18; SoundPlayLoop uses 16 and 17

struct SoundParams
{
    /*0x0*/ s16 x;    // screen position of the source
    /*0x2*/ s16 y;
    /*0x4*/ s16 arg;  // SOUND_ADD_ARG; loop 9's tempo step
};

struct SoundLoopSlot
{
    /*0x0*/ u32 owner;  // -1: free
    /*0x4*/ u8 refreshed;
};

extern const u8 gSoundBankHeader[];
extern const u32 gSoundTable[];
extern const u32 gSoundLoopTable[];

extern s16 gSoundPlayerSoundId[SOUND_API_PLAYERS];  // -1: none
extern u8 gSoundStarted[SOUND_API_PLAYERS];         // started this frame
extern u8 gSoundLoop9Active;
extern u16 gSoundLoop9Player;
extern u8 gSoundLoop9Timer;       // frames since loop 9 stopped, up to 150
extern s16 gSoundLoop9TempoOfs;
extern u8 gSoundLoop9Param;
extern struct SoundLoopSlot gSoundLoopSlots[2];

void SoundDriverInit(const void *bankHeader);
void SoundApiInit(void);
void SoundApiUpdate(void);
u32 PlayerGetState(u32 player);
void SoundCmdFlush(void);
void SoundCmdStartSe(u16 player, u16 set, u16 index);
void SoundCmdFadeOut(u16 player, u16 frames);
void SoundCmdPausePlayers(u32 mask, u8 on);
void SoundCmdFadeOutPlayers(u32 mask, u16 frames);
void SoundCmdSetTempo(u16 player, s16 tempo);
void SoundCmdSetTrackVolume(u16 player, u32 mask, u8 volume);
void SoundCmdSetTrackPan(u16 player, u32 mask, u8 pan);

// Commands (struct SoundCmd.cmd): the high byte selects the handler table (SoundCmdPlayer,
// SoundCmdTracks, SoundCmdPlayers, SoundCmdGlobal), the low byte the command.
#define SOUND_CMD_START_SONG        0x000
#define SOUND_CMD_START_SE          0x001
#define SOUND_CMD_FADE_OUT          0x002
#define SOUND_CMD_PAUSE             0x003
#define SOUND_CMD_SET_TEMPO         0x004
#define SOUND_CMD_SET_VOLUME        0x005
#define SOUND_CMD_SET_FLAGS         0x006
#define SOUND_CMD_MUTE_TRACKS       0x100
#define SOUND_CMD_SET_TRACK_PAN     0x101
#define SOUND_CMD_SET_TRACK_VOLUME  0x102
#define SOUND_CMD_SET_TRACK_BEND    0x103
#define SOUND_CMD_FADE_OUT_PLAYERS  0x200
#define SOUND_CMD_PAUSE_PLAYERS     0x201
#define SOUND_CMD_NOP               0x300
#define SOUND_CMD_CALL              0x301
#define SOUND_CMD_SET_SEQ_HOOK      0x302
#define SOUND_CMD_SET_NOTE_HOOK     0x303
#define SOUND_CMD_SET_PCM_VOICES    0x304

struct SoundCmd
{
    /*0x0*/ u16 cmd;
    /*0x4*/ u32 a;
    /*0x8*/ u32 b;
};

typedef void (*SoundSeqHook)(void *track, u8 arg);  // sequence command CA
typedef void (*SoundNoteHook)(void *voice);         // notes of players with flags bit 0

extern SoundNoteHook gSoundNoteHook;
extern SoundSeqHook gSoundSeqHook;
extern struct SoundCmd gSoundCmdQueue[46];
extern struct SoundCmd *gSoundCmdRead;
extern struct SoundCmd *gSoundCmdWrite;
extern struct SoundCmd *gSoundCmdLimit;  // end of what SoundCmdFlush published
extern struct SoundCmd *gSoundCmdEnd;

void SoundInit(void)
{
    SoundDriverInit(&gSoundBankHeader);
    SoundApiInit();
}

void SoundMain(void)
{
    SoundApiUpdate();
    SoundCmdFlush();
}

void SoundPauseAll(u8 on)
{
    SoundCmdPausePlayers(0xFFFFE, on);
}

void SoundStopAll(void)
{
    SoundCmdFadeOutPlayers(0xFFFFF, 0);
}

void SoundStopMost(void)
{
    SoundCmdFadeOutPlayers(0x7FFFF, 0);
}

void SoundApiInit(void)
{
    s32 i;

    for (i = 0; i < SOUND_API_PLAYERS; i++)
        gSoundStarted[i] = FALSE;
    gSoundLoop9Active = FALSE;
    gSoundLoop9Timer = 150;
    CpuFill16(0xFFFF, gSoundPlayerSoundId, sizeof(gSoundPlayerSoundId));
}

// Stop the loop slots nobody refreshed this frame, count the frames since loop 9 stopped, and
// forget the sound of each player that has finished.
void SoundApiUpdate(void)
{
    s32 i;

    for (i = 0; i < 2; i++)
    {
        if (gSoundLoopSlots[i].refreshed == 0)
        {
            s32 player = i + 16;

            if (PlayerGetState(player) != 0)
                SoundCmdFadeOut(player, 0);
            gSoundLoopSlots[i].owner = -1;
            if (gSoundLoop9Active && player == gSoundLoop9Player)
            {
                gSoundLoop9Active = FALSE;
                gSoundLoop9Timer = 0;
            }
        }
        else
        {
            gSoundLoopSlots[i].refreshed = 0;
        }
    }
    if (!gSoundLoop9Active && gSoundLoop9Timer < 150)
        gSoundLoop9Timer++;
    for (i = 0; i < SOUND_API_PLAYERS; i++)
    {
        if (gSoundStarted[i])
            gSoundStarted[i] = FALSE;
        else if (PlayerGetState(i) == 0)
            gSoundPlayerSoundId[i] = -1;
    }
}

u8 SoundPanFromX(struct SoundParams *params)
{
    if (params == NULL)
        return 0x40;
    if (params->x < -0x150)
        return 0;
    if (params->x < 0x3C)
        return (params->x >> 3) + 0x2A;
    if (params->x < 0xB4)
        return ((params->x - 0x3C) >> 2) + 0x31;
    if (params->x <= 0x233)
        return ((params->x - 0xB4) >> 3) + 0x4F;
    return 0x7F;
}

u16 SoundVolumeFromX(struct SoundParams *params)
{
    if (params == NULL)
        return 0x80;
    if (params->x < -0x80)
        return 0;
    if (params->x < 0)
        return params->x + 0x80;
    if (params->x > 0x170)
        return 0;
    if (params->x > 0xF0)
        return 0x170 - params->x;
    return 0x80;
}

u16 SoundVolumeFromY(struct SoundParams *params)
{
    if (params == NULL)
        return 0x80;
    if (params->y < -0x80)
        return 0;
    if (params->y < 0)
        return params->y + 0x80;
    if (params->y > 0x120)
        return 0;
    if (params->y > 0xA0)
        return 0x120 - params->y;
    return 0x80;
}

#define SOUND_PRIORITY(entry) ((entry) & SOUND_HIGH_PRIORITY ? 2 : !((entry) & SOUND_LOW_PRIORITY))

// Near-miss: identical but for one register copy in the volume product (`volume * y >> 7`
// shifts a copy instead of the product's register) and so 4 bytes longer.
//     void SoundPlay_(u16 id, struct SoundParams *params) asm("SoundPlay");
//     void SoundPlay_(u16 id, struct SoundParams *params)
//     {
//         u8 player = (gSoundTable[id] & 0xFF0000) >> 16;
//         u32 entry = gSoundTable[id];
//         u32 volume = 0x80;
//         u16 index;
//         u32 cur;
//
//         if (gSoundPlayerSoundId[player] != -1)
//         {
//             if ((entry & SOUND_NO_RESTART) && id == gSoundPlayerSoundId[player])
//                 return;
//             cur = gSoundTable[gSoundPlayerSoundId[player]];
//             if (SOUND_PRIORITY(entry) < SOUND_PRIORITY(cur))
//                 return;
//         }
//         index = entry;
//         if (entry & SOUND_ADD_ARG)
//             index += params->arg;
//         SoundCmdStartSe(player, 0, index);
//         gSoundPlayerSoundId[player] = id;
//         gSoundStarted[player] = TRUE;
//         if (entry & SOUND_PAN_X)
//         {
//             SoundCmdSetTrackPan(player, 1, SoundPanFromX(params));
//             volume = SoundVolumeFromX(params);
//         }
//         if (entry & SOUND_VOLUME_Y)
//             volume = volume * SoundVolumeFromY(params) >> 7;
//         SoundCmdSetTrackVolume(player, 1, volume);
//     }
INCLUDE_ASM(SoundPlay);

bool32 SoundIsPlaying(u16 id)
{
    u32 player = (gSoundTable[id] & 0xFF0000) >> 16;
    bool32 playing = FALSE;

    if (gSoundPlayerSoundId[player] == id)
        playing = TRUE;
    return playing;
}

s8 SoundFindLoopSlot(u32 owner)
{
    s8 i;

    for (i = 0; i < 2; i++)
    {
        if (gSoundLoopSlots[i].owner == -1 || gSoundLoopSlots[i].owner == owner)
            return i;
    }
    return -1;
}

u8 SoundLoop9Recent(void)
{
    if (gSoundLoop9Timer < 150)
        return TRUE;
    return FALSE;
}

s16 SoundLoop9Tempo(struct SoundParams *params)
{
    gSoundLoop9Param = params->arg;
    if (params->arg == 0)
        return -30 * params->arg;
    else if (params->arg == 1)
        return -30 * params->arg;
    else if (params->arg == 2)
        return -30 * params->arg;
    else if (params->arg == 3)
        return -30 * params->arg;
}

// Near-miss: the volume and the slot index (spilled) swap stack slots, and the volume
// product multiplies into the other register.
//     void SoundPlayLoop_(u32 owner, u16 id, struct SoundParams *params) asm("SoundPlayLoop");
//     void SoundPlayLoop_(u32 owner, u16 id, struct SoundParams *params)
//     {
//         u32 volume = 0x80;
//         u32 entry = gSoundLoopTable[id];
//         u16 index = *(u16 *)&gSoundLoopTable[id];
//         s8 i = SoundFindLoopSlot(owner);
//
//         if (i >= 0)
//         {
//             u32 player = i + 16;
//
//             if (owner != gSoundLoopSlots[i].owner)
//             {
//                 if (id == 9)
//                 {
//                     gSoundLoop9Player = player;
//                     gSoundLoop9Active = TRUE;
//                     index += SoundLoop9Recent();
//                 }
//                 SoundCmdStartSe(player, 0, index);
//                 gSoundLoopSlots[i].owner = owner;
//             }
//             if (id == 9)
//             {
//                 if (params->arg != gSoundLoop9Param)
//                     gSoundLoop9TempoOfs = SoundLoop9Tempo(params);
//                 SoundCmdSetTempo(player, gSoundLoop9TempoOfs);
//             }
//             gSoundPlayerSoundId[player] = id;
//             if (entry & SOUND_PAN_X)
//             {
//                 SoundCmdSetTrackPan(player, 1, SoundPanFromX(params));
//                 volume = SoundVolumeFromX(params);
//             }
//             if (entry & SOUND_VOLUME_Y)
//                 volume = volume * SoundVolumeFromY(params) >> 7;
//             SoundCmdSetTrackVolume(player, 1, volume);
//             gSoundLoopSlots[i].refreshed = TRUE;
//         }
//     }
INCLUDE_ASM(SoundPlayLoop);

INCLUDE_ASM(SoundDriverInit);

INCLUDE_ASM(SoundVSync);

INCLUDE_ASM(SoundVSyncProc);

INCLUDE_ASM(PcmInit);

INCLUDE_ASM(PcmVSync);

INCLUDE_ASM(SoundDmaStop);

INCLUDE_ASM(SoundDmaStart);

INCLUDE_ASM(PcmMixVoice);

INCLUDE_ASM(DrumInstrumentInit);

INCLUDE_ASM(TrackLookupInstrument);

INCLUDE_ASM(VoicesInit);

INCLUDE_ASM(VoiceListRemove);

INCLUDE_ASM(VoiceListPush);

INCLUDE_ASM(VoiceListInsert);

INCLUDE_ASM(VoiceNoteFreq);

INCLUDE_ASM(NoiseFreq);

INCLUDE_ASM(EnvelopeStep);

INCLUDE_ASM(SoundCmdNop);

INCLUDE_ASM(SetPcmVoiceCount);

INCLUDE_ASM(PcmVoiceVolume);

INCLUDE_ASM(CgbVoiceVolume);

INCLUDE_ASM(VoiceUpdatePitch);

INCLUDE_ASM(PcmMixFrame);

INCLUDE_ASM(CgbUpdate);

INCLUDE_ASM(TrackNoteOn);

INCLUDE_ASM(VoiceNoteOff);

INCLUDE_ASM(VoiceStop);

INCLUDE_ASM(CgbVoiceStart);

INCLUDE_ASM(VoiceAlloc);

INCLUDE_ASM(TracksInit);

INCLUDE_ASM(TrackAlloc);

INCLUDE_ASM(TrackStart);

INCLUDE_ASM(TrackReleaseNotes);

INCLUDE_ASM(TrackStop);

INCLUDE_ASM(TrackExecute);

INCLUDE_ASM(TrackAddVoice);

INCLUDE_ASM(TrackRemoveVoice);

INCLUDE_ASM(TrackReadVarLen);

INCLUDE_ASM(TrackSetBank);

INCLUDE_ASM(PlayersInit);

INCLUDE_ASM(PlayerReset);

INCLUDE_ASM(PlayerUpdateAll);

INCLUDE_ASM(PlayerStartSong);

INCLUDE_ASM(PlayerStartSe);

INCLUDE_ASM(PlayerStartSongData);

INCLUDE_ASM(PlayerStartSeData);

INCLUDE_ASM(PlayerStop);

INCLUDE_ASM(PlayerFadeOut);

INCLUDE_ASM(PlayerSetPause);

INCLUDE_ASM(PlayerGetState);

// The command queue: the API writes commands at gSoundCmdWrite, SoundCmdFlush (from
// SoundMain) publishes them up to there, and SoundCmdProcess (in VBlank) reads them.
void SoundCmdPush(void)
{
    if (++gSoundCmdWrite == gSoundCmdEnd)
        gSoundCmdWrite = gSoundCmdQueue;
}

void SoundCmdQueueInit(void)
{
    gSoundCmdRead = gSoundCmdQueue;
    gSoundCmdWrite = gSoundCmdQueue;
    gSoundCmdLimit = gSoundCmdQueue;
    gSoundCmdEnd = gSoundCmdQueue + ARRAY_COUNT(gSoundCmdQueue);
    gSoundNoteHook = NULL;
    gSoundSeqHook = NULL;
}

struct SoundCmd *SoundCmdPop(void)
{
    struct SoundCmd *cmd = gSoundCmdRead;

    if (cmd == gSoundCmdLimit)
        return NULL;
    if (++gSoundCmdRead == gSoundCmdEnd)
        gSoundCmdRead = gSoundCmdQueue;
    return cmd;
}

void SoundCmdFlush(void)
{
    gSoundCmdLimit = gSoundCmdWrite;
}

void SoundCmdStartSong(u16 player, u16 song)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_START_SONG;
    cmd->a = player;
    cmd->b = song;
    SoundCmdPush();
}

void SoundCmdStartSe(u16 player, u16 set, u16 index)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_START_SE;
    cmd->a = (player << 16) | set;
    cmd->b = index;
    SoundCmdPush();
}

void SoundCmdFadeOut(u16 player, u16 frames)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_FADE_OUT;
    cmd->a = player;
    cmd->b = frames;
    SoundCmdPush();
}

void SoundCmdPause(u16 player, u8 on)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_PAUSE;
    cmd->a = player;
    cmd->b = on;
    SoundCmdPush();
}

void SoundCmdFadeOutPlayers(u32 mask, u16 frames)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_FADE_OUT_PLAYERS;
    cmd->a = frames;
    cmd->b = mask;
    SoundCmdPush();
}

void SoundCmdPausePlayers(u32 mask, u8 on)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_PAUSE_PLAYERS;
    cmd->a = on;
    cmd->b = mask;
    SoundCmdPush();
}

void SoundCmdSetTempo(u16 player, s16 tempo)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_TEMPO;
    cmd->a = player;
    cmd->b = tempo;
    SoundCmdPush();
}

void SoundCmdSetVolume(u16 player, u8 volume)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_VOLUME;
    cmd->a = player;
    cmd->b = volume;
    SoundCmdPush();
}

void SoundCmdSetFlags(u16 player, u8 flags)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_FLAGS;
    cmd->a = player;
    cmd->b = flags;
    SoundCmdPush();
}

void SoundCmdMuteTracks(u16 player, u32 mask, u8 on)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_MUTE_TRACKS;
    cmd->a = (player << 16) | on;
    cmd->b = mask;
    SoundCmdPush();
}

void SoundCmdSetTrackVolume(u16 player, u32 mask, u8 volume)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_TRACK_VOLUME;
    cmd->a = (player << 16) | volume;
    cmd->b = mask;
    SoundCmdPush();
}

// Near-miss: the original shifts player first (`lsls r0, r0, #0x10` before range's
// `lsls r2, r2, #0x18`); every parameter type and operand order tried leaves that one swap.
//     void SoundCmdSetTrackBend(u16 player, u32 mask, u8 range, s8 bend)
//     {
//         struct SoundCmd *cmd = gSoundCmdWrite;
//
//         cmd->cmd = SOUND_CMD_SET_TRACK_BEND;
//         cmd->a = (range << 8) | (player << 16) | (u8)bend;
//         cmd->b = mask;
//         SoundCmdPush();
//     }
INCLUDE_ASM(SoundCmdSetTrackBend);

void SoundCmdSetTrackPan(u16 player, u32 mask, u8 pan)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_TRACK_PAN;
    cmd->a = (player << 16) | pan;
    cmd->b = mask;
    SoundCmdPush();
}

void SoundCmdNop300(u8 arg)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_NOP;
    cmd->a = arg;
    SoundCmdPush();
}

void SoundCmdSetPcmVoices(u8 count)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_PCM_VOICES;
    cmd->a = count;
    SoundCmdPush();
}

void SoundCmdCall(void (*func)(u32 arg), u32 arg)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_CALL;
    cmd->a = (u32)func;
    cmd->b = arg;
    SoundCmdPush();
}

void SoundCmdSetSeqHook(SoundSeqHook hook)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_SEQ_HOOK;
    cmd->a = (u32)hook;
    SoundCmdPush();
}

void SoundCmdSetNoteHook(SoundNoteHook hook)
{
    struct SoundCmd *cmd = gSoundCmdWrite;

    cmd->cmd = SOUND_CMD_SET_NOTE_HOOK;
    cmd->a = (u32)hook;
    SoundCmdPush();
}

INCLUDE_ASM(SoundCmdPlayer);

INCLUDE_ASM(SoundCmdTracks);

INCLUDE_ASM(SoundCmdPlayers);

INCLUDE_ASM(SoundCmdGlobal);

INCLUDE_ASM(SoundCmdProcess);
