.globl __aarch64_cas4_sync
__aarch64_cas4_sync:
	mov	w16, w0
	ldxr	w0, [x2]
	cmp	w0, w16
0:	bne	1f

	stlxr	w17, w1, [x2]
	cbnz	w17, 0b
1:	ret
