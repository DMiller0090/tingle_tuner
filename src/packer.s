@ Tingle Tuner client: outer multiboot image ("packer").
@
@ The GameCube JOY-boots this image to 0x02000000. The stub copies the raw tail up to
@ TAIL_DEST, LZ77-decompresses the program to PROG_BASE and jumps to it. Identical code
@ in every version; only the literal pool (layout) differs.
@
@ Assembled with -I build/<ver> --defsym PROG_BASE=... --defsym TAIL_DEST=...

	.syntax unified
	.cpu arm7tdmi
	.arm
	.section .text

REG_BASE	= 0x04000130
RCNT		= 0x04	@ 0x04000134
JOYCNT		= 0x10	@ 0x04000140
JOY_RECV	= 0x20	@ 0x04000150
JOY_TRANS	= 0x24	@ 0x04000154
JOYSTAT		= 0x28	@ 0x04000158

	.global _start
_start:
	b	mb_entry
	.incbin	"header.bin"		@ 0x004: logo, title, game code (0x0AC), checksum
header_game_code = _start + 0xAC

mb_entry:				@ 0x0C0: normal multiboot entry
	b	unpack
	.space	0x1C			@ 0x0C4: multiboot boot mode / client number, reserved

joy_entry:				@ 0x0E0: JOY bus boot entry
	ldr	r3, pool_reg_base
	mov	r1, #0
	strh	r1, [r3, #JOYCNT]
	mov	r1, #0xC0
	lsl	r1, r1, #8
	strh	r1, [r3, #RCNT]		@ RCNT = 0xC000: JOY bus mode
1:	ldrh	r1, [r3, #JOYSTAT]
	ands	r1, r1, #8
	bne	1b
	ldr	r2, pool_game_code
	ldr	r1, [r2]
	str	r1, [r3, #JOY_TRANS]	@ send our game code
2:	ldrh	r0, [r3, #JOYCNT]
	and	r0, r0, #7
	cmp	r0, #4			@ wait for the GC to read it
	bne	2b
	strh	r0, [r3, #JOYCNT]
3:	ldrh	r0, [r3, #JOYCNT]
	and	r0, r0, #7
	cmp	r0, #2			@ wait for the GC to write back
	bne	3b
	strh	r0, [r3, #JOYCNT]
	ldr	r0, [r3, #JOY_RECV]
	cmp	r0, r1
4:	bne	4b			@ hang unless it echoed our game code
	mov	r1, #0
	str	r1, [r3, #JOY_TRANS]

unpack:
	ldr	r0, pool_tail_src
	ldr	r1, pool_tail_dest
	ldr	r2, pool_tail_end
	sub	r2, r2, r0
	lsr	r2, r2, #2
	orr	r2, r2, #0		@ CpuSet mode 0 (16-bit units, count = words)
	svc	#0x0C << 16		@ CpuSet
	mov	r1, #0
	lsl	r1, r1, #8
	strh	r1, [r3, #RCNT]
	ldr	r0, pool_lz_src
	ldr	r1, pool_prog_base
	svc	#0x11 << 16		@ LZ77UnCompWram
	ldr	lr, pool_prog_base
	bx	lr

pool_reg_base:	.4byte	REG_BASE
pool_game_code:	.4byte	header_game_code
pool_tail_src:	.4byte	tail_src
pool_tail_dest:	.4byte	TAIL_DEST
pool_tail_end:	.4byte	tail_end
pool_lz_src:	.4byte	lz_src
pool_prog_base:	.4byte	PROG_BASE

lz_src:
	.incbin	"prog.lz"
	.balign	4, 0
tail_src:
	.incbin	"tail.bin"
tail_end:
