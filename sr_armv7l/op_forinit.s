#################################################################
#								#
# Copyright (c) 2017-2020 YottaDB LLC and/or its subsidiaries.	#
# All rights reserved.						#
#								#
# Copyright (c) 2017 Stephen L Johnson. All rights reserved.	#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

/* op_forinit.s */

	/* r0 - initial value */
	/* r1 - increment */
	/* r2 - final value */

	.title	op_forinit.s

	.include "linkage.si"
	.include "g_msf.si"
	.include "mval_def.si"
#	include "debug.si"

	.sbttl	op_forinit

	.data
	.extern	frame_pointer

	.text
	.extern	numcmp
	.extern	mval2num

arg2_save	= -12
arg0_save	=  -4
FRAME_SIZE	=  16					/* 16 bytes of save area */

ENTRY op_forinit
	push	{fp, lr}
	ldr	r12, [r5]
	str	r6, [r12, #msf_ctxt_off]
	mov	fp, sp
	sub	sp, #FRAME_SIZE				/* Allocate save area */
	CHKSTKALIGN					/* Verify stack alignment */
	str	r0, [fp, #arg0_save]			/* Save args to avoid getting modified across function calls */
	str	r2, [fp, #arg2_save]
	mov	r0, r1					/* Copy 2nd argument as 1st argument for mval2num call */
	bl	mval2num				/* Convert to a numeric. Issue ZYSQLNULLNOTVALID error if needed */
	ldr	r12, [r0, #mval_l_m1]			/* r0 holds potentially updated `mval *` value from mval2num() call */
	cmp	r12, #0
	bmi	l2
	mv_if_int r0, l1
	ldrb	r12, [r0, #mval_b_exp]
	tst	r12, #mval_esign_mask
	bne	l2
l1:
	ldr	r0, [fp, #arg0_save]			/* Compare first with third */
	ldr	r1, [fp, #arg2_save]
	b	comp
l2:
	ldr	r0, [fp, #arg2_save]			/* Compare third with first */
	ldr	r1, [fp, #arg0_save]
comp:
	bl	numcmp
	cmp	r0, #0					/* Set condition code for caller */
	mov	sp, fp
	pop	{fp, pc}

	.end
