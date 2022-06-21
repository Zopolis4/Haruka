# Video Scroll Test
# Copyright (C) 2016 Hideki EIRAKU

	.code16
_start:	.global	_start
	jmp	1f
	.org	3
	.ascii	"IBMJ"
	.org	32
1:	mov	$6, %ax		# 640x200 2 color graphics
	int	$0x10
	mov	$0x2001, %ax	# Horizontal Displayed 0x20 (32) chars 512x200
	mov	$0x3d4, %dx
	out	%ax, %dx
	mov	$0xb800, %ax
	mov	%ax, %es
	xor	%si, %si
	xor	%di, %di
	cld
	xor	%dx, %dx
1:	call	drawln
	test	%dx, %dx
	jne	1b
mainlp:	call	drawln
	add	$0x20, %si
	and	$0xfff, %si
	push	%dx
	mov	$0x3da, %dx
1:	in	%dx, %al
	test	$8, %al		# VSYNC
	je	1b
	mov	$0x3d4, %dx
	mov	%si, %ax
	mov	$0xc, %al	# Start Address High
	out	%ax, %dx
	mov	%si, %ax
	mov	%al, %ah
	mov	$0xd, %al	# Start Address Low
	out	%ax, %dx
	mov	$0x3da, %dx
1:	in	%dx, %al
	test	$8, %al		# VSYNC
	jne	1b
	pop	%dx
	jmp	mainlp
drawln:	push	%di
	mov	%dx, %bx
	cmp	$100, %bx
	jb	1f
	mov	$200, %bx
	sub	%dx, %bx
1:	shl	%bx
	mov	$256, %cx
	sub	%bx, %cx
	xor	%ax, %ax
	push	%cx
	push	%bx
	push	%bx
	call	draw
	pop	%cx
	not	%al
	call	draw
	pop	%cx
	call	draw
	pop	%cx
	not	%al
	call	draw
	pop	%di
	inc	%dx
	xor	$0x2000, %di
	test	$0x2000, %di
	jne	drawln
	add	$0x40, %di
	and	$0x1fff, %di
	cmp	$200, %dx
	jb	1f
	xor	%dx, %dx
1:	ret
draw:	cmp	$0, %ah
	je	1f
3:	sar	%al
	rcl	%bl
	inc	%ah
	and	$7, %ah
	je	2f
	loop	3b
	ret
2:	xchg	%bl, %al
	stosb
	xchg	%bl, %al
	loop	1f
	ret
1:	push	%cx
	shr	%cx
	shr	%cx
	shr	%cx
	rep	stosb
	pop	%cx
	and	$7, %cx
	je	1f
2:	sar	%al
	rcl	%bl
	inc	%ah
	loop	2b
1:	ret
	.org	510
	.byte	0x55, 0xaa
