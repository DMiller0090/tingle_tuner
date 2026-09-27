#ifndef GUARD_GLOBAL_H
#define GUARD_GLOBAL_H

// Built with agbcc (GCC 2.9-arm-000512), the Nintendo AGB SDK compiler.

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef u8 bool8;
typedef u32 bool32;

#define TRUE 1
#define FALSE 0
#define NULL ((void *)0)

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define IWRAM_START 0x03000000

#define REG_DISPSTAT (*(vu16 *)0x04000004)
#define REG_KEYINPUT (*(vu16 *)0x04000130)
#define REG_RCNT     (*(vu16 *)0x04000134)
#define REG_IE       (*(vu16 *)0x04000200)
#define REG_WAITCNT  (*(vu16 *)0x04000204)
#define REG_IME      (*(vu16 *)0x04000208)
#define INTR_VECTOR  (*(void **)0x03007FFC)

#define KEYS_MASK            0x03FF
#define INTR_FLAG_VBLANK     0x0001
#define DISPSTAT_VBLANK_INTR 0x0008

// Keep a not-yet-decompiled function as asm inside a C file: it is taken from
// build/<ver>/asm/funcs/<name>.s, which disasm.py writes for every function in a C range.
#define INCLUDE_ASM(name) asm(".include \"funcs/" #name ".s\"")

#endif // GUARD_GLOBAL_H
