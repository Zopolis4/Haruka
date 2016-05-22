# Font ROM dump program for JX
# Copyright (C) 2013 Hideki EIRAKU

# To assemble:
# gcc -nostdlib -nostdinc -Wl,--oformat=binary -Wl,-Ttext,0x100 -o jxfntdmp.com jxfntdmp.s
	.code16

_start:	.global	_start
	mov	$0x11, %ax
	int	$0x10
	mov	$startm, %dx
	mov	$0x9, %ah
	int	$0x21
	push	%cs
	pop	%ds
	mov	$filnam, %dx
	xor	%cx, %cx
	mov	$0x3c, %ah
	int	$0x21
	jc	opener
	mov	%ax, %bx
	mov	$0x8000, %ax
1:	mov	%ax, %ds
	push	%ax
	xor	%si, %si
	push	%cs
	pop	%es
	mov	$buf, %di
	cld
	mov	$0x1ff, %dx
	mov	$0x3709, %cx
	cli
	call	outga
	inc	%cx
	call	outga
	mov	$0xb007, %cx
	call	outga
	mov	$0x4000 / 2, %cx
	rep	movsw
	mov	$0x3007, %cx
	call	outga
	mov	$0xb70a, %cx
	call	outga
	sti
	push	%cs
	pop	%ds
	mov	$buf, %dx
	mov	$0x4000, %cx
	mov	$0x40, %ah
	int	$0x21
	jc	writee
	cmp	$0x4000, %ax
	jne	writee
	pop	%ax
	add	$0x400, %ax
	cmp	$0xc000, %ax
	jb	1b
	mov	$0x3e, %ah
	int	$0x21
	mov	$endm, %dx
	mov	$0x9, %ah
	int	$0x21
	mov	$0x4c00, %ax
	int	$0x21
outga:	in	%dx, %al
	mov	%cx, %ax
	out	%al, %dx
	mov	%ah, %al
	out	%al, %dx
	ret
opener:	mov	$openms, %dx
1:	mov	$0x9, %ah
	int	$0x21
	mov	$0x4c01, %ax
	int	$0x21
writee:	mov	$writem, %dx
	jmp	1b
filnam:	.string	"font.dat"
startm:	.ascii	"Font ROM dump program for JX\r\n"
	.ascii	"Copyright (C) 2013 Hideki EIRAKU\r\n$"
openms:	.ascii	"Create file failed\r\n$"
writem:	.ascii	"Write to file failed\r\n$"
endm:	.ascii	"Done\r\n$"
buf:
