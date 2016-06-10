# JX cartridge application test
# Copyright (C) 2000-2016  Hideki EIRAKU

# Load address D0000 (-d0)

# [up]    rotate
# [left]  move left
# [right] move right
# [space] drop
# [enter] restart after gameover

	.code16
	bss=0x8000
	seed=0x8000
	score=0x8010
	curbr=score+4
	curxy=curbr+2
	nextbr=0x8018
	timer=0x8020
	.text
_start:	.global	_start
	.byte	0xaa, 0x55
	.byte	0x01		# Length: 512 bytes
	cli
	jmp	1f
	.byte	0x00
gameover:
	mov	$24*80+22+10*2, %di
	mov	$0x0823, %ax
	std
3:	scasw
	jnb	2f
	inc	%di
	inc	%di
	stosw
2:	test	%di, %di
	jns	3b
2:	mov	$0, %ah
	int	$0x16
	cmp	$13, %al
	jne	2b
1:	xor	%ax, %ax
	mov	%ax, %ss
	mov	%ax, %sp
	mov	%ax, %ds
	mov	%ax, %es
	mov	$bss-6, %di
	cld
	cli
	call	1f
	incb	%cs:timer
	iret
1:	pop	%si
	mov	%ax, 0x1c*4+2
	movw	%di, 0x1c*4+0
	cs
	movsw
	cs
	movsw
	cs
	movsw
	out	%al, $0x21
	sti
	mov	%di, %cx
	rep	stosb
	inc	%ax
	int	$0x10
	mov	$0xb800, %ax
	mov	%ax, %es
	mov	$24*80+22, %cx
	rep	stosb
	not	%ax
	mov	$10, %cl
	rep	stosw
1:	stosw
	mov	%ax, %es:-24(%di)
	sub	$82, %di
	jnb	1b
main3:	call	clearlines
	movb	$0x04, curxy-curbr(%si)
main2:	mov	%es:8*80+64-8, %al
	aaa
	mov	%al, timer
main:	call	wrblkc
	js	gameover
	call	wrblks
wait:	incw	seed
	mov	$1, %ah
	int	$0x16
	mov	%sp, %ax
	jnz	1f
	cmpb	$0x9, timer
	ja	2f
	jmp	wait
1:	int	$0x16
2:	call	wrblks
	mov	$curxy+1, %si
	test	%ax, %ax
	je	movedown
	cmp	$' ', %al
	jne	1f
drop:	decb	(%si)
	addw	$1, score-(curxy+1)(%si)
	adcw	$0, score+2-(curxy+1)(%si)
	call	wrblkc
	jns	drop
	.byte	0xb8	# mov $imm, %ax
movedown:
	decb	(%si)
	call	wrblkc
	jns	main2
	incb	(%si)
	call	wrblks
	jmp	main3
1:	inc	%ax
	dec	%si
	cmp	$77, %ah
	ja	main
	je	move
	neg	%al
	cmp	$72, %ah
	jb	main
	ja	move
	dec	%si
move:	addb	%al, (%si)
	call	wrblkc
	jns	main
	subb	%al, (%si)
	jmp	main
clearlines:
	cld
	mov	$100, %bx
	mov	$23*80+22, %di
	push	%di
2:	mov	$10, %cx
	mov	$0x800, %ax
	push	%di
1:	inc	%di
	scasb
	loopne	1b
	pop	%di
	mov	$0xdb, %al
	je	1f
	add	%bx, score
	adc	%cx, score+2
	shl	%bx
	push	%di
	mov	$10, %cl
	rep	stosw
	pop	%di
1:	sub	$80, %di
	jnb	2b
3:	pop	%di
2:	cmp	%ax, %es:(%di)
	jne	3f
	mov	%al, timer
1:	cmpb	$0xdb+4, timer
	jb	1b
	push	%di
	lea	-80(%di), %si
1:	push	%si
	mov	$10, %cl
	rep
	es
	movsw
	pop	%di
	sub	$80+20, %si
	jnb	1b
	jmp	3b
3:	sub	$80, %di
	jnb	2b
getnext:
	# rand
	mov	$0, %ah
	int	$0x1a
	mov	$score, %si
	add	%dx, seed-score(%si)
	# print score
	cld
	lodsw
	xchg	%ax, %bx
	lodsw
	mov	$8*80+64, %di
	std
	mov	$10, %cl
1:	xor	%dx, %dx
	div	%cx
	xchg	%ax, %bx
	div	%cx
	xchg	%ax, %bx
	xchg	%ax, %dx
	add	$0x0730, %ax
	stosw
	xchg	%ax, %dx
	test	%ax, %ax
	jne	1b
	test	%bx, %bx
	jne	1b
	movw	$0x1610, curxy-curbr(%si)
	cwtd
	mov	seed, %ax
	mov	$0x7, %cl
	div	%cx
	mov	%al, %dh
	xchg	%ax, %dx
	add	$9, %al
	mov	%ax, (%si)
	xchg	%ax, nextbr-curbr(%si)
	call	wrblks
	mov	%ax, (%si)
	test	%ax, %ax
	je	getnext
wrblks:
	stc
	.byte	0xb1	# mov $imm, %cl
wrblkc:
	clc
wrblk:
	push	%ax
	mov	curbr, %ax
	sbb	%dx, %dx
	and	$0xdb, %dl
	and	%al, %dh
	call	1f
	.byte	0x7b, 0xd9, 0x9c, 0x1c, 0x6d, 0x3c, 0x5c
1:	pop	%bx
	sub	$9, %bx
	cs
	xlatb
	xchg	%ax, %bx
	mov	$0x802, %cx
	mov	$3, %al
	shr	%bl
	sbb	%ah, %ah
	shr	%bl
	jnp	2f
	adc	$1, %al
3:	sar	%ch
	jnc	1f
	add	%cl, %ah
	neg	%cl
	.byte	0x3d	# cmp $imm, %ax
1:	sub	%cl, %al
	shr	%bl
	jnc	4f
2:	push	%ax
	test	$1, %bh
	jne	1f
	xchg	%ah, %al
	neg	%al
1:	test	$2, %bh
	jne	1f
	neg	%al
	neg	%ah
1:	add	$16+1, %al
	sar	%al
	sar	%ah
	add	curxy, %ax
	mov	$25*80+22-16, %di
1:	sub	$80, %di
	dec	%ah
	jne	1b
	add	%ax, %di
	add	%ax, %di
	xor	%dx, %es:(%di)
	je	1f
	or	$0x80, %ch
1:	inc	%ax
	pop	%ax
4:	jne	3b
	cmp	%bl, %ch	# clears AF
	pop	%ax
	ret

	.org	510
