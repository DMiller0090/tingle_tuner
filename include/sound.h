#ifndef GUARD_SOUND_H
#define GUARD_SOUND_H

#include "global.h"

// Sound IDs (SoundPlay: gSoundTable index). IDs 0-6 are voice clips on player 4; the US
// table drops ID 6 (and its clip), so the IDs from 7 on are one lower.
#ifdef VERSION_GZLJ01
#define SOUND_ID_SHIFT 0
#else
#define SOUND_ID_SHIFT 1
#endif
#define SE_RUPEE_COUNT_END  (9 - SOUND_ID_SHIFT)
#define SE_TITLE            (0x16 - SOUND_ID_SHIFT)  // Mode0State0
#define SE_LINKED           (0x1F - SOUND_ID_SHIFT)  // Mode0State3: all boot data received
#define SE_KEY_BLOCKED      (0x1C - SOUND_ID_SHIFT)  // key pressed while the screen waits
#define SE_RUPEE_COUNT_UP   (0x1D - SOUND_ID_SHIFT)
#define SE_RUPEE_COUNT_DOWN (0x1E - SOUND_ID_SHIFT)

void SoundInit(void);
void SoundMain(void);
void SoundVSync(void);
void SoundVSyncProc(void);
void SoundDmaStop(void);
void SoundPlay(u16 id, void *params);
// Loop sound gSoundLoopTable[id] for `owner`; call it every frame to keep it playing.
// Reads params[1]'s low half (as an s16, a tempo step) when id is 9.
void SoundPlayLoop(u32 owner, u16 id, u32 *params);

#endif // GUARD_SOUND_H
