#ifndef GUARD_GBA_SYSCALL_H
#define GUARD_GBA_SYSCALL_H

#include "global.h"

// libagbsyscall

#define CPU_SET_SRC_FIXED 0x01000000
#define CPU_SET_16BIT     0x00000000
#define CPU_SET_32BIT     0x04000000

void CpuSet(const void *src, void *dest, u32 control);
void CpuFastSet(const void *src, void *dest, u32 control);
void LZ77UnCompVram(const void *src, void *dest);
void LZ77UnCompWram(const void *src, void *dest);

#define CpuCopy16(src, dest, size) CpuSet(src, dest, CPU_SET_16BIT | (((size) / 2) & 0x1FFFFF))
#define CpuCopy32(src, dest, size) CpuSet(src, dest, CPU_SET_32BIT | (((size) / 4) & 0x1FFFFF))

#define CpuFastCopy(src, dest, size) CpuFastSet(src, dest, ((size) / 4) & 0x1FFFFF)

// SWI 0x26 HardReset, only if no cartridge is inserted and RCNT is in JOY bus mode
void JoyHardReset(void);

#define CpuFill16(value, dest, size)                                                          \
{                                                                                             \
    vu16 _tmp = (vu16)(value);                                                                \
    CpuSet((void *)&_tmp, dest, CPU_SET_16BIT | CPU_SET_SRC_FIXED | (((size) / 2) & 0x1FFFFF)); \
}

#define CpuFill32(value, dest, size)                                                          \
{                                                                                             \
    vu32 _tmp = (vu32)(value);                                                                \
    CpuSet((void *)&_tmp, dest, CPU_SET_32BIT | CPU_SET_SRC_FIXED | (((size) / 4) & 0x1FFFFF)); \
}

#endif // GUARD_GBA_SYSCALL_H
