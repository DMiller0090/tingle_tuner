#ifndef GUARD_GLOBAL_H
#define GUARD_GLOBAL_H

// Built with agbcc (GCC 2.9-arm-000512), the Nintendo AGB SDK compiler.

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef volatile u8 vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;
typedef u8 bool8;
typedef u32 bool32;

#define TRUE 1
#define FALSE 0
#define NULL ((void *)0)

#define ARRAY_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#define IWRAM_START 0x03000000

#define REG_DISPSTAT (*(vu16 *)0x04000004)
#define REG_TM3CNT   (*(vu32 *)0x0400010C)
#define REG_TM3CNT_H (*(vu16 *)0x0400010E)
#define REG_SIOMULTI0 (*(vu16 *)0x04000120)
#define REG_SIOCNT   (*(vu16 *)0x04000128)
#define REG_SIOMLT_SEND (*(vu16 *)0x0400012A)
#define REG_KEYINPUT (*(vu16 *)0x04000130)
#define REG_RCNT     (*(vu16 *)0x04000134)
#define REG_JOYCNT   (*(vu16 *)0x04000140)
#define REG_JOY_RECV (*(vu32 *)0x04000150)
#define REG_JOY_TRANS (*(vu32 *)0x04000154)
#define REG_JOYSTAT  (*(vu16 *)0x04000158)
#define REG_IE       (*(vu16 *)0x04000200)
#define REG_IF       (*(vu16 *)0x04000202)
#define REG_WAITCNT  (*(vu16 *)0x04000204)
#define REG_IME      (*(vu16 *)0x04000208)
#define INTR_CHECK   (*(vu16 *)0x03007FF8)  // BIOS IntrWait flags
#define INTR_VECTOR  (*(void **)0x03007FFC)

#define KEYS_MASK            0x03FF
#define INTR_FLAG_VBLANK     0x0001
#define INTR_FLAG_TIMER3     0x0040
#define INTR_FLAG_SERIAL     0x0080
#define TIMER_INTR_ENABLE    0x0040
#define TIMER_ENABLE         0x0080
#define SIO_115200_BPS       0x0003
#define SIO_SI               0x0004
#define SIO_SD               0x0008
#define SIO_ID_MASK          0x0030
#define SIO_START            0x0080
#define SIO_MULTI_MODE       0x2000
#define SIO_IF_ENABLE        0x4000
#define RCNT_JOY_BUS         0xC000
#define JOYCNT_RESET         0x0001  // JOY reset command received
#define JOYCNT_RECV          0x0002  // GC wrote JOY_RECV
#define JOYCNT_SEND          0x0004  // GC read JOY_TRANS
#define JOYCNT_ACK_ALL       0x0007  // reset / receive / send complete (write 1 to clear)
#define JOYCNT_INTR          0x0040
#define JOYSTAT_SEND         0x0008  // JOY_TRANS not read by the GC yet
#define JOYSTAT_GP_FLAG0     0x0010  // general-purpose flags the GC sees in its status byte
#define JOYSTAT_GP_FLAG1     0x0020
#define DISPSTAT_VBLANK_INTR 0x0008

// Keep a not-yet-decompiled function as asm inside a C file: it is taken from
// build/<ver>/asm/funcs/<name>.s, which disasm.py writes for every function in a C range.
#define INCLUDE_ASM(name) asm(".include \"funcs/" #name ".s\"")

#endif // GUARD_GLOBAL_H
