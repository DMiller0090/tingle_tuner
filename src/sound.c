#include "global.h"
#include "sound.h"
#include "gba/dma.h"
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


extern s16 gSoundPlayerSoundId[SOUND_API_PLAYERS];  // -1: none
extern u8 gSoundStarted[SOUND_API_PLAYERS];         // started this frame
extern u8 gSoundLoop9Active;
extern u16 gSoundLoop9Player;
extern u8 gSoundLoop9Timer;       // frames since loop 9 stopped, up to 150
extern s16 gSoundLoop9TempoOfs;
extern u8 gSoundLoop9Param;
extern struct SoundLoopSlot gSoundLoopSlots[2];

struct SoundBankHeader;
void SoundDriverInit(const struct SoundBankHeader *header);
void SoundApiInit(void);
void SoundApiUpdate(void);
u32 PlayerGetState(u32 player);
struct SoundCmd *SoundCmdPop(void);
void SoundCmdPlayer(struct SoundCmd *cmd);
void SoundCmdTracks(struct SoundCmd *cmd);
void SoundCmdPlayers(struct SoundCmd *cmd);
void SoundCmdGlobal(struct SoundCmd *cmd);
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
typedef void (*SoundNoteHook)(void *track, u8 key, u8 velocity, u32 length);  // notes of players with flags bit 0

extern SoundNoteHook gSoundNoteHook;
extern SoundSeqHook gSoundSeqHook;
extern struct SoundCmd gSoundCmdQueue[46];
extern struct SoundCmd *gSoundCmdRead;
extern struct SoundCmd *gSoundCmdWrite;
extern struct SoundCmd *gSoundCmdLimit;  // end of what SoundCmdFlush published
extern struct SoundCmd *gSoundCmdEnd;

struct SoundTrack;
struct SoundVoice;
struct SoundBankHeader;

// A sequence player (gSoundPlayers, 20): an SE plays on one track, a song on up to ten.
struct SoundPlayer
{
    /*0x00*/ const u16 *bankMap;      // sequence bank -> global bank
    /*0x04*/ const u16 *seqOffsets;   // the SE set's (or song's) u16 offsets from here
    /*0x08*/ struct SoundTrack *tracks[10];
    /*0x30*/ u16 tempo;               // subtracted from the tracks' waits each frame
    /*0x32*/ s16 tempoOfs;
    /*0x34*/ u16 fadeVolume;          // 0x8000 = full
    /*0x36*/ s16 fadeStep;
    /*0x38*/ u16 fadeTarget;
    /*0x3A*/ u16 fadeFrames;
    /*0x3C*/ u8 paused:1;
    /*0x3D*/ u8 unk3D[3];
    /*0x40*/ u8 unk40;
    /*0x41*/ u8 volume;
    /*0x42*/ u8 state;                // 0 idle, 1 playing, 2 fading out
    /*0x43*/ u8 isSe;
    /*0x44*/ u8 flags;                // bit 0: notes go to gSoundNoteHook
};

// Sequence commands E5-E7; voices point at their track's.
struct TrackVibrato
{
    /*0x0*/ u16 delay;
    /*0x4*/ u32 speed;
    /*0x8*/ u32 depth;
};

// A sequence track (gSoundTracks, 24), see docs/layout.md for the byte code.
struct SoundTrack
{
    /*0x00*/ const u8 *pc;
    /*0x04*/ const u32 *sampleSet;
    /*0x08*/ struct SoundPlayer *player;  // NULL: free
    /*0x0C*/ struct SoundVoice *voices;
    /*0x10*/ struct TrackVibrato vibrato;
    /*0x1C*/ u8 sweep1C;
    /*0x1D*/ u8 sweep1D;
    /*0x1E*/ u8 sweep1E;
    /*0x20*/ u16 sweep20;
    /*0x22*/ u16 sweep22;
    /*0x24*/ const u8 *stack[3];         // F4 call return addresses
    /*0x30*/ const u8 **sp;
    /*0x34*/ s32 wait;
    /*0x38*/ u8 unk38[8];
    /*0x40*/ u16 bank;
    /*0x42*/ u16 program;
    /*0x44*/ u16 lastLength;
    /*0x46*/ u16 lastWait;
    /*0x48*/ u8 velocity;
    /*0x49*/ u8 tie;
    /*0x4A*/ u8 mute;
    /*0x4B*/ u8 pan;
    /*0x4C*/ u8 unk4C;                   // sequence command E3
    /*0x4D*/ u8 volume;                  // sequence command E0
    /*0x4E*/ u8 cmdVolume;               // SoundCmdSetTrackVolume
    /*0x4F*/ s8 bend;
    /*0x50*/ u8 bendRange;
    /*0x51*/ s8 transpose;
    /*0x52*/ u8 priority;
    /*0x53*/ u8 notesWait;
};

struct EnvelopeStep;

struct VoiceVibrato
{
    /*0x0*/ u32 phase;  // gVibratoTable index * 2
    /*0x4*/ u32 delay;
    /*0x8*/ const struct TrackVibrato *params;
};

// Pitch sweep (sequence commands D0-DF): offset moves by step to target after delay.
struct VoiceSweep
{
    /*0x00*/ u32 delay;
    /*0x04*/ u32 frames;
    /*0x08*/ s32 offset;
    /*0x0C*/ u32 target;
    /*0x10*/ s32 step;
};

struct VoiceEnvelope
{
    /*0x00*/ u32 level;
    /*0x04*/ s32 target;
    /*0x08*/ u16 frames;  // left in this step
    /*0x0A*/ s16 step;    // added to level per frame
    /*0x0C*/ const struct EnvelopeStep *steps;
    /*0x10*/ s8 index;
};

// A voice (gPcmVoices 7, gCgbVoices 4).
struct SoundVoice
{
    /*0x00*/ u8 type;   // 0 PCM, 1-4 GB channel
    /*0x01*/ u8 state;  // 0 free, 1 on, 2 released
    /*0x04*/ struct SoundTrack *track;
    /*0x08*/ u8 priority;
    /*0x09*/ u8 key;
    /*0x0A*/ u8 velocity;
    /*0x0B*/ u8 unk0B;
    /*0x0C*/ u32 baseFreq;
    /*0x10*/ union { u32 pcm; u16 cgb; } freq;  // PCM: 16.16 step; GB: the frequency register
    /*0x14*/ u32 volume;   // GB channels: 0-15
    /*0x18*/ u16 gate;     // frames until the note is released
    /*0x1A*/ u8 unk1A;     // the track's E3 value
    /*0x1B*/ u8 fixedPan;  // the instrument's key split sets pan
    /*0x1C*/ u8 pan;
    /*0x1D*/ u8 unk1D[3];
    /*0x20*/ struct VoiceVibrato vibrato;
    /*0x2C*/ struct VoiceSweep sweep;
    /*0x40*/ struct VoiceEnvelope env;
    /*0x54*/ const struct SoundInstrument *inst;
    /*0x58*/ u8 release;  // released: volume * (0xE6 + release) / 512 per frame
    /*0x59*/ u8 unk59[3];
    /*0x5C*/ const struct SoundSample *sample;
    /*0x60*/ u32 pos;  // PCM: sample position; GB: frames since the note started
    /*0x64*/ union { const u8 *ptr; u8 duty; } cgb;  // wave data or duty list (u16 count, then the duties), else the duty
    /*0x68*/ struct SoundVoice *prev;  // in gPcmActiveVoices / gPcmFreeVoices
    /*0x6C*/ struct SoundVoice *next;
    /*0x70*/ struct SoundVoice *trackPrev;
    /*0x74*/ struct SoundVoice *trackNext;
};

// VoicesInit and TracksInit write the pointers below a byte at a time: they went through
// packed (1-byte aligned) views of the structs.
struct PackedVoice
{
    /*0x00*/ u8 type;
    /*0x01*/ u8 state;
    /*0x02*/ u8 unk02[2];
    /*0x04*/ struct SoundTrack *track;
    /*0x08*/ u8 unk08[0x60];
    /*0x68*/ struct PackedVoice *prev;
    /*0x6C*/ struct PackedVoice *next;
    /*0x70*/ u8 unk70[8];
} __attribute__((packed));

struct PackedTrack
{
    /*0x00*/ u8 unk00[8];
    /*0x08*/ struct SoundPlayer *player;
    /*0x0C*/ struct SoundVoice *voices;
    /*0x10*/ u8 unk10[0x44];
} __attribute__((packed));

#define PACKED_PCM_VOICES ((struct PackedVoice *)gPcmVoices)
#define PACKED_CGB_VOICES ((struct PackedVoice *)gCgbVoices)

// A sample in the bank: s8 data at the header's rate.
struct SoundSample
{
    /*0x0*/ u32 length;
    /*0x4*/ u32 rate;       // Hz; the instrument's base key plays at it
    /*0x8*/ u32 loopStart;
    /*0xC*/ u32 loopEnd;    // 0: one-shot
    /*0x10*/ s8 data[1];
};

struct SoundInstrument;

// What TrackLookupInstrument resolves a note to.
struct InstLookup
{
    /*0x00*/ const struct SoundInstrument *inst;
    /*0x04*/ const struct EnvelopeStep *envelope;
    /*0x08*/ const u8 *dutyList;  // GB square channels: per-frame duties (flags bit 0)
    /*0x0C*/ const u8 *wave;      // GB wave channel
    /*0x10*/ u8 pan;
    /*0x11*/ u8 fixedPan;
    /*0x12*/ u8 isDrum;
};

// An instrument record in a bank (8 bytes), see docs/layout.md "Raw tail block".
struct SoundInstrument
{
    /*0x0*/ u8 type;      // 0 PCM, 1-4 GB channel, 0x10-0x12 key split, drum kit, key ranges
    /*0x1*/ u8 flags;
    /*0x2*/ u16 arg;
    /*0x4*/ u16 envelope;
    /*0x6*/ u8 release;
    /*0x7*/ u8 baseKey;
};

#define REG_SOUND1CNT_L   (*(vu16 *)0x04000060)
#define REG_NR10          (*(vu8 *)0x04000060)
#define REG_SOUND1CNT_H   (*(vu16 *)0x04000062)
#define REG_NR11          (*(vu8 *)0x04000062)
#define REG_NR12          (*(vu8 *)0x04000063)
#define REG_SOUND1CNT_X   (*(vu16 *)0x04000064)
#define REG_NR14          (*(vu8 *)0x04000065)
#define REG_NR21          (*(vu8 *)0x04000068)
#define REG_NR22          (*(vu8 *)0x04000069)
#define REG_SOUND2CNT_H   (*(vu16 *)0x0400006C)
#define REG_NR24          (*(vu8 *)0x0400006D)
#define REG_NR30          (*(vu8 *)0x04000070)
#define REG_NR31          (*(vu8 *)0x04000072)
#define REG_NR32          (*(vu8 *)0x04000073)
#define REG_SOUND3CNT_X   (*(vu16 *)0x04000074)
#define REG_NR41          (*(vu8 *)0x04000078)
#define REG_NR42          (*(vu8 *)0x04000079)
#define REG_NR43          (*(vu8 *)0x0400007C)
#define REG_NR44          (*(vu8 *)0x0400007D)
#define REG_NR51          (*(vu8 *)0x04000081)
#define REG_SOUNDCNT_L    (*(vu16 *)0x04000080)
#define REG_SOUNDCNT_H_LO (*(vu8 *)0x04000082)
#define REG_SOUNDCNT_H_HI (*(vu8 *)0x04000083)
#define REG_SOUNDCNT_X    (*(vu8 *)0x04000084)
#define REG_SOUNDBIAS     (*(vu16 *)0x04000088)
#define REG_WAVE_RAM      ((vu32 *)0x04000090)
#define REG_FIFO_A        (*(vu32 *)0x040000A0)
#define REG_FIFO_B        (*(vu32 *)0x040000A4)
#define REG_TM0CNT        (*(vu32 *)0x04000100)

#define PCM_SAMPLE_RATE 10512
#define PCM_FRAME_SAMPLES 176  // per channel and frame

extern u8 gPcmEnabled;
extern struct SoundInstrument gDrumInstrument;
extern struct SoundVoice gPcmVoices[7];
extern struct SoundVoice gCgbVoices[4];
extern s16 gPcmMixBuffer[PCM_FRAME_SAMPLES * 2];

typedef void (*PcmClipFunc)(s16 *mix, s8 *dest, s16 *mixEnd);
// Returns the new position.
typedef u32 (*PcmMixFunc)(s8 *data, s16 *mixL, s16 *mixR, s16 *mixEnd, u32 pos, u32 step,
                          u32 volL, u32 volR);
extern struct SoundVoice *gPcmActiveVoices;
extern struct SoundVoice *gPcmFreeVoices;
extern struct SoundVoice *gPcmDisabledVoices;
extern u8 gPcmVoiceCount;
extern const u8 *gCgbWaveLoaded;  // the wave data in wave RAM
extern s8 *gPcmBufA[2];
extern s8 *gPcmBufB[2];
extern u16 gPcmTimerReload;
extern u8 gPcmBufIndex;
extern u32 gPcmCode[];
extern void *gPcmClipFunc;
extern void *gPcmMixFunc;
extern void *gPcmEchoFunc;
extern s8 gPcmDmaBuffers[];

// ARM, run from the IWRAM copy in gPcmCode.
void PcmClipS16ToS8(void);
void PcmMixChannel(void);
void PcmEchoMix(void);

void PcmInit(s8 *buffers);
struct SoundVoice *VoiceAlloc(u8 type, u8 priority);
u32 EnvelopeStep(struct SoundVoice *voice);
u32 VoiceUpdatePitch(struct SoundVoice *voice);
u8 PcmMixVoice(struct SoundVoice *voice, u32 volume, u32 step, u8 pan);
void VoiceListRemove(struct SoundVoice *voice, struct SoundVoice **list);
void VoiceListPush(struct SoundVoice *voice, struct SoundVoice **list);
void VoiceListInsert(struct SoundVoice *voice);
void VoiceStop(struct SoundVoice *voice);
void TrackRemoveVoice(struct SoundTrack *track, struct SoundVoice *voice);
void PcmVSync(void);
void PcmMixFrame(void);
void CgbUpdate(void);
void SoundCmdQueueInit(void);
void SoundCmdProcess(void);
void DrumInstrumentInit(void);
void VoicesInit(void);
void TracksInit(void);
void PlayersInit(void);
void PlayerUpdateAll(void);

extern struct SoundPlayer gSoundPlayers[20];
extern struct SoundTrack gSoundTracks[24];
extern const struct SoundBankHeader *gSoundBank;

void TrackStop(struct SoundTrack *track);
void TrackSetBank(struct SoundTrack *track, u32 bank);
u8 TrackExecute(struct SoundTrack *track);
void TrackNoteOn(struct SoundTrack *track, u8 key, u8 velocity, u32 length);
u32 TrackReadVarLen(struct SoundTrack *track);
void VoiceNoteOff(struct SoundVoice *voice);
void PlayerStop(u32 index);
void PlayerStartSongData(u32 index, const void *song, const void *bankMap);
void PlayerStartSeData(u32 index, const void *set, const void *bankMap, u32 se);
void SoundCmdNop(u8 arg);
void SetPcmVoiceCount(u8 count);

// The sound bank's tables (offsets from each table's start), see docs/layout.md.
struct SoundBankHeader
{
    const u32 *sampleSets;
    const u32 *instBanks;
    const u32 *songs;  // empty: the bank has no songs
    const u32 *seSets;
    const u16 *bankToSampleSet;
    const u32 *songBankMaps;
    const u32 *seBankMaps;
};

// Entry i of an offset table: the table's address plus the offset stored there.
#define SOUND_OFFSET(table, i) ((const void *)((const u8 *)(table) + (table)[i]))

// One step of a volume envelope: ramp linearly to level over frames (-1: hold).
struct EnvelopeStep
{
    s16 frames;
    s16 level;
};

extern const u32 gSoundBankSeBankMaps[];
extern const u32 gSoundBankSeSets[];
extern const u32 gSoundBankInstBanks[];
extern const u16 gSoundBankBankToSampleSet[];
extern const u32 gSoundBankSongBankMaps[];
extern const u32 gSoundBankSampleSets[];
extern const u32 gSoundBankEnd[];

const struct SoundBankHeader gSoundBankHeader =
{
    gSoundBankSampleSets,
    gSoundBankInstBanks,
    gSoundBankEnd,
    gSoundBankSeSets,
    gSoundBankBankToSampleSet,
    gSoundBankSongBankMaps,
    gSoundBankSeBankMaps
};

// The US bank drops SE sequence 7 (voice clip 6), so the SE indices from 8 on are one lower.
#define SOUND_SE(se) ((se) < 8 ? (se) : (se) - SOUND_ID_SHIFT)
#define SOUND(se, player, flags) (SOUND_SE(se) | ((player) << 16) | (flags))

const u32 gSoundTable[] =
{
    SOUND(0x02, 4, 0),
    SOUND(0x03, 4, 0),
    SOUND(0x04, 4, 0),
    SOUND(0x05, 4, 0),
    SOUND(0x08, 4, 0),
    SOUND(0x06, 4, 0),
#ifdef VERSION_GZLJ01
    SOUND(0x07, 4, 0),  // voice clip 6, not in the US sound bank
#endif
    SOUND(0x14, 0, 0),
    SOUND(0x17, 0, 0),
    SOUND(0x18, 0, 0),
    SOUND(0x1A, 0, 0),
    SOUND(0x19, 0, 0),
    SOUND(0x1B, 0, 0),
    SOUND(0x1C, 0, 0),
    SOUND(0x1D, 0, 0),
    SOUND(0x1E, 0, 0),
    SOUND(0x1F, 0, 0),
    SOUND(0x15, 0, SOUND_HIGH_PRIORITY),
    SOUND(0x16, 0, SOUND_HIGH_PRIORITY),
    SOUND(0x22, 0, SOUND_LOW_PRIORITY | SOUND_ADD_ARG),
    SOUND(0x2A, 0, 0),
    SOUND(0x2B, 0, 0),
    SOUND(0x20, 0, 0),
    SOUND(0x21, 0, 0),
    SOUND(0x0A, 0, 0),
    SOUND(0x0D, 0, 0),
    SOUND(0x34, 0, 0),
    SOUND(0x35, 0, 0),
    SOUND(0x36, 0, 0),
    SOUND(0x37, 0, 0),
    SOUND(0x38, 0, 0),
    SOUND(0x30, 0, 0),
    SOUND(0x29, 1, 0),
    SOUND(0x28, 1, 0),
    SOUND(0x27, 1, 0),
    SOUND(0x2F, 1, 0),
    SOUND(0x11, 1, 0),
    SOUND(0x0E, 1, 0),
    SOUND(0x09, 2, 0),
    SOUND(0x2E, 2, 0),
    SOUND(0x0B, 2, SOUND_LOW_PRIORITY),
    SOUND(0x33, 2, 0),
    SOUND(0x32, 3, 0),
    SOUND(0x31, 3, 0)
};

const u32 gSoundLoopTable[11] =
{
    SOUND(0x0C, 0, 0),
    SOUND(0x0F, 0, 0),
    SOUND(0x10, 0, 0),
    SOUND(0x12, 0, 0),
    SOUND(0x13, 0, 0),
    SOUND(0x25, 0, 0),
    SOUND(0x26, 0, 0),
    SOUND(0x2C, 0, 0),
    SOUND(0x2D, 0, 0),
    SOUND(0x00, 0, 0),
    SOUND(0x00, 0, 0)
};

const u8 gInstTypeToVoicePool[5] =
{
    0, 1, 2, 3, 4
};

const struct EnvelopeStep gDefaultEnvelope[3] =
{
    {1, 0x7FFF},
    {0x7FFF, 0x7FFF},
    {-1, 0}
};

const u8 gCgbWaveVolumes[5] =
{
    0x0, 0x60, 0x40, 0x80, 0x20
};

void (*const gSoundCmdHandlers[4])(struct SoundCmd *cmd) =
{
    SoundCmdPlayer,
    SoundCmdTracks,
    SoundCmdPlayers,
    SoundCmdGlobal
};

const u16 gCgbFreqTable[120] =
{
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x2C, 0x9D, 0x107, 0x16B, 0x1C9, 0x223, 0x277, 0x2C7, 0x312, 0x358, 0x39B, 0x3DA,
    0x416, 0x44E, 0x483, 0x4B5, 0x4E5, 0x511, 0x53B, 0x563, 0x589, 0x5AC, 0x5CE, 0x5ED,
    0x60B, 0x627, 0x642, 0x65B, 0x672, 0x689, 0x69E, 0x6B2, 0x6C4, 0x6D6, 0x6E7, 0x6F7,
    0x706, 0x714, 0x721, 0x72D, 0x739, 0x744, 0x74F, 0x759, 0x762, 0x76B, 0x773, 0x77B,
    0x783, 0x78A, 0x790, 0x797, 0x79D, 0x7A2, 0x7A7, 0x7AC, 0x7B1, 0x7B6, 0x7BA, 0x7BE,
    0x7C1, 0x7C5, 0x7C8, 0x7CB, 0x7CE, 0x7D1, 0x7D4, 0x7D6, 0x7D9, 0x7DB, 0x7DD, 0x7DF,
    0x7E1, 0x7E2, 0x7E4, 0x7E6, 0x7E7, 0x7E9, 0x7EA, 0x7EB, 0x7EC, 0x7ED, 0x7EE, 0x7EF,
    0x7F0, 0x7F1, 0x7F2, 0x7F3, 0x7F4, 0x7F4, 0x7F5, 0x7F6, 0x7F6, 0x7F7, 0x7F7, 0x7F8
};

const u8 gNoiseFreqTable[120] =
{
    0xD4, 0xC7, 0xC6, 0xC5, 0xC4, 0xB7, 0xB6, 0xB5, 0xB4, 0xA7, 0xA6, 0xA5,
    0xA4, 0x97, 0x96, 0x95, 0x94, 0x87, 0x86, 0x85, 0x84, 0x77, 0x76, 0x75,
    0x74, 0x67, 0x66, 0x65, 0x64, 0x57, 0x56, 0x55, 0x54, 0x47, 0x46, 0x45,
    0x44, 0x37, 0x36, 0x35, 0x34, 0x27, 0x26, 0x25, 0x24, 0x17, 0x16, 0x15,
    0x14, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0
};

const u32 gPcmPitchTable[120] =
{
    0x800, 0x879, 0x8FA, 0x983, 0xA14, 0xAAD, 0xB50, 0xBFC,
    0xCB2, 0xD74, 0xE41, 0xF1A, 0x1000, 0x10F3, 0x11F5, 0x1306,
    0x1428, 0x155B, 0x16A0, 0x17F9, 0x1965, 0x1AE8, 0x1C82, 0x1E34,
    0x2000, 0x21E7, 0x23EB, 0x260D, 0x2851, 0x2AB7, 0x2D41, 0x2FF2,
    0x32CB, 0x35D1, 0x3904, 0x3C68, 0x4000, 0x43CE, 0x47D6, 0x4C1B,
    0x50A2, 0x556E, 0x5A82, 0x5FE4, 0x6597, 0x6BA2, 0x7208, 0x78D0,
    0x8000, 0x879C, 0x8FAC, 0x9837, 0xA145, 0xAADC, 0xB504, 0xBFC8,
    0xCB2F, 0xD744, 0xE411, 0xF1A1, 0x10000, 0x10F38, 0x11F59, 0x1306F,
    0x1428A, 0x155B8, 0x16A09, 0x17F91, 0x1965F, 0x1AE89, 0x1C823, 0x1E343,
    0x20000, 0x21E71, 0x23EB3, 0x260DF, 0x28514, 0x2AB70, 0x2D413, 0x2FF22,
    0x32CBF, 0x35D13, 0x39047, 0x3C686, 0x40000, 0x43CE3, 0x47D66, 0x4C1BF,
    0x50A28, 0x556E0, 0x5A827, 0x5FE44, 0x6597F, 0x6BA27, 0x7208F, 0x78D0D,
    0x80000, 0x879C7, 0x8FACD, 0x9837F, 0xA1451, 0xAADC0, 0xB504F, 0xBFC88,
    0xCB2FF, 0xD744F, 0xE411F, 0xF1A1B, 0x100000, 0x10F38F, 0x11F59A, 0x1306FE,
    0x1428A2, 0x155B81, 0x16A09E, 0x17F910, 0x1965FE, 0x1AE89F, 0x1C823E, 0x1E3437
};

const s8 gVibratoTable[256] =
{
    0, 3, 6, 9, 12, 15, 18, 21, 24, 28, 31, 34, 37, 40, 43, 46,
    48, 51, 54, 57, 60, 63, 65, 68, 71, 73, 76, 78, 81, 83, 85, 88,
    90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 109, 111, 112, 114, 115, 117,
    118, 119, 120, 121, 122, 123, 124, 124, 125, 126, 126, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 126, 126, 125, 124, 124, 123, 122, 121, 120, 119,
    118, 117, 115, 114, 112, 111, 109, 108, 106, 104, 102, 100, 98, 96, 94, 92,
    90, 88, 85, 83, 81, 78, 76, 73, 71, 68, 65, 63, 60, 57, 54, 51,
    48, 46, 43, 40, 37, 34, 31, 28, 24, 21, 18, 15, 12, 9, 6, 3,
    0, -3, -6, -9, -12, -15, -18, -21, -24, -28, -31, -34, -37, -40, -43, -46,
    -48, -51, -54, -57, -60, -63, -65, -68, -71, -73, -76, -78, -81, -83, -85, -88,
    -90, -92, -94, -96, -98, -100, -102, -104, -106, -108, -109, -111, -112, -114, -115, -117,
    -118, -119, -120, -121, -122, -123, -124, -124, -125, -126, -126, -127, -127, -127, -127, -127,
    -127, -127, -127, -127, -127, -127, -126, -126, -125, -124, -124, -123, -122, -121, -120, -119,
    -118, -117, -115, -114, -112, -111, -109, -108, -106, -104, -102, -100, -98, -96, -94, -92,
    -90, -88, -85, -83, -81, -78, -76, -73, -71, -68, -65, -63, -60, -57, -54, -51,
    -49, -46, -43, -40, -37, -34, -31, -28, -24, -21, -18, -15, -12, -9, -6, -3
};

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

// The volume product and its shift are separate statements (one expression shifts a copy).
void SoundPlay_(u16 id, struct SoundParams *params) asm("SoundPlay");
void SoundPlay_(u16 id, struct SoundParams *params)
{
    u8 player = (gSoundTable[id] & 0xFF0000) >> 16;
    u32 entry = gSoundTable[id];
    u32 volume = 0x80;
    u16 index;
    u32 cur;

    if (gSoundPlayerSoundId[player] != -1)
    {
        if ((entry & SOUND_NO_RESTART) && id == gSoundPlayerSoundId[player])
            return;
        cur = gSoundTable[gSoundPlayerSoundId[player]];
        if (SOUND_PRIORITY(entry) < SOUND_PRIORITY(cur))
            return;
    }
    index = entry;
    if (entry & SOUND_ADD_ARG)
        index += params->arg;
    SoundCmdStartSe(player, 0, index);
    gSoundPlayerSoundId[player] = id;
    gSoundStarted[player] = TRUE;
    if (entry & SOUND_PAN_X)
    {
        SoundCmdSetTrackPan(player, 1, SoundPanFromX(params));
        volume = SoundVolumeFromX(params);
    }
    if (entry & SOUND_VOLUME_Y)
    {
        volume = volume * SoundVolumeFromY(params);
        volume >>= 7;
    }
    SoundCmdSetTrackVolume(player, 1, volume);
}

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

// i is declared first so it gets the first stack slot; the volume product and its shift are
// separate statements (one expression shifts a copy).
void SoundPlayLoop_(u32 owner, u16 id, struct SoundParams *params) asm("SoundPlayLoop");
void SoundPlayLoop_(u32 owner, u16 id, struct SoundParams *params)
{
    s8 i;
    u32 volume = 0x80;
    u32 entry = gSoundLoopTable[id];
    u16 index = *(u16 *)&gSoundLoopTable[id];

    i = SoundFindLoopSlot(owner);

    if (i >= 0)
    {
        u32 player = i + 16;

        if (owner != gSoundLoopSlots[i].owner)
        {
            if (id == 9)
            {
                gSoundLoop9Player = player;
                gSoundLoop9Active = TRUE;
                index += SoundLoop9Recent();
            }
            SoundCmdStartSe(player, 0, index);
            gSoundLoopSlots[i].owner = owner;
        }
        if (id == 9)
        {
            if (params->arg != gSoundLoop9Param)
                gSoundLoop9TempoOfs = SoundLoop9Tempo(params);
            SoundCmdSetTempo(player, gSoundLoop9TempoOfs);
        }
        gSoundPlayerSoundId[player] = id;
        if (entry & SOUND_PAN_X)
        {
            SoundCmdSetTrackPan(player, 1, SoundPanFromX(params));
            volume = SoundVolumeFromX(params);
        }
        if (entry & SOUND_VOLUME_Y)
        {
            volume = volume * SoundVolumeFromY(params);
            volume >>= 7;
        }
        SoundCmdSetTrackVolume(player, 1, volume);
        gSoundLoopSlots[i].refreshed = TRUE;
    }
}

void SoundDriverInit(const struct SoundBankHeader *header)
{
    gSoundBank = header;
    REG_SOUNDCNT_X = 0;
    REG_SOUNDCNT_X = 0x80;
    REG_SOUNDCNT_L = 0xFF77;
    REG_SOUNDCNT_H_LO = 0x0D;
    REG_SOUNDBIAS = (REG_SOUNDBIAS & 0x3FFF) | 0x4000;
    REG_SOUND1CNT_L = 8;
    REG_SOUND1CNT_H = 0xF000;
    CpuFastSet(PcmClipS16ToS8, gPcmCode, 0xD8);
    gPcmClipFunc = (void *)gPcmCode;
    gPcmMixFunc = (void *)((u32)PcmMixChannel - (u32)PcmClipS16ToS8 + (u32)gPcmCode);
    gPcmEchoFunc = (void *)((u32)PcmEchoMix - (u32)PcmClipS16ToS8 + (u32)gPcmCode);
    PcmInit(gPcmDmaBuffers);
    SoundCmdQueueInit();
    DrumInstrumentInit();
    VoicesInit();
    TracksInit();
    PlayersInit();
}

void SoundVSync(void)
{
    PcmVSync();
}

void SoundVSyncProc(void)
{
    SoundCmdProcess();
    PlayerUpdateAll();
    CgbUpdate();
    if (gPcmEnabled)
        PcmMixFrame();
}

// Four 176-byte buffers: gPcmBufA/B[0] and [1], left and right, mixed and played in turn.
void PcmInit(s8 *buffers)
{
    u8 unused[4];  // FAKEMATCH: the original has an unused 4-byte stack slot

    gPcmEnabled = TRUE;
    gPcmBufA[0] = buffers;
    gPcmBufB[0] = buffers + PCM_FRAME_SAMPLES;
    gPcmBufA[1] = buffers + PCM_FRAME_SAMPLES * 2;
    gPcmBufB[1] = buffers + PCM_FRAME_SAMPLES * 3;
    gPcmTimerReload = 0xF9C4;  // 16.78 MHz / (0x10000 - 0xF9C4) = 10512 Hz
    gPcmBufIndex = 0;
    REG_SOUNDCNT_H_HI = 0x9A;
    REG_FIFO_A = 0;
    REG_FIFO_B = 0;
}

void PcmVSync(void)
{
    REG_TM0CNT = gPcmTimerReload | 0x800000;
    if (gPcmEnabled)
    {
        DmaStop(1);
        DmaStop(2);
        DmaSet(1, gPcmBufA[gPcmBufIndex], &REG_FIFO_A, 0xB6400004);
        DmaSet(2, gPcmBufB[gPcmBufIndex], &REG_FIFO_B, 0xB6400004);
        gPcmBufIndex = 1 - gPcmBufIndex;
    }
}

void SoundDmaStop(void)
{
    gPcmEnabled = FALSE;
    DmaStop(1);
    DmaStop(2);
}

void SoundDmaStart(void)
{
    gPcmEnabled = TRUE;
}

// Mix PCM_FRAME_SAMPLES of a voice into gPcmMixBuffer; a looped sample wraps as often as it
// needs to. TRUE when a one-shot sample ran out.
u8 PcmMixVoice(struct SoundVoice *voice, u32 volume, u32 step, u8 pan)
{
    u32 pos = voice->pos;
    u16 vol = volume;
    u8 done = FALSE;
    u32 left = PCM_FRAME_SAMPLES;
    s16 *mixL;
    s16 *mixR;
    u32 volL;
    s8 *data = voice->sample->data;
    u32 end;
    s16 *mixEnd;
    u32 loopLen;

    mixL = gPcmMixBuffer;
    mixR = mixL + PCM_FRAME_SAMPLES;
    volL = (u16)(vol * (127 - pan)) >> 8;
    vol = (u16)(vol * pan) >> 8;  // now the right channel's
    end = voice->sample->loopEnd;
    if (end == 0)
        end = voice->sample->length;
    // Reaching the end within this frame: mix only the samples up to it (rounded up).
    if ((pos + left * step) >> 8 < end)
    {
        mixEnd = mixL + left;
    }
    else
    {
        mixEnd = mixL + Div((end << 8) - pos - (1 - step), step);
        done = TRUE;
    }
    if (voice->sample->loopEnd == 0 || !done)
    {
        pos = ((PcmMixFunc)gPcmMixFunc)(data, mixL, mixR, mixEnd, pos, step, volL, vol);
        if (done)
            return TRUE;
    }
    else
    {
        loopLen = voice->sample->loopEnd - voice->sample->loopStart;
        loopLen <<= 8;
        pos = ((PcmMixFunc)gPcmMixFunc)(data, mixL, mixR, mixEnd, pos, step, volL, vol);
        pos -= loopLen;
        left -= mixEnd - mixL;
        while (left != 0)
        {
            mixL = mixEnd;
            mixR = mixL + PCM_FRAME_SAMPLES;
            if ((pos + left * step) >> 8 < end)
            {
                mixEnd += left;
                done = FALSE;
            }
            else
            {
                mixEnd += Div((end << 8) - pos - (1 - step), step);
                done = TRUE;
            }
            pos = ((PcmMixFunc)gPcmMixFunc)(data, mixL, mixR, mixEnd, pos, step, volL, vol);
            if (done)
                pos -= loopLen;
            left -= mixEnd - mixL;
        }
    }
    voice->pos = pos;
    return FALSE;
}

// The instrument of a drum kit key: PCM, default envelope, played at its natural pitch.
void DrumInstrumentInit(void)
{
    gDrumInstrument.type = 0;
    gDrumInstrument.flags = 0;
    gDrumInstrument.arg = 0;
    gDrumInstrument.envelope = 0;
    gDrumInstrument.release = 0;
    gDrumInstrument.baseKey = 0x30;
}

// Resolve the track's program for key through key splits, drum kits and key ranges to an
// instrument record and its envelope (and a GB channel's wave or duty list).
void TrackLookupInstrument(struct SoundTrack *track, u8 key, struct InstLookup *out)
{
    const struct SoundBankHeader *header = gSoundBank;
    const u8 *bank = SOUND_OFFSET(header->instBanks, track->player->bankMap[track->bank]);
    u32 program = track->program * 2;
    const struct SoundInstrument *inst = (const void *)(bank + *(const u16 *)(bank + program));

    out->fixedPan = FALSE;
    out->isDrum = FALSE;
    if (inst->type & 0xF0)
    {
        if (inst->type == 0x10)
        {
            // Key split: {u16 record; u8 pan; u8} per key from the record's byte 4.
            const u8 *split = bank + inst->arg;
            u8 i = key - ((const u8 *)inst)[4];

            out->fixedPan = TRUE;
            out->pan = split[i * 4 + 2];
            out->inst = (const void *)(bank + *(u16 *)(split + i * 4));
            out->envelope = (const void *)(bank + out->inst->envelope);
        }
        else if (inst->type == 0x11)
        {
            // Drum kit: a sample per key, played by gDrumInstrument.
            const u16 *samples = (const void *)(bank + inst->arg);

            gDrumInstrument.arg = samples[key];
            out->inst = &gDrumInstrument;
            out->envelope = gDefaultEnvelope;
            out->isDrum = TRUE;
        }
        else if (inst->type == 0x12)
        {
            // Key ranges: {u8 top key; u8; u16 record}.
            const u8 *range = bank + inst->arg;

            while (key > range[0])
                range += 4;
            out->inst = (const void *)(bank + *(u16 *)(range + 2));
            out->envelope = (const void *)(bank + out->inst->envelope);
        }
    }
    else
    {
        out->inst = inst;
        out->envelope = (const void *)(bank + inst->envelope);
    }
    if (out->inst->type == 3)
        out->wave = bank + inst->arg;
    if (out->inst->flags & 1)
        out->dutyList = bank + out->inst->arg;
}

// GB voices 1-4 on their channels, the PCM voices all free in one list.
void VoicesInit(void)
{
    s32 i;
    struct PackedVoice *voices;

    gCgbWaveLoaded = NULL;
    for (i = 0; i < 4; i++)
    {
        PACKED_CGB_VOICES[i].state = 0;
        PACKED_CGB_VOICES[i].track = NULL;
    }
    PACKED_CGB_VOICES[0].type = 1;
    PACKED_CGB_VOICES[1].type = 2;
    PACKED_CGB_VOICES[2].type = 3;
    PACKED_CGB_VOICES[3].type = 4;
    for (i = 0; i < 7; i++)
    {
        PACKED_PCM_VOICES[i].state = 0;
        PACKED_PCM_VOICES[i].type = 0;
        PACKED_PCM_VOICES[i].track = NULL;
    }
    gPcmVoiceCount = 7;
    gPcmFreeVoices = gPcmVoices;
    PACKED_PCM_VOICES[0].next = &PACKED_PCM_VOICES[1];
    PACKED_PCM_VOICES[0].prev = NULL;
    for (i = 1; i < 6; i++)
    {
        PACKED_PCM_VOICES[i].next = &PACKED_PCM_VOICES[i + 1];
        PACKED_PCM_VOICES[i].prev = &PACKED_PCM_VOICES[i - 1];
    }
    voices = PACKED_PCM_VOICES;  // FAKEMATCH: indexing PACKED_PCM_VOICES hoists &[6]
    voices[6].prev = &voices[5];
    voices[6].next = NULL;
    gPcmActiveVoices = NULL;
    gPcmDisabledVoices = NULL;
}


void VoiceListRemove(struct SoundVoice *voice, struct SoundVoice **list)
{
    struct SoundVoice *prev = voice->prev;
    struct SoundVoice *next = voice->next;

    if (prev != NULL)
        prev->next = next;
    else
        *list = next;
    if (next != NULL)
        next->prev = prev;
}

void VoiceListPush(struct SoundVoice *voice, struct SoundVoice **list)
{
    struct SoundVoice *head = *list;

    voice->next = head;
    voice->prev = NULL;
    if (head != NULL)
        head->prev = voice;
    *list = voice;
}

// Insert voice into gPcmActiveVoices, kept in order: released voices (lowest priority first),
// then playing ones (lowest priority first), so VoiceAlloc steals from the front.
void VoiceListInsert(struct SoundVoice *voice)
{
    struct SoundVoice *prev = NULL;
    struct SoundVoice *cur = gPcmActiveVoices;

    if (voice->state == 1)
    {
        while (cur != NULL && (cur->state != 1 || voice->priority >= cur->priority))
        {
            prev = cur;
            cur = prev->next;
        }
    }
    else if (voice->state == 2)
    {
        while (cur != NULL && cur->state != 1 && voice->priority >= cur->priority)
        {
            prev = cur;
            cur = prev->next;
        }
    }
    else
    {
        return;
    }
    voice->next = cur;
    if (cur != NULL)
        cur->prev = voice;
    voice->prev = prev;
    if (prev != NULL)
        prev->next = voice;
    else
        gPcmActiveVoices = voice;
}

// A voice's frequency value for key (relative to the instrument's base key): the PCM step
// (gPcmPitchTable), the noise channel's key, or a GB channel's frequency register.
u32 VoiceNoteFreq(struct SoundVoice *voice, u8 key, u8 baseKey)
{
    s16 k = key + 0x30 - baseKey;

    if (k < 0)
        k = 0;
    else if (k > 0x77)
        k = 0x78;
    if (voice->type == 0)
        return gPcmPitchTable[k];
    if (voice->type == 4)
        return k;
    return gCgbFreqTable[k];
}

u8 NoiseFreq(u16 key)
{
    if (key > 0x77)
        key = 0x77;
    return gNoiseFreqTable[key];
}

// Advance the envelope by a frame; at the end of a step, set up the next (a negative frame
// count holds the current one). Returns the level.
u32 EnvelopeStep(struct SoundVoice *voice)
{
    struct VoiceEnvelope *env = &voice->env;

    if (env->frames == 0)
    {
        u8 index;

        voice->env.level = env->target;
        index = env->index;
        env->index = index + 1;
        if (env->steps[env->index].frames < 0)
            env->index = index;
        env->target = env->steps[env->index].level;
        env->frames = env->steps[env->index].frames;
        env->step = env->target;
        env->step -= voice->env.level;
        env->step /= env->frames;
    }
    env->level += env->step;
    env->frames--;
    return env->level;
}

void SoundCmdNop(u8 arg)
{
}

// Move voices between gPcmDisabledVoices and gPcmFreeVoices (taking playing ones with
// VoiceAlloc) until count PCM voices are enabled.
void SetPcmVoiceCount(u8 count)
{
    s8 diff = count - gPcmVoiceCount;
    struct SoundVoice *voice;
    struct SoundVoice *next;

    if (diff == 0)
        return;
    if (diff > 0)
    {
        next = gPcmDisabledVoices;
        while (next != NULL && diff > 0)
        {
            voice = next;
            next = voice->next;
            VoiceListRemove(voice, &gPcmDisabledVoices);
            VoiceListPush(voice, &gPcmFreeVoices);
            diff--;
            gPcmVoiceCount++;
        }
    }
    else
    {
        diff = -diff;
        while (diff > 0)
        {
            voice = VoiceAlloc(0, 0xFF);
            if (voice == NULL)
                break;
            VoiceListRemove(voice, &gPcmActiveVoices);
            voice->state = 0;
            VoiceListPush(voice, &gPcmDisabledVoices);
            diff--;
            gPcmVoiceCount--;
        }
    }
}

// A PCM voice's volume this frame (0-0x7F): velocity, the player's fade and volumes, the
// track's volumes and the envelope; a released voice decays by gRelease.
u32 PcmVoiceVolume(struct SoundVoice *voice)
{
    u32 volume;

    if (voice->state == 1)
    {
        struct SoundTrack *track;
        struct SoundPlayer *player;

        volume = voice->velocity;
        volume <<= 7;
        track = voice->track;
        player = track->player;
        volume *= player->fadeVolume;
        volume >>= 8;
        volume *= player->unk40;
        volume >>= 7;
        volume *= player->volume;
        volume >>= 8;
        volume *= track->volume;
        volume >>= 8;
        volume *= track->cmdVolume;
        volume >>= 15;
        volume *= EnvelopeStep(voice);
        volume >>= 11;
        voice->volume = volume;
    }
    else
    {
        volume = voice->volume = (voice->release + 0xE6) * voice->volume >> 9;
    }
    volume >>= 8;
    return volume;
}

// The NRx2 envelope byte for a GB voice at the start of each envelope step (8 = no change
// otherwise): the start volume and a hardware step toward the step's target. For the wave
// channel, the output level (0-4).
// newStep doubles as the result (the original keeps both in one register), and both results
// leave through one return.
u8 CgbVoiceVolume(struct SoundVoice *voice, u8 loud)
{
    u8 newStep;
    u32 volume;
    u32 end;
    u16 frames;

    newStep = FALSE;
    volume = voice->velocity;
    if (voice->env.frames == 0)
        newStep = TRUE;
    EnvelopeStep(voice);
    if (!newStep)
        return 8;
    if (loud)
        volume <<= 1;
    volume <<= 15;
    volume *= voice->track->volume;
    volume >>= 14;
    volume *= voice->track->cmdVolume;
    volume >>= 7;
    volume *= voice->track->player->unk40;
    volume >>= 7;
    volume *= voice->track->player->volume;
    volume >>= 8;
    volume *= voice->track->player->fadeVolume;
    if (voice->type == 3)
    {
        volume >>= 22;
        voice->volume = volume;
        volume *= 5;
        volume >>= 7;
        if (volume > 4)
            volume = 4;
        return volume;
    }
    volume >>= 15;
    voice->volume = volume;
    volume *= voice->env.level;
    volume >>= 25;
    if (volume & ~0xF)
        volume = 15;
    voice->volume = voice->volume * voice->env.target >> 25;
    if (voice->volume & ~0xF)
        voice->volume = 15;
    end = voice->volume;
    if (end != volume)
    {
        s32 diff;

        frames = voice->env.frames;
        frames += 15;
        diff = end - volume;
        if (diff < 0)
            diff = -diff;
        frames = frames / diff;
        if (frames != 0)
            goto step;
    }
    newStep = (volume << 4) | 8;
    goto done;
step:
    if (frames & 0xFFF8)
        frames = 7;
    newStep = (volume << 4) | frames;
    if (volume < end)
        newStep |= 8;
done:
    return newStep;
}


// This frame's frequency value: the base plus the pitch sweep, the track's bend (a PCM step
// scales; a GB channel's register value counts down from 0x800), and vibrato.
// Near-miss: same code shape; freq and the sweep pointer swap r2/r3, and the vibrato pointer
// and depth take r7/r6 (one more saved register) where the original has r4/r3.
//     u32 VoiceUpdatePitch(struct SoundVoice *voice)
//     {
//         u32 freq = voice->baseFreq;
//         struct SoundTrack *track = voice->track;
//         struct VoiceSweep *sweep = &voice->sweep;
//         struct VoiceVibrato *vib;
//         u32 depth;
//
//         if (sweep->delay != 0)
//         {
//             sweep->delay--;
//         }
//         else if (sweep->frames != 0)
//         {
//             sweep->offset += sweep->step;
//             if (--sweep->frames == 0)
//                 sweep->offset = sweep->target;
//         }
//         freq += sweep->offset;
//         if (track->bend != 0)
//         {
//             u32 bend;
//
//             if (track->bend > 0)
//             {
//                 bend = track->bend;
//                 bend *= gPcmPitchTable[track->bendRange + 0x30];
//                 bend += 0x400000;
//                 if (voice->type == 0)
//                 {
//                     bend >>= 7;
//                     freq *= bend;
//                     freq >>= 15;
//                 }
//                 else
//                 {
//                     freq = 0x800 - freq;
//                     freq <<= 22;
//                     freq /= bend;
//                     freq = 0x800 - freq;
//                 }
//             }
//             else
//             {
//                 bend = -track->bend;
//                 bend *= gPcmPitchTable[track->bendRange + 0x30];
//                 bend += 0x400000;
//                 if (voice->type == 0)
//                 {
//                     freq <<= 15;
//                     bend >>= 7;
//                     freq /= bend;
//                 }
//                 else
//                 {
//                     freq = 0x800 - freq;
//                     bend >>= 7;
//                     freq *= bend;
//                     freq >>= 15;
//                     if (freq < 0x800)
//                         freq = 0x800 - freq;
//                     else
//                         freq = 0;
//                 }
//             }
//         }
//         vib = &voice->vibrato;
//         depth = vib->params->depth;
//         if (depth != 0)
//         {
//             if (vib->delay == 0)
//             {
//                 s8 v = gVibratoTable[vib->phase >> 1];
//
//                 if (voice->type == 0)
//                 {
//                     if (v >= 0)
//                         freq += v * freq * depth >> 19;
//                     else
//                     {
//                         freq <<= 12;
//                         freq /= (-v * depth >> 3) + 0x10000;
//                         freq <<= 4;
//                     }
//                 }
//                 else
//                 {
//                     if (v >= 0)
//                         freq = 0x800 - ((0x800 - freq) << 19) / (v * depth + 0x80000);
//                     else
//                         freq = 0x800 - ((0x800 - freq) * (-v * depth + 0x80000) >> 19);
//                 }
//                 vib->phase += vib->params->speed;
//                 if ((vib->phase >> 1) > 0xFF)
//                     vib->phase -= 0x200;
//             }
//             else
//             {
//                 vib->delay--;
//             }
//         }
//         return freq;
//     }
INCLUDE_ASM(VoiceUpdatePitch);

// Mix every active PCM voice into gPcmMixBuffer, release the voices whose gate ran out, and
// clip the mix into the DMA buffers.
void PcmMixFrame(void)
{
    struct SoundVoice *next = gPcmActiveVoices;
    s32 i;

    CpuFastFill(0, gPcmMixBuffer, sizeof(gPcmMixBuffer));
    while (next != NULL)
    {
        struct SoundVoice *voice = next;
        u32 volume;
        u8 pan;
        u32 step;

        next = voice->next;
        volume = PcmVoiceVolume(voice);
        if (voice->state == 1)
        {
            struct SoundTrack *track;

            voice->gate--;
            track = voice->track;
            pan = voice->fixedPan ? voice->pan : track->pan;
            step = VoiceUpdatePitch(voice);
            voice->freq.pcm = step;
            voice->unk1A = track->unk4C;
        }
        else
        {
            if (volume == 0)
            {
                VoiceStop(voice);
                continue;
            }
            pan = voice->pan;
            step = voice->freq.pcm;
        }
        step >>= 2;
        step *= voice->sample->rate;
        step /= PCM_SAMPLE_RATE;
        step >>= 5;
        if (PcmMixVoice(voice, volume, step, pan) == TRUE)
            VoiceStop(voice);
    }
        for (i = 0; i < 7; i++)
    {
        struct SoundVoice *voice = &gPcmVoices[i];

        if (voice->state == 1 && voice->gate == 0)
            VoiceNoteOff(voice);
    }
    ((PcmClipFunc)gPcmClipFunc)(gPcmMixBuffer, gPcmBufA[gPcmBufIndex], gPcmMixBuffer + ARRAY_COUNT(gPcmMixBuffer));
}

u8 CgbVoiceVolume(struct SoundVoice *voice, u8 loud);
void CgbVoiceStart(struct SoundVoice *voice, u8 nr2);

// Per frame for each GB voice: release it when its gate ran out, then update the pitch,
// the NR51 routing (pan 0x40 both sides, below left, above right), the envelope (new
// NRx2 bytes from CgbVoiceVolume restart the channel) and the duty list. A note's first
// frame starts the channel instead; a released wave voice decays in software.
void CgbUpdate(void)
{
    s32 i;

    for (i = 0; i < 4; i++)
    {
        struct SoundVoice *voice = &gCgbVoices[i];
        u32 freq;
        u8 pan;
        u8 nr2;
        u8 ch;
        u8 mask;
        u8 duty;
        vu8 *nr51;

        if (voice->state == 1 && voice->gate == 0)
            VoiceNoteOff(voice);
        if (voice->state == 0)
            continue;
        if (voice->state == 1)
        {
            freq = VoiceUpdatePitch(voice);
            voice->freq.pcm = freq;
            pan = voice->fixedPan ? voice->pan : voice->track->pan;
        }
        else
        {
            freq = voice->freq.pcm;
            pan = voice->pan;
        }
        nr2 = CgbVoiceVolume(voice, pan != 0x40);
        nr51 = &REG_NR51;
        ch = voice->type - 1;
        mask = ~(0x11 << ch);
        if (pan == 0x40)
            *nr51 = (*nr51 & mask) | (0x11 << ch);
        else if (pan < 0x40)
            *nr51 = (*nr51 & mask) | (0x10 << ch);
        else
            *nr51 = (*nr51 & mask) | (1 << ch);
        if (voice->state == 1)
        {
            if (voice->pos == 0)
            {
                CgbVoiceStart(voice, nr2);
                voice->pos = 1;
                voice->gate--;
                continue;
            }
            voice->pos++;
            voice->gate--;
        }
        else if (voice->type == 3)
        {
            u32 volume;

            voice->volume = (voice->release + 0xE6) * voice->volume >> 9;
            volume = voice->volume;
            if (pan != 0x40)
                volume <<= 1;
            volume *= 5;
            volume >>= 7;
            if (volume != 0)
            {
                u8 level;

                if (volume > 4)
                    volume = 4;
                level = volume;
                REG_NR32 = gCgbWaveVolumes[level];
                continue;
            }
            VoiceStop(voice);
            continue;
        }
        if (voice->inst->flags & 1)
        {
            const u8 *list = voice->cgb.ptr;
            u16 n = *(const u16 *)list;

            if (voice->pos < n)
                duty = *(list + voice->pos + 2);
            else
                duty = list[n + 1];
        }
        else
        {
            duty = 0xFF;
        }
        switch (voice->type)
        {
        case 1:
            if (nr2 != 8)
            {
                REG_NR12 = nr2;
                REG_SOUND1CNT_X = freq | 0x8000;
            }
            else if (((const u8 *)voice->inst)[8] == 8)  // the sweep byte after the record
            {
                REG_SOUND1CNT_X = freq;
            }
            REG_NR11 &= 0xC0;
            if (duty != 0xFF)
                REG_NR11 = duty << 6;
            break;
        case 2:
            if (nr2 != 8)
            {
                REG_NR22 = nr2;
                REG_SOUND2CNT_H = freq | 0x8000;
            }
            else
            {
                REG_SOUND2CNT_H = freq;
            }
            REG_NR21 &= 0xC0;
            if (duty != 0xFF)
                REG_NR21 = duty << 6;
            break;
        case 3:
            REG_SOUND3CNT_X = (REG_SOUND3CNT_X & 0x4000) | freq;
            if (nr2 != 8)
                REG_NR32 = gCgbWaveVolumes[nr2];
            break;
        case 4:
            if (nr2 != 8)
            {
                REG_NR42 = nr2;
                REG_NR44 = 0x80;
            }
            if (duty != 0xFF)
            {
                vu8 *nr43 = &REG_NR43;
                u8 v = NoiseFreq(freq);

                if (duty != 0)
                    v |= 8;
                *nr43 = v;
            }
            else
            {
                REG_NR43 = (REG_NR43 & 8) | NoiseFreq(freq);
            }
            break;
        }
    }
}

void TrackAddVoice(struct SoundTrack *track, struct SoundVoice *voice);
void bzero(void *dest, u32 size);

// Play key on the track for length ticks: resolve the instrument, take a voice (the tied
// note's, or a new one from VoiceAlloc) and set up its pitch, sweep, envelope and sample.
void TrackNoteOn(struct SoundTrack *track, u8 key, u8 velocity, u32 length)
{
    struct InstLookup lookup;
    struct SoundPlayer *player = track->player;
    u8 noteKey = key;
    const struct SoundInstrument *inst;
    struct SoundVoice *voice;

    if (track->mute)
        return;
    key += track->transpose;
    TrackLookupInstrument(track, key, &lookup);
    inst = lookup.inst;
    if (inst->flags & 0x10)
        length /= player->tempo;
    else
        length /= player->tempoOfs + player->tempo;
    if (track->tie && track->voices != NULL)
    {
        voice = track->voices;
    }
    else
    {
        voice = VoiceAlloc(gInstTypeToVoicePool[inst->type], track->priority);
        if (voice == NULL)
            return;
        TrackAddVoice(track, voice);
        voice->key = noteKey;
        voice->pos = 0;
        voice->vibrato.params = &track->vibrato;
        voice->vibrato.phase = 0;
        voice->vibrato.delay = track->vibrato.delay;
        voice->env.steps = lookup.envelope;
        voice->env.level = 0;
        voice->env.target = 0;
        voice->env.frames = 0;
        voice->env.index = -1;
        voice->release = inst->release;
        voice->inst = inst;
    }
    voice->fixedPan = lookup.fixedPan;
    if (voice->fixedPan)
    {
        key = 0x30;
        voice->pan = lookup.pan;
    }
    else if (lookup.isDrum)
    {
        key = 0x30;
    }
    voice->velocity = velocity;
    voice->volume = 0;
    voice->gate = length;
    voice->unk1A = track->unk4C;
    voice->baseFreq = VoiceNoteFreq(voice, key, inst->baseKey);
    if (track->sweep1C == 0)
    {
        bzero(&voice->sweep, sizeof(voice->sweep));
    }
    else
    {
        u32 freq = VoiceNoteFreq(voice, track->sweep1E, inst->baseKey);

        voice->sweep.delay = track->sweep20;
        voice->sweep.frames = length * track->sweep22 >> 8;
        if (track->sweep1D & 2)
        {
            voice->sweep.target = freq - voice->baseFreq;
        }
        else
        {
            voice->sweep.target = voice->baseFreq - freq;
            voice->baseFreq = freq;
        }
        voice->sweep.step = voice->sweep.target;
        voice->sweep.step = voice->sweep.step / (s32)voice->sweep.frames;
        if (track->sweep1D & 4)
            track->sweep1E = key;
        else
            track->sweep1C = 0;
        voice->sweep.offset = 0;
    }
    if (voice->type == 0)
        voice->sample = SOUND_OFFSET(track->sampleSet, inst->arg);
    else if (voice->type != 3)
    {
        if (inst->flags & 1)
            voice->cgb.ptr = lookup.dutyList;
        else
            voice->cgb.duty = inst->arg;
    }
    else
    {
        voice->cgb.ptr = lookup.wave;
    }
    if (length == 0)
        VoiceNoteOff(voice);
}

// Release a note: a PCM voice (and the wave channel) decays in software, the other GB
// channels get their hardware envelope (release / 32 as the step, or off).
// The switch re-reads voice->type (the volatile read keeps cse from reusing `type`).
void VoiceNoteOff(struct SoundVoice *voice)
{
    if (voice->state == 1 && !voice->track->tie)
    {
        u8 type = voice->type;

        if (voice->type == 0)
        {
            VoiceListRemove(voice, &gPcmActiveVoices);
            voice->state = 2;
            VoiceListInsert(voice);
        }
        else
        {
            u16 freq = voice->freq.cgb;
            u8 step = voice->release;

            if (type == 3)
            {
                voice->state = 2;
            }
            else
            {
                u8 env;

                step >>= 5;
                if (step == 0)
                    env = 0;
                else
                    env = step | (voice->volume << 4);
                switch (*(volatile u8 *)&voice->type)
                {
                case 1:
                    REG_NR12 = env;
                    REG_SOUND1CNT_X = freq | 0x8000;
                    REG_NR11 &= 0xC0;
                    break;
                case 2:
                    REG_NR22 = env;
                    REG_SOUND2CNT_H = freq | 0x8000;
                    REG_NR21 &= 0xC0;
                    break;
                case 4:
                    REG_NR42 = env;
                    REG_NR44 = 0x80;
                    break;
                }
                voice->state = 0;
            }
        }
        if (voice->fixedPan == 0)
            voice->pan = voice->track->pan;
        TrackRemoveVoice(voice->track, voice);
    }
}

void VoiceStop(struct SoundVoice *voice)
{
    if (voice->state != 0)
    {
        switch (voice->type)
        {
        case 0:
            VoiceListRemove(voice, &gPcmActiveVoices);
            VoiceListPush(voice, &gPcmFreeVoices);
            break;
        case 1:
            REG_NR12 = 8;
            REG_NR14 = 0xC0;
            break;
        case 2:
            REG_NR22 = 8;
            REG_NR24 = 0xC0;
            break;
        case 3:
            REG_NR30 = 0;
            break;
        case 4:
            REG_NR42 = 8;
            REG_NR44 = 0xC0;
            break;
        }
        TrackRemoveVoice(voice->track, voice);
        voice->state = 0;
    }
}

// Start a note on a GB channel voice with the envelope byte nr2 (the wave channel: output
// level index): duty (from the duty list's first entry with flags bit 0), frequency, trigger.
void CgbVoiceStart(struct SoundVoice *voice, u8 nr2)
{
    switch (voice->type)
    {
    case 1:
        REG_NR10 = ((const u8 *)voice->inst)[8];
        REG_SOUND1CNT_X = voice->baseFreq | 0x8000;
        REG_NR12 = nr2;
        if (voice->inst->flags & 1)
            REG_NR11 = voice->cgb.ptr[2] << 6;
        else
            REG_NR11 = voice->cgb.duty << 6;
        REG_SOUND1CNT_X = voice->baseFreq | 0x8000;
        break;
    case 2:
        REG_NR22 = nr2;
        REG_SOUND2CNT_H = voice->baseFreq | 0x8000;
        REG_NR21 = voice->cgb.duty << 6;
        break;
    case 3:
        if (voice->cgb.ptr != gCgbWaveLoaded)
        {
            REG_NR30 = 0;
            CpuSet(voice->cgb.ptr, REG_WAVE_RAM, 8);
            gCgbWaveLoaded = voice->cgb.ptr;
        }
        REG_NR30 = 0xC0;
        REG_SOUND3CNT_X = voice->baseFreq | 0x8000;
        REG_NR32 = gCgbWaveVolumes[nr2];
        REG_NR31 = 0;
        break;
    case 4:
    {
        u8 v;

        REG_NR42 = nr2;
        if (voice->inst->flags & 1)
        {
            v = NoiseFreq(voice->baseFreq);
            if (voice->cgb.ptr[2])
                v |= 8;
        }
        else
        {
            v = NoiseFreq(voice->baseFreq);
            if (voice->cgb.duty)
                v |= 8;
        }
        REG_NR43 = v;
        REG_NR44 = 0x80;
        REG_NR41 = 0;
        break;
    }
    }
}

// A voice for a note of this priority: a free PCM voice, else the first in gPcmActiveVoices
// (released or lowest priority) unless it plays a higher priority; or GB channel `type`.
struct SoundVoice *VoiceAlloc(u8 type, u8 priority)
{
    struct SoundVoice *voice;

    if (type == 0)
    {
        if (gPcmFreeVoices != NULL)
        {
            voice = gPcmFreeVoices;
        }
        else
        {
            struct SoundVoice *active = gPcmActiveVoices;

            if (active == NULL)
                return NULL;
            if (active->state == 1 && priority < active->priority)
                return NULL;
            voice = active;
            VoiceStop(voice);
        }
        VoiceListRemove(voice, &gPcmFreeVoices);
        voice->state = 1;
        voice->priority = priority;
        VoiceListInsert(voice);
    }
    else
    {
        voice = &gCgbVoices[type - 1];
        if (voice->state == 1 && priority < voice->priority)
            return NULL;
        if (voice->state != 0)
            VoiceStop(voice);
        voice->state = 1;
        voice->priority = priority;
    }
    return voice;
}

void TracksInit(void)
{
    s32 i;

    for (i = 0; i < 24; i++)
    {
        struct PackedTrack *track = (struct PackedTrack *)&gSoundTracks[i];

        track->player = NULL;
        track->voices = NULL;
    }
}


struct SoundTrack *TrackAlloc(void)
{
    s32 i;

    for (i = 0; i < 24; i++)
    {
        struct SoundTrack *track = &gSoundTracks[i];

        if (track->player == NULL)
            return track;
    }
    return NULL;
}

void TrackStart(struct SoundTrack *track, struct SoundPlayer *player, const u8 *pc)
{
    if (track != NULL)
    {
        if (track->player != NULL)
            TrackStop(track);
        track->wait = 0;
        track->tie = 0;
        track->mute = 0;
        track->pc = pc;
        track->player = player;
        track->voices = NULL;
        TrackSetBank(track, 0);
        track->pan = 0x40;
        track->vibrato.delay = 0;
        track->vibrato.speed = 0x22;
        track->vibrato.depth = 0;
        track->sweep1C = 0;
        track->sweep1D = 0;
        track->sweep1E = 0;
        track->sweep20 = 0;
        track->sweep22 = 0;
        track->volume = 0x80;
        track->cmdVolume = 0x80;
        track->bend = 0;
        track->bendRange = 2;
        track->transpose = 0;
        if (player->isSe == TRUE)
        {
            track->priority = 12;
            track->unk4C = 0x7F;
            track->notesWait = TRUE;
        }
        else
        {
            track->priority = 3;
            track->unk4C = 0;
            track->notesWait = FALSE;
        }
        track->lastLength = 0x7F;
        track->velocity = 0x7F;
        track->lastWait = 0;
        track->sp = track->stack;
    }
}

void TrackReleaseNotes(struct SoundTrack *track)
{
    if (track != NULL)
    {
        u8 tie = track->tie;
        struct SoundVoice *voice;

        track->tie = FALSE;
        voice = track->voices;
        while (voice != NULL)
        {
            struct SoundVoice *next = voice->trackNext;

            VoiceNoteOff(voice);
            voice = next;
        }
        track->tie = tie;
    }
}

void TrackStop(struct SoundTrack *track)
{
    if (track != NULL)
    {
        TrackReleaseNotes(track);
        track->player = NULL;
    }
}

// One frame of a track: run its byte code (docs/layout.md) until it waits, then count the
// wait down by the player's tempo. Returns 0 while playing, 1 without a track or player, 2
// when it ended (FF with an empty call stack). Jump offsets are little-endian u16s.
u8 TrackExecute(struct SoundTrack *track)
{
    struct SoundPlayer *player;
    u8 cmd;
    u32 len;
    u8 velocity;
    u16 ofs;
    const u8 *base;

    if (track == NULL || track->player == NULL)
        return 1;
    player = track->player;
    if (player->paused)
    {
        TrackReleaseNotes(track);
        return 0;
    }
    while (track->wait <= 0)
    {
        cmd = *track->pc++;
        if (cmd < 0xC0)
        {
            if (cmd < 0x60)
            {
                len = track->lastLength;
                velocity = track->velocity;
            }
            else
            {
                len = (u16)TrackReadVarLen(track);
                track->lastLength = len;
                velocity = *track->pc++;
                track->velocity = velocity;
                cmd -= 0x60;
            }
            len *= 150;
            if (gSoundNoteHook != NULL && (player->flags & 1))
                gSoundNoteHook(track, cmd, velocity, len);
            else
                TrackNoteOn(track, cmd, velocity, len);
            if (track->notesWait == TRUE)
                track->wait += len;
        }
        else if (cmd == 0xC0)
        {
            len = track->lastWait;
            len *= 150;
            track->wait += len;
        }
        else if (cmd == 0xC1)
        {
            len = (u16)TrackReadVarLen(track);
            track->lastWait = len;
            len *= 150;
            track->wait += len;
        }
        else if ((cmd & 0xF0) == 0xD0)
        {
            u8 arg = cmd & 0xF;

            track->sweep1D = arg;
            track->sweep1E = *track->pc++ + track->transpose;
            track->sweep22 = *track->pc++;
            if (arg & 1)
                track->sweep20 = *track->pc++;
            else
                track->sweep20 = 0;
            track->sweep1C = 1;
        }
        else
        {
            switch (cmd)
            {
            case 0xFF:
                if (track->sp == track->stack)
                {
                    TrackStop(track);
                    return 2;
                }
                track->pc = *--track->sp;
                break;
            case 0xE8:
                track->sweep1C = 0;
                break;
            case 0xF0:
                ((u8 *)&ofs)[0] = *track->pc++;
                ((u8 *)&ofs)[1] = *track->pc++;
                base = (const u8 *)player->seqOffsets;
                goto jump;
            case 0xF4:
                ((u8 *)&ofs)[0] = *track->pc++;
                ((u8 *)&ofs)[1] = *track->pc++;
                *track->sp++ = track->pc;
                base = (const u8 *)player->seqOffsets;
            jump:
                base += ofs;
                track->pc = base;
                break;
            case 0xC2:
                track->program = *track->pc++;
                break;
            case 0xC7:
                TrackSetBank(track, *track->pc++);
                break;
            case 0xC3:
                track->pan = *track->pc++;
                break;
            case 0xC4:
                track->priority = *track->pc++;
                break;
            case 0xE0:
                track->volume = *track->pc++;
                break;
            case 0xEA:
                track->player->unk40 = *track->pc++;
                break;
            case 0xE1:
                track->bend = *track->pc++;
                break;
            case 0xE2:
                track->bendRange = *track->pc++;
                break;
            case 0xE9:
                track->transpose = *track->pc++;
                break;
            case 0xE3:
                track->unk4C = *track->pc++;
                break;
            case 0xC5:
            case 0xC6:
                TrackReleaseNotes(track);
                track->tie = (cmd == 0xC5);
                break;
            case 0xC8:
                track->notesWait = TRUE;
                break;
            case 0xC9:
                track->notesWait = FALSE;
                break;
            case 0xCA:
                if (gSoundSeqHook != NULL)
                    gSoundSeqHook(track, *track->pc++);
                else
                    track->pc++;
                break;
            case 0xE4:
                player->tempo = TrackReadVarLen(track);
                break;
            case 0xE5:
                track->vibrato.delay = *track->pc++;
                break;
            case 0xE7:
                track->vibrato.depth = *track->pc++;
                break;
            case 0xE6:
                track->vibrato.speed = *track->pc++;
                break;
            case 0xF8:
            {
                u8 n = *track->pc++;
                struct SoundTrack *t;

                ((u8 *)&ofs)[0] = *track->pc++;
                ((u8 *)&ofs)[1] = *track->pc++;
                if (player->tracks[n] == NULL)
                {
                    t = TrackAlloc();
                    player->tracks[n] = t;
                }
                else
                {
                    t = player->tracks[n];
                    TrackStop(t);
                }
                TrackStart(t, player, (const u8 *)player->seqOffsets + ofs);
                t->sampleSet = track->sampleSet;
                t->bank = track->bank;
                t->program = track->program;
                t->pan = track->pan;
                t->unk4C = track->unk4C;
                t->volume = track->volume;
                t->cmdVolume = track->cmdVolume;
                t->priority = track->priority;
                t->bend = track->bend;
                t->bendRange = track->bendRange;
                t->transpose = track->transpose;
                break;
            }
            }
        }
    }
    track->wait -= player->tempo;
    track->wait -= player->tempoOfs;
    return 0;
}


void TrackAddVoice(struct SoundTrack *track, struct SoundVoice *voice)
{
    if (voice->track == NULL)
    {
        voice->track = track;
        voice->trackPrev = NULL;
        voice->trackNext = track->voices;
        track->voices = voice;
        if (voice->trackNext != NULL)
            voice->trackNext->trackPrev = voice;
    }
}

void TrackRemoveVoice(struct SoundTrack *track, struct SoundVoice *voice)
{
    if (voice->track != NULL)
    {
        voice->track = NULL;
        if (voice->trackNext != NULL)
            voice->trackNext->trackPrev = voice->trackPrev;
        if (voice->trackPrev != NULL)
            voice->trackPrev->trackNext = voice->trackNext;
        else
            track->voices = voice->trackNext;
    }
}

// A length or wait: one byte, or two (big-endian, 15 bits) when bit 7 is set.
u32 TrackReadVarLen(struct SoundTrack *track)
{
    u32 value = *track->pc++;

    if (value & 0x80)
        value = ((value & 0x7F) << 8) | *track->pc++;
    return value;
}

void TrackSetBank(struct SoundTrack *track, u32 bank)
{
    const struct SoundBankHeader *header;
    u16 set;

    track->bank = bank;
    track->program = 0;
    header = gSoundBank;
    set = header->bankToSampleSet[track->player->bankMap[bank]];
    track->sampleSet = SOUND_OFFSET(header->sampleSets, set);
}

void PlayersInit(void)
{
    s32 i, j;

    for (i = 0; i < 20; i++)
    {
        struct SoundPlayer *player = &gSoundPlayers[i];

        player->state = 0;
        for (j = 0; j < 10; j++)
            player->tracks[j] = NULL;
    }
}

void PlayerReset(struct SoundPlayer *player)
{
    player->paused = FALSE;
    player->tempo = 150;
    player->tempoOfs = 0;
    player->unk40 = 0x80;
    player->volume = 0x80;
    player->fadeVolume = 0x8000;
    player->fadeStep = 0;
    player->fadeFrames = 0;
    player->fadeTarget = 0;
    player->flags = 0;
}

// Fade, then run each track; stop the players whose tracks have all ended.
void PlayerUpdateAll(void)
{
    s32 i;

    for (i = 0; i < 20; i++)
    {
        struct SoundPlayer *player = &gSoundPlayers[i];

        if (player->state != 0)
        {
            bool32 playing;
            s32 j;

            if (player->fadeFrames == 0)
            {
                if (player->state == 2)
                {
                    PlayerStop(i);
                    continue;
                }
            }
            else
            {
                player->fadeVolume += player->fadeStep;
                if (--player->fadeFrames == 0)
                    player->fadeVolume = player->fadeTarget;
            }
            playing = FALSE;
            for (j = 0; j < 10; j++)
            {
                if (player->tracks[j] != NULL)
                {
                    if (TrackExecute(player->tracks[j]) == 0)
                        playing = TRUE;
                    else
                        player->tracks[j] = NULL;
                }
            }
            if (!playing)
                PlayerStop(i);
        }
    }
}

void PlayerStartSong(u32 player, u32 song)
{
    const struct SoundBankHeader *header = gSoundBank;

    PlayerStartSongData(player, SOUND_OFFSET(header->songs, song),
                        SOUND_OFFSET(header->songBankMaps, song));
}

void PlayerStartSe(u32 player, u32 set, u32 index)
{
    const struct SoundBankHeader *header = gSoundBank;

    PlayerStartSeData(player, SOUND_OFFSET(header->seSets, set),
                      SOUND_OFFSET(header->seBankMaps, set), index);
}

void PlayerStartSongData(u32 index, const void *song, const void *bankMap)
{
    struct SoundPlayer *player = &gSoundPlayers[index];
    const u16 *offsets;
    s32 count;
    s32 i;

    if (player->state != 0)
        PlayerStop(index);
    player->seqOffsets = song;
    player->bankMap = bankMap;
    player->isSe = FALSE;
    PlayerReset(player);
    offsets = player->seqOffsets;
    count = *(s8 *)offsets;
    offsets++;
    for (i = 0; i < count; i++)
    {
        if (offsets[i] != 0)
        {
            struct SoundTrack *track = TrackAlloc();

            player->tracks[i] = track;
            TrackStart(track, player, (const u8 *)player->seqOffsets + offsets[i]);
        }
    }
    player->state = 1;
}

void PlayerStartSeData(u32 index, const void *set, const void *bankMap, u32 se)
{
    struct SoundPlayer *player = &gSoundPlayers[index];
    struct SoundTrack *track;

    if (player->state != 0)
        PlayerStop(index);
    player->seqOffsets = set;
    player->bankMap = bankMap;
    player->isSe = TRUE;
    PlayerReset(player);
    track = TrackAlloc();
    player->tracks[0] = track;
    TrackStart(track, player, (const u8 *)player->seqOffsets + player->seqOffsets[se]);
    player->state = 1;
}

void PlayerStop(u32 index)
{
    struct SoundPlayer *player = &gSoundPlayers[index];

    if (player->state != 0)
    {
        s32 i;

        for (i = 0; i < 10; i++)
        {
            TrackStop(player->tracks[i]);
            player->tracks[i] = NULL;
        }
        player->state = 0;
    }
}

void PlayerFadeOut(u32 index, s32 frames)
{
    struct SoundPlayer *player = &gSoundPlayers[index];

    if (player->state != 0)
    {
        player->state = 2;
        player->fadeTarget = 0;
        player->fadeFrames = frames;
        player->fadeStep = -player->fadeVolume / frames;
    }
}

void PlayerSetPause(u32 index, u8 on)
{
    gSoundPlayers[index].paused = on;
}

// Near-miss: the original adds 0x42 to the table address before the index.
//     u32 PlayerGetState(u32 index)
//     {
//         return gSoundPlayers[index].state;
//     }
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

// Commands 000-006: a = player (START_SE: player << 16 | SE set), b = the argument.
void SoundCmdPlayer(struct SoundCmd *cmd)
{
    struct SoundPlayer *player = &gSoundPlayers[cmd->a];

    switch (cmd->cmd)
    {
    case SOUND_CMD_START_SONG:
        PlayerStartSong(cmd->a, cmd->b);
        break;
    case SOUND_CMD_START_SE:
        PlayerStartSe(cmd->a >> 16, cmd->a & 0xFFFF, cmd->b);
        break;
    case SOUND_CMD_FADE_OUT:
        PlayerFadeOut(cmd->a, cmd->b);
        break;
    case SOUND_CMD_PAUSE:
        PlayerSetPause(cmd->a, cmd->b);
        break;
    case SOUND_CMD_SET_FLAGS:
        player->flags = cmd->b;
        break;
    case SOUND_CMD_SET_TEMPO:
        player->tempoOfs = cmd->b;
        break;
    case SOUND_CMD_SET_VOLUME:
        player->volume = cmd->b;
        break;
    }
}

// Commands 100-103: a = player << 16 | value, b = mask of the player's tracks (shifted out).
// Near-miss: in the bend case the range, bend and the constant 1 get other registers
// (r2/r4/r6 in the original); the types and a shared value variable change nothing.
//     void SoundCmdTracks(struct SoundCmd *cmd)
//     {
//         u32 a = cmd->a;
//         struct SoundPlayer *player = &gSoundPlayers[a >> 16];
//         s32 i = 0;
//
//         switch (cmd->cmd)
//         {
//         case SOUND_CMD_MUTE_TRACKS:
//         {
//             u8 mute = a;
//
//             for (; cmd->b != 0 && i < 10; i++, cmd->b >>= 1)
//             {
//                 if ((cmd->b & 1) && player->tracks[i] != NULL)
//                     player->tracks[i]->mute = mute;
//             }
//             break;
//         }
//         case SOUND_CMD_SET_TRACK_VOLUME:
//         {
//             u8 volume = a;
//
//             for (; cmd->b != 0 && i < 10; i++, cmd->b >>= 1)
//             {
//                 if ((cmd->b & 1) && player->tracks[i] != NULL)
//                     player->tracks[i]->cmdVolume = volume;
//             }
//             break;
//         }
//         case SOUND_CMD_SET_TRACK_BEND:
//         {
//             u8 range = (a & 0xFF00) >> 8;
//             u8 bend = a;
//
//             for (; cmd->b != 0 && i < 10; i++, cmd->b >>= 1)
//             {
//                 if ((cmd->b & 1) && player->tracks[i] != NULL)
//                 {
//                     player->tracks[i]->bendRange = range;
//                     player->tracks[i]->bend = bend;
//                 }
//             }
//             break;
//         }
//         case SOUND_CMD_SET_TRACK_PAN:
//         {
//             u8 pan = a;
//
//             for (; cmd->b != 0 && i < 10; i++, cmd->b >>= 1)
//             {
//                 if ((cmd->b & 1) && player->tracks[i] != NULL)
//                     player->tracks[i]->pan = pan;
//             }
//             break;
//         }
//         }
//     }
INCLUDE_ASM(SoundCmdTracks);

// Commands 200-201: a = the argument, b = mask of players (shifted out).
void SoundCmdPlayers(struct SoundCmd *cmd)
{
    s32 i = 0;

    switch (cmd->cmd)
    {
    case SOUND_CMD_FADE_OUT_PLAYERS:
        for (; cmd->b != 0 && i < 20; i++, cmd->b >>= 1)
        {
            if (cmd->b & 1)
                PlayerFadeOut(i, cmd->a);
        }
        break;
    case SOUND_CMD_PAUSE_PLAYERS:
        for (; cmd->b != 0 && i < 20; i++, cmd->b >>= 1)
        {
            if (cmd->b & 1)
                PlayerSetPause(i, cmd->a);
        }
        break;
    }
}

void SoundCmdGlobal(struct SoundCmd *cmd)
{
    switch (cmd->cmd)
    {
    case SOUND_CMD_CALL:
        ((void (*)(u32))cmd->a)(cmd->b);
        break;
    case SOUND_CMD_SET_SEQ_HOOK:
        gSoundSeqHook = (SoundSeqHook)cmd->a;
        break;
    case SOUND_CMD_SET_NOTE_HOOK:
        gSoundNoteHook = (SoundNoteHook)cmd->a;
        break;
    case SOUND_CMD_NOP:
        SoundCmdNop(cmd->a);
        break;
    case SOUND_CMD_SET_PCM_VOICES:
        SetPcmVoiceCount(cmd->a);
        break;
    }
}


void SoundCmdProcess(void)
{
    struct SoundCmd *cmd = SoundCmdPop();

    while (cmd != NULL)
    {
        gSoundCmdHandlers[cmd->cmd >> 8](cmd);
        cmd = SoundCmdPop();
    }
}
