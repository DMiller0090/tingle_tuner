#ifndef GUARD_GBA_SYSCALL_H
#define GUARD_GBA_SYSCALL_H

#include "global.h"

// libagbsyscall

#define CPU_SET_SRC_FIXED 0x01000000
#define CPU_SET_16BIT     0x00000000
#define CPU_SET_32BIT     0x04000000

void CpuSet(const void *src, void *dest, u32 control);
void CpuFastSet(const void *src, void *dest, u32 control);

#define CpuCopy16(src, dest, size) CpuSet(src, dest, CPU_SET_16BIT | (((size) / 2) & 0x1FFFFF))
#define CpuCopy32(src, dest, size) CpuSet(src, dest, CPU_SET_32BIT | (((size) / 4) & 0x1FFFFF))

#endif // GUARD_GBA_SYSCALL_H
