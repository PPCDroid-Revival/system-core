
/* MIPS assembler acceleration.
*/
#include <machine/asm.h>
#include <sys/regdef.h>

/* scanline - needs [03545404:00000077:00000a01:00000000]
*/
LEAF(scanline_mips_accel1)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lh	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lh	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute texture coordinates
*/
	lw	t4, 0x17c(a0)
	sra	t7, t4, 16
	lw	t3, 0x18c(a0)
	addu	a2, a2, t7
	sra	t3, t3, 16
	addu	a3, a3, t3
	lh	t4, 0x3c8(a0)
	mul	t7, a3, t4
	lw	t3, 0x3cc(a0)
	addu	a2, a2, t7
	sll	t7, a2, 2
	addu	t4, t3, t7
	lui	t2, 0x0001		/* Constant used to update counter */
	lui	t3, 0x00ff		/* Constant blue mask */

.L1fragment_loop:
	lw	a2, 0(t4)		/* fetch texel */
	lhu	a3, 0(t5)		/* fetch color-buffer */
	srl	v0, a2, 24		/* alpha */
	andi	v1, a2, 0xff		/* red */
	addiu	t4, t4, 4

/* blending
*/
	srl	t0, a3, 11
	srl	t7, v0, 7
	addu	v0, v0, t7
	srl	t6, v1, 3
	subu	v1, t6, t0
	mul	v1, v1, v0
	sll	t7, t0, 8
	addu	v1, v1, t7
	srl	t1, v1, 8
	sll	t1, t1, 11

	andi	v1, a2, 0xff00		/* green */
	srl	v1, v1, 8

/* blending
*/
	andi	t0, a3, 0x07e0
	srl	t0, t0, 5
	srl	t6, v1, 2
	subu	v1, t6, t0
	mul	v1, v1, v0
	sll	t6, t0, 8
	addu	v1, v1, t6
	srl	v1, v1, 8
	sll	t6, v1, 5
	or	t1, t1, t6

	and	v1, a2, t3		/* blue */
	srl	v1, v1, 16

/* blending
*/
	andi	t0, a3, 0x001f
	srl	t6, v1, 3
	subu	v1, t6, t0
	mul	v1, v1, v0
	sll	t7, t0, 8
	addu	v1, v1, t7
	srl	t6, v1, 8
	or	t1, t1, t6

/* store
*/
	sh	t1, 0(t5)
	sub	a1, a1, t2
	bgez	a1, .L1fragment_loop
	addiu	t5, t5, 2
	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel1)


/* scanline - needs [03545404:00000077:00000a04:00000000]
*/
LEAF(scanline_mips_accel2)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lh	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lh	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute texture coordinates
*/
	lw	t4, 0x17c(a0)
	sra	t7, t4, 16
	lw	t3, 0x18c(a0)
	addu	a2, a2, t7
	sra	t3, t3, 16
	lw	t4, 0x3c8(a0)
	addu	a3, a3, t3
	mul	t7, a3, t4
	lw	t3, 0x3cc(a0)
	addu	a2, a2, t7
	sll	t7, a2, 1
	addu	t4, t3, t7
	lui	t2, 0x0001		/* Constant used to update counter */

/* load initial iterated color (8888 packed)
*/
	lw	a3, 0x464(a0)
.L2fragment_loop:
	lhu	a2, 0(t4)		/* fetch texel */
	lhu	t1, 0(t5)		/* fetch color-buffer */
	addiu	t4, t4, 2
	srl	v1, a3, 24		/* alpha */

	srl	t0, a2, 11		/* red */

/* blending
*/
	srl	t3, t1, 11
	srl	t7, v1, 7
	addu	v1, v1, t7
	subu	t0, t0, t3
	mul	t0, t0, v1
	sll	t7, t3, 8
	addu	t0, t0, t7
	srl	v0, t0, 8
	sll	v0, v0, 11

	andi	t0, a2, 0x07e0		/* green */
	srl	t0, t0, 5
/* blending
*/
	andi	t3, t1, 0x07e0
	srl	t3, t3, 5
	subu	t0, t0, t3
	mul	t0, t0, v1
	sll	t7, t3, 8
	addu	t0, t0, t7
	srl	t0, t0, 8
	sll	t7, t0, 5
	or	v0, v0, t7

	andi	t0, a2, 0x1f		/* blue */
/* blending
*/
	andi	t3, t1, 0x1f
	subu	t0, t0, t3
	mul	t0, t0, v1
	sll	t7, t3, 8
	addu	t0, t0, t7
	srl	t7, t0, 8
	or	v0, v0, t7

/* store
*/
	sh	v0, 0(t5)
	subu	a1, a1, t2
	bgez	a1, .L2fragment_loop
	addiu	t5, t5, 2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel2)

/* scanline - needs [03010104:00000077:00000004:00000000]
*/
LEAF(scanline_mips_accel3)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lw	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lw	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute texture coordinates
*/
	lw	t4, 0x3d0(a0)
	lw	v0, 0x17c(a0)
	lw	t1, 0x3d4(a0)
	lw	v1, 0x18c(a0)
	mul	t6, a2, t4
	addu	t4, t6, v0
	mul	t7, a2, t1
	addu	t1, t7, v1
	lui	t2, 0x0001		/* Constant used to update counter */

.L3fragment_loop:
/* compute repeat/clamp
*/
	lw	v0, 0x3c0(a0)
	lw	v1, 0x3c4(a0)
	sra	a2, t4, 16
	sra	t7, a2, 31
	nor	t7, zero, t7
	and	a2, a2, t7
	slt	t6, a2, v0
	bnez	t6, 1f
	nop
	addiu	a2, v0, -1
1:
	sra	a3, t1, 16
	sra	t7, a3, 31
	nor	t7, zero, t7
	and	a3, a3, t7
	slt	t6, a3, v1
	bnez	t6, 2f
	nop
	addiu	a3, v1, -1
2:
/* iterate s,t
*/
	lw	v0, 0x3d0(a0)
	lw	v1, 0x3d4(a0)
	addu	t4, t4, v0
	addu	t1, t1, v1

/* merge base & offset
*/
	lw	v1, 0x3c8(a0)
	lw	v0, 0x3cc(a0)
	mul	t6, a3, v1
	addu	a2, a2, t6
	sll	t7, a2, 1
	addu	v0, v0, t7
	lhu	v0, 0(v0)		/* fetch texel */

/* store
*/
	sh	v0, 0(t5)
	sub	a1, a1, t2
	bgez	a1, .L3fragment_loop
	addiu	t5, t5, 2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel3)

/* scanline - needs [03545404:00000077:00000004:00000000]
*/
LEAF(scanline_mips_accel4)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lh	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lw	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute texture coordinates
*/
	lh	t4, 0x3d0(a0)
	lw	v0, 0x17c(a0)
	lw	t1, 0x3d4(a0)
	lw	v1, 0x18c(a0)
	mul	t6, a2, t4
	addu	t4, t6, v0
	mul	t7, a2, t1
	addu	t1, t7, v1
	lui	t2, 0x0001		/* Constant used to update counter */

/* load initial iterated color (8888 packed)
*/
	lw	a3, 0x464(a0)

.L4fragment_loop:
/* compute repeat/clamp
*/
	lw	v1, 0x3c0(a0)
	lw	t0, 0x3c4(a0)
	sra	a2, t4, 16
	sra	t7, a2, 31
	nor	t7, zero, t7
	and	a2, a2, t7
	slt	t6, a2, v1
	bnez	t6, 1f
	nop
	addiu	a2, v1, -1
1:
	sra	v0, t1, 16
	sra	t7, v0, 31
	nor	t7, zero, t7
	and	v0, v0, t7
	slt	t6, v0, t0
	bnez	t6, 2f
	nop
	addiu	v0, t0, -1
2:
/* iterate s,t
*/
	lw	v1, 0x3a0(a0)
	lw	t0, 0x3d4(a0)
	addu	t4, t4, v1
	addu	t1, t1, t0

/* merge base & offset
*/
	lw	t0, 0x3c8(a0)
	lw	v1, 0x3cc(a0)
	mul	t7, v0, t0
	addu	a2, a2, t7
	sll	t6, a2, 1
	addu	v1, v1, t6
	lhu	v1, 0(v1)		/* fetch texel */
	lhu	a2, 0(t5)		/* fetch color-buffer */
	srl	t0, a3, 24		/* alpha */
	srl	t3, v1, 11		/* red */

/* blending
*/
	srl	t6, a2, 11
	srl	t7, t0, 7
	addu	t0, t0, t7
	subu	t3, t3, t6
	mul	t3, t3, t0
	sll	t7, t6, 8
	addu	t3, t3, t7
	srl	v0, t3, 8
	sll	v0, v0, 11

	andi	t3, v1, 0x07e0		/* green */
	srl	t3, t3, 5

/* blending
*/
	andi	t6, a2, 0x07e0
	srl	t6, t6, 5
	subu	t3, t3, t6
	mul	t3, t3, t0
	sll	t7, t6, 8
	addu	t3, t3, t7
	srl	t3, t3, 8
	sll	t7, t3, 5
	or	v0, v0, t3

	andi	t3, v1, 0x001f		/* blue */

/* blending
*/
	andi	t6, a2, 0x001f
	subu	t3, t3, t6
	mul	t3, t3, t0
	sll	t7, t6, 8
	addu	t3, t3, t7
	srl	t6, t3, 8
	or	v0, v0, t6

/* store
*/
	sh	v0, 0(t5)
	sub	a1, a1, t2
	bgez	a1, .L4fragment_loop
	addiu	t5, t5, 2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel4)

/* scanline - needs [03515104:00000077:00001a01:00000000]
*/
LEAF(scanline_mips_accel5)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lh	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lh	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute texture coordinates
*/
	lw	t4, 0x17c(a0)
	sra	t7, t4, 16
	lw	t3, 0x18c(a0)
	addu	a2, a2, t7
	sra	t3, t3, 16
	addu	a3, a3, t3
	lh	t4, 0x3c8(a0)
	mul	t7, a3, t4
	lw	t3, 0x3cc(a0)
	addu	a2, a2, t7
	sll	t7, a2, 2
	addu	t4, t3, t7
	lui	t2, 0x0001		/* Constant used to update counter */
	lui	t8, 0x00ff		/* Constant blue mask */

/* load initial iterated color (8888 packed)
*/
	lw	a3, 0x464(a0)

.L5fragment_loop:
	lw	a2, 0(t4)		/* fetch texel */
	addiu	t4, t4, 4
	lhu	t1, 0(t5)		/* fetch color-buffer */

/* alpha
*/
	srl	v1, a3, 24
	srl	t0, a2, 24
	srl	t7, v1, 7
	addu	v1, v1, t7
	mul	v1, t0, v1
	srl	v1, v1, 8

	andi	t0, a3, 0x00ff		/* red */
	andi	t3, a2, 0x00ff
	srl	t7, t0, 7
	addu	t0, t0, t7
	mul	t0, t3, t0

/* blending
*/
	srl	t3, t1, 11
	srl	t0, t0, 8
	srl	t7, v1, 7
	addu	v1, v1, t7
	li	t7, 0x0100
	subu	v1, t7, v1
	mul	t6, t3, v1
	sll	t7, t0, 5
	addu	t0, t6, t7
	srl	v0, t0, 8
	sll	v0, v0, 11

	andi	t0, a3, 0xff00		/* green */
	srl	t0, t0, 8
	andi	t3, a2, 0xff00
	srl	t3, t3, 8
	srl	t7, t0, 7
	addu	t0, t0, t7
	mul	t0, t3, t0

/* blending
*/
	andi	t3, t1, 0x07e0
	srl	t3, t3, 5
	srl	t0, t0, 8
	mul	t6, t3, v1
	sll	t7, t0, 6
	addu	t0, t6, t7
	srl	t0, t0, 8
	sll	t7, t0, 5
	or	v0, v0, t7

	and	t0, a3, t8		/* blue */
	srl	t0, t0, 16
	and	t3, a2, t8
	srl	t3, t3, 16
	srl	t7, t0, 7
	addu	t0, t0, t7
	mul	t0, t3, t0

/* blending
*/
	andi	t3, t1, 0x001f
	srl	t0, t0, 8
	mul	t6, t3, v1
	sll	t7, t0, 5
	addu	t0, t6, t7
	srl	t0, 8
	or	v0, v0, t0

/* store
*/
	sh	v0, 0(t5)
	sub	a1, a1, t2
	bgez	a1, .L5fragment_loop
	addiu	t5, t5, 2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel5)

/* scanline - needs [03515104:00000177:00000a01:00000000]
*/
LEAF(scanline_mips_accel6)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lw	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	andi	t4, a2, 0x0007
	andi	t5, a3, 0x0007
	sll	t7, t5, 3
	addu	t4, t4, t7
	sll	t7, a1, 16
	or	a1, t4, t7

/* compute color-buffer pointer
*/
	lw	t4, 0x00b8(a0)
	lw	t5, 0x00bc(a0)
	mul	t7, a3, t4
	addu	t4, t7, a2
	sll	t7, t4, 1
	addu	t5, t5, t7

/* compute texture coordinates
*/
	lw	t4, 0x17c(a0)
	sra	t7, t4, 16
	addu	a2, a2, t7
	lw	t4, 0x18c(a0)
	sra	t7, t4, 16
	addu	a3, a3, t7
	lw	t4, 0x3c8(a0)
	mul	t7, a3, t4
	addu	a2, t7, a2
	lw	t4, 0(a0)
	sll	t7, a2, 2
	addu	t4, t4, t7
	lui	t2, 0x0001		/* Constant used to update counter */
	lui	t8, 0x00ff		/* Constant blue mask */

.L6fragment_loop:
	ror	a1, a1, 3
	lui	t7, 0x2000
	addu	a1, a1, t7
	ror	a1, a1, 29

	lw	a2, 0(t4)		/* fetch texel */
	addu	t4, t4, 4

/* fetch color-buffer
*/
	lhu	a3, 0(t5)
	andi	t1, a1, 0x003f
	addu	t1, t1, a0
	lbu	t1, 0x420(t1)
	srl	v1, a2, 24		/* alpha */

	andi	t0, a2, 0x00ff		/* red */

/* blending
*/
	srl	t3, a3, 11
	sll	t3, t3, 3
	srl	t7, t3, 5
	or	t3, t3, t7
	srl	t7, v1, 7
	addu	v1, v1, t7
	lui	t7, 0x0100
	subu	v1, t7, v1
	sll	t7, t0, 8
	or	t6, t0, t7
	mul	t7, t3, v1
	addu	t0, t7, t6

/* dithering
*/
	srl	t7, t0, 5
	subu	v0, t0, t7
	sll	t7, t1, 5
	addu	v0, v0, t7
	srl	v0, v0, 11
	sll	v0, v0, 11

/* green
*/
	andi	t0, a0, 0xff00
	srl	t0, t0, 8

/* blending
*/
	andi	t3, a3, 0x07e0
	srl	t3, t3, 5
	sll	t3, t3, 2
	srl	t7, t3, 6
	or	t3, t3, t7
	sll	t7, t0, 8
	or	t6, t0, t7
	mul	t7, t3, v1
	addu	t0, t7, t6

/* dithering
*/
	srl	t7, t0, 6
	subu	t0, t0, t7
	sll	t7, t1, 4
	addu	t0, t0, t7
	srl	t0, t0, 10
	sll	t7, t0, 5

/* blue
*/
	and	t0, a2, t8
	srl	t0, t0, 16

/* blending
*/
	andi	t3, a3, 0x001f
	sll	t3, t3, 3
	srl	t7, t3, 5
	or	t3, t3, t7
	sll	t7, t0, 8
	or	t6, t0, t7
	mul	t7, t3, v1
	addu	t0, t7, t6

/* dithering
*/
	srl	t7, t0, 5
	subu	t0, t0, t7
	sll	t7, t1, 5
	addu	t0, t0, t7
	srl	t7, t0, 11
	or	v0, v0, t7

/* store
*/
	sh	v0, 0(t5)
	sub	a1, a1, t2
	bgez	a1, .L6fragment_loop
	addiu	t5, t5, 2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel6)

/* scanline - needs [03010104:00000097:00000000:00000000]
*/
LEAF(scanline_mips_accel7)
	.set	noreorder
	/* compute ct (# of pixels to process)
	*/
	lw 	a2, 0x344(a0)
	lw	a1, 0x348(a0)
	lw	a3, 0x34c(a0)
	subu	a1, a1, a2
	addiu	a1, a1, -1
	sll	a1, a1, 16

/* compute color-buffer pointer
*/
	lw	t4, 0xb8(a0)
	lw	t5, 0xbc(a0)
	mul	t6, a3, t4
	addu	t4, t6, a2
	sll	t6, t4, 1
	addu	t5, t5, t6

/* compute initial Z coordinate
*/
	lw	t1, 0x3a4(a0)
	lw	t4, 0x360(a0)
	mul	t7, a2, t1
	addu	t4, t7, t4
	lw	t1, 0x100(a0)
	lw	v0, 0x10c(a0)
	mul	t7, a3, t1
	addu	t1, t7, a2
	srl	t7, a1, 16
	addu	t1, t1, t7
	sll	t7, t1, 1
	addu	v0, v0, t7
	sw	v0, 0x3a8(a0)

/* load initial iterated color (dest format packed)
*/
	lw	a3, 0x460(a0)
	lui	t2, 0x0001		/* Constant used to update counter */
	srl	a3, a3, 16

.L7fragment_loop:
/* Depth Test
*/
	lw	a2, 0x3a8(a0)
	srl	t7, a1, 15
	subu	a2, a2, t7
	lhu	t1, 0(a2)
	srl	t7, t4, 16
	slt	t6, t1, t7
	bnez	t6, 1f
	nop
	beq	t1, t7, 2f
	nop
	sh	t7, 0(a2)
2:
/* store
*/
	sh	a3, 0(t5)
	addiu	t5, t5, 2
	lw	a2, 0x3a4(a0)

	sub	a1, a1, t2
	bgez	a1, .L7fragment_loop
	addu	t4, t4, a2

	jr	ra
	nop
1:
/* discard_after_textures:
*/
	lw	a2, 0x3a4(a0)
	addiu	t5, t5, 2
	sub	a1, a1, t2
	bgez	a1, .L7fragment_loop
	addu	t4, t4, a2

	jr	ra
	nop
	.set	reorder
END(scanline_mips_accel7)

