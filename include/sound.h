#ifndef GUARD_SOUND_H
#define GUARD_SOUND_H

#include "global.h"

// Sound IDs (SoundPlay). The US sound table has one entry fewer somewhere below 7, so the
// IDs from there on are one lower (7 and up, at least).
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
// Reads params[1]'s low half (as an s16) when id is 9.
void Func_02021284(u32 a, u16 id, u32 *params);

#endif // GUARD_SOUND_H
