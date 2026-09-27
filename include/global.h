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

#endif // GUARD_GLOBAL_H
