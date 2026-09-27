#include "global.h"
#include "sound.h"

// The sound driver (GZLJ01 02020E28-020237CC): the API the game calls, then the sequencer,
// PCM mixer front end, GB channels and the command queue. See docs/layout.md "Sound driver".

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

INCLUDE_ASM(SoundCmdPush);

INCLUDE_ASM(SoundCmdQueueInit);

INCLUDE_ASM(SoundCmdPop);

INCLUDE_ASM(SoundCmdFlush);

INCLUDE_ASM(SoundCmdStartSong);

INCLUDE_ASM(SoundCmdStartSe);

INCLUDE_ASM(SoundCmdFadeOut);

INCLUDE_ASM(SoundCmdPause);

INCLUDE_ASM(SoundCmdFadeOutPlayers);

INCLUDE_ASM(SoundCmdPausePlayers);

INCLUDE_ASM(SoundCmdSetTempo);

INCLUDE_ASM(SoundCmdSetVolume);

INCLUDE_ASM(SoundCmdSetFlags);

INCLUDE_ASM(SoundCmdMuteTracks);

INCLUDE_ASM(SoundCmdSetTrackVolume);

INCLUDE_ASM(SoundCmdSetTrackBend);

INCLUDE_ASM(SoundCmdSetTrackPan);

INCLUDE_ASM(SoundCmdNop300);

INCLUDE_ASM(SoundCmdSetPcmVoices);

INCLUDE_ASM(SoundCmdCall);

INCLUDE_ASM(SoundCmdSetSeqHook);

INCLUDE_ASM(SoundCmdSetNoteHook);

INCLUDE_ASM(SoundCmdPlayer);

INCLUDE_ASM(SoundCmdTracks);

INCLUDE_ASM(SoundCmdPlayers);

INCLUDE_ASM(SoundCmdGlobal);

INCLUDE_ASM(SoundCmdProcess);
