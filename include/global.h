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

// PROG_BASE, where the packer unpacks the program, comes from config/versions.yml
// (prog_base); tools/build.py passes it with the VERSION_* macros.

#define REG_DISPCNT  (*(vu16 *)0x04000000)
#define REG_DISPSTAT (*(vu16 *)0x04000004)
#define REG_BG0CNT   (*(vu16 *)0x04000008)
#define REG_BG1CNT   (*(vu16 *)0x0400000A)
#define REG_BG2CNT   (*(vu16 *)0x0400000C)
#define REG_BG3CNT   (*(vu16 *)0x0400000E)
#define REG_BG0HOFS  (*(vu16 *)0x04000010)
#define REG_BG0VOFS  (*(vu16 *)0x04000012)
#define REG_BG1HOFS  (*(vu16 *)0x04000014)
#define REG_BG1VOFS  (*(vu16 *)0x04000016)
#define REG_BG2HOFS  (*(vu16 *)0x04000018)
#define REG_BG2VOFS  (*(vu16 *)0x0400001A)
#define REG_BG3HOFS  (*(vu16 *)0x0400001C)
#define REG_BG3VOFS  (*(vu16 *)0x0400001E)
#define REG_WIN0H    (*(vu16 *)0x04000040)
#define REG_WIN1H    (*(vu16 *)0x04000042)
#define REG_WIN0V    (*(vu16 *)0x04000044)
#define REG_WIN1V    (*(vu16 *)0x04000046)
#define REG_WININ    (*(vu16 *)0x04000048)
#define REG_WINOUT   (*(vu16 *)0x0400004A)
#define REG_MOSAIC   (*(vu16 *)0x0400004C)
#define REG_BLDCNT   (*(vu16 *)0x04000050)
#define REG_BLDALPHA (*(vu16 *)0x04000052)
#define REG_BLDY     (*(vu16 *)0x04000054)
#define REG_DMA0SAD  (*(vu32 *)0x040000B0)
#define REG_DMA0DAD  (*(vu32 *)0x040000B4)
#define REG_DMA0CNT  (*(vu32 *)0x040000B8)
#define REG_DMA0CNT_H (*(vu16 *)0x040000BA)
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
#define OAM          0x07000000
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
#define DMA_ENABLE           0x80000000
#define DMA_START_HBLANK     0x20000000
#define DMA_REPEAT           0x02000000
#define DMA_16BIT            0x00000000
#define DMA_DEST_RELOAD      0x00600000
#define DMA_START_MASK       0x30000000
#define DMA_DREQ_ON          0x08000000

// Keep a not-yet-decompiled function as asm inside a C file: it is taken from
// build/<ver>/asm/funcs/<name>.s, which disasm.py writes for every function in a C range.
// It is wrapped in .pushsection so that it lands in .text even where agbcc has just
// emitted .rodata (e.g. right after a const table). Every function starts 4-aligned, as
// agbcc's own do; a C function before it may end on a halfword.
#define INCLUDE_ASM(name) asm(".pushsection .text\n.align 2, 0\n.include \"funcs/" #name ".s\"\n.popsection")

#endif // GUARD_GLOBAL_H
