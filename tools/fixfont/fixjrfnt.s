# Fix PCjr Font Program for DOS
# Copyright (C) 2016 Hideki EIRAKU

# Replace every 8x8 PCjr font in FONT_8.ROM with
# the font in the BIOS ROM

# for DOS, PC compatible

# To assemble:
# gcc -nostdlib -nostdinc -Wl,--oformat=binary -Wl,-Ttext,0x100 -o fixjrfnt.com fixjrfnt.s
	.code16

_start:	.global	_start
	mov	$filnam, %dx
	xor	%cx, %cx
	mov	$0x3d02, %ax	# Open
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, %bx
	mov	$6, %ax		# 640x200 2 colors
	int	$0x10
	mov	$0x900, %ax
lp1:	push	%ax
	push	%bx
	mov	$1, %bx
	mov	$1, %cx
	cwtd
	mov	%al, %dl
	int	$0x10		# Write caharacter
	pop	%bx
	mov	$0xb800, %ax
	mov	%ax, %es
	push	%es: 80 * 3 + 0x2000
	push	%es: 80 * 3
	push	%es: 80 * 2 + 0x2000
	push	%es: 80 * 2
	push	%es: 80 * 1 + 0x2000
	push	%es: 80 * 1
	push	%es: 80 * 0 + 0x2000
	push	%es: 80 * 0
	shl	%dx
	shl	%dx
	shl	%dx
	shl	%dx
	shl	%dx
	add	$0x11, %dx
lp2:	push	%dx
	xor	%cx, %cx
	mov	$0x4200, %ax	# Seek
	int	$0x21
	jnc	1f
	mov	$seekm, %dx
	jmp	err
1:	mov	%sp, %dx
	inc	%dx
	inc	%dx
	mov	$1, %cx
	mov	$0x40, %ah
	int	$0x21		# Write
	jnc	1f
	mov	$writem, %dx
	jmp	err
1:	pop	%dx
	pop	%ax
	inc	%dx
	inc	%dx
	test	$0x10, %dx
	jne	lp2
	pop	%ax
	inc	%al
	jne	lp1
	mov	$0x3e, %ah	# Close
	int	$0x21
	mov	$endm, %dx
	mov	$0x9, %ah
	int	$0x21
	mov	$0x4c00, %ax
	int	$0x21
err:	mov	$0x9, %ah
	int	$0x21
	mov	$0x4c01, %ax
	int	$0x21
filnam:	.string	"FONT_8.ROM"
openms:	.ascii	"Open file failed\r\n$"
writem:	.ascii	"Write to file failed\r\n$"
seekm:	.ascii	"Seek failed\r\n$"
endm:	.ascii	"Done\r\n$"
