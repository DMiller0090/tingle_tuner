#include "global.h"
#include "sound.h"

// The sound driver (GZLJ01 02020E28-020237CC): the API the game calls, then the sequencer,
// PCM mixer front end, GB channels and the command queue. See docs/layout.md "Sound driver".

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

INCLUDE_ASM(SoundInit);

INCLUDE_ASM(SoundMain);

INCLUDE_ASM(SoundPauseAll);

INCLUDE_ASM(SoundStopAll);

INCLUDE_ASM(SoundStopMost);

INCLUDE_ASM(SoundApiInit);

INCLUDE_ASM(SoundApiUpdate);

INCLUDE_ASM(SoundPanFromX);

INCLUDE_ASM(SoundVolumeFromX);

INCLUDE_ASM(SoundVolumeFromY);

INCLUDE_ASM(SoundPlay);

INCLUDE_ASM(SoundIsPlaying);

INCLUDE_ASM(SoundFindLoopSlot);

INCLUDE_ASM(SoundLoop9Recent);

INCLUDE_ASM(SoundLoop9Tempo);

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
