# Make font for DOS
# Copyright (C) 2016 Hideki EIRAKU

# Create FONT_*.ROM from fontsrc.dat and the BIOS font

	.code16

_start:	.global	_start
	# Open files
	mov	$fntsrc, %dx
	xor	%cx, %cx
	mov	$0x3d00, %ax	# Open
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, hdlsrc
	mov	$font_8, %dx
	xor	%cx, %cx
	mov	$0x3c, %ah	# Create
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, hdl_8
	mov	$font_9, %dx
	xor	%cx, %cx
	mov	$0x3c, %ah	# Create
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, hdl_9
	mov	$font_a, %dx
	xor	%cx, %cx
	mov	$0x3c, %ah	# Create
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, hdl_a
	mov	$font_b, %dx
	xor	%cx, %cx
	mov	$0x3c, %ah	# Create
	int	$0x21
	jnc	1f
	mov	$openms, %dx
	jmp	err
1:	mov	%ax, hdl_b
	# Set video mode
	mov	$6, %ax		# 640x200 2 colors
	int	$0x10
	# Read fontsrc bitmap size
	mov	hdlsrc, %bx
	mov	$2, %cx
	mov	$buf, %dx
	call	filerd
	mov	buf, %ax
	mov	$32, %dx
	mul	%dx
	add	$2, %ax
	adc	$0, %dx
	mov	%ax, pos	# pos=bitmapsize*32+2
	mov	%dx, pos+2
	# Read fontsrc index
mainl:	cmpw	$2048*3, outcnt
	jb	1f
	jmp	done
1:	mov	hdlsrc, %bx
	mov	pos, %dx
	mov	pos+2, %cx
	call	filesk
	mov	$2, %cx
	mov	$buf, %dx
	call	filerd
	add	%ax, pos
	adcw	$0, pos+2
	testw	$0xc000, buf
	je	1f
	mov	$1, %cx
	mov	$buf+2, %dx
	call	filerd
	add	%ax, pos
	adcw	$0, pos+2
1:	movw	$1, consec
	movw	$1, repcnt
	mov	buf, %ax
	mov	$0, %ch
	mov	buf+2, %cl
	inc	%cx
	test	$0x4000, %ax
	je	1f
	mov	%cx, consec
1:	test	$0x8000, %ax
	je	1f
	mov	%cx, repcnt
1:	and	$0x3fff, %ax
	mov	consec, %cx
	call	readf
	mov	consec, %si
	mov	repcnt, %di
	mov	$buf, %dx
	mov	hdl_8, %bx
	mov	$2048*0, %cx
	call	writef
	mov	hdl_9, %bx
	mov	$2048*1, %cx
	call	writef
	mov	hdl_a, %bx
	mov	$2048*2, %cx
	call	writef
	jmp	mainl
readf:	# Read font, %ax=index, %cx=readsize/32
	push	%cx
	mov	$32, %dx
	mul	%dx
	add	$2, %ax
	adc	$0, %dx
	mov	%dx, %cx
	mov	%ax, %dx
	mov	hdlsrc, %bx
	call	filesk
	pop	%ax
	mov	$5, %cl
	shl	%cl, %ax
	mov	%ax, %cx
	mov	$buf, %dx
	call	filerd
	ret
writef:	# Write font %bx=handle, %cx=startaddr, %si=consec, %di=repcnt
	mov	outcnt, %ax
	and	$~2047, %ax
	cmp	%ax, %cx
	jne	2f
	test	%di, %di
	jne	1f
2:	ret
1:	push	%cx
	add	$2048, %cx
	sub	outcnt, %cx
	cmp	%si, %cx
	jbe	1f
	mov	%si, %cx
1:	mov	%cx, %ax
	push	%ax
	mov	$5, %cl
	shl	%cl, %ax
	mov	%ax, %cx
	call	filewr
	pop	%cx
	add	%ax, %dx
	add	%cx, outcnt
	sub	%cx, %si
	pop	%cx
	jne	1f
	sub	%ax, %dx
	mov	$1, %si
	dec	%di
	jmp	writef
1:	ret
mkinv:	# Make Japanese 8x8 invalid mark for first byte of 2-byte code
	mov	$buf+1, %bx
	mov	%cl, %al
	mov	$32, %ah
	mul	%ah
	add	%ax, %bx
2:	mov	$~0xc0, %ax
	xor	%si, %si
1:	mov	%al, (%bx, %si)
	shr	%ax
	add	$2, %si
	cmp	$2*8, %si
	jb	1b
	cmp	%cl, %ch
	je	1f
	inc	%cl
	add	$32, %bx
	jmp	2b
1:	ret
chksum:	# Calculate checksum in the file %ah=sum, %bx=hdl, %dx=offset, %cx=len
	push	%si
	push	%di
	push	%cx
	mov	%ax, %di
	push	%dx
	push	%cx
	xor	%cx, %cx
	call	filesk
	pop	%cx
	pop	%dx
2:	mov	%cx, %ax
	cmp	$8192, %ax
	jb	1f
	mov	$8192, %ax
1:	add	%ax, %dx
	sub	%ax, %cx
	push	%dx
	push	%cx
	mov	$buf, %dx
	mov	%ax, %cx
	call	filerd
	mov	%dx, %si
	mov	%di, %ax
	cld
1:	lodsb
	add	%al, %ah
	loop	1b
	mov	%ax, %di
	pop	%cx
	pop	%dx
	test	%cx, %cx
	jne	2b
	mov	%di, %ax
	pop	%cx
	pop	%di
	pop	%si
	ret
wrcksm:	push	%ax
	neg	%ah
	mov	%ah, buf
	xor	%cx, %cx
	call	filesk
	mov	$buf, %dx
	mov	$1, %cx
	call	filewr
	pop	%ax
	ret
done:	# Hankaku special hack
	mov	hdl_8, %bx
	xor	%cx, %cx
	xor	%dx, %dx
	call	filesk
	mov	$buf, %dx
	mov	$8192, %cx
	call	filerd
	# 1. Copy BIOS 8x8 fonts
	mov	$0x900, %ax
	mov	$buf+1, %bx
1:	push	%ax
	push	%bx
	mov	$1, %bx
	mov	$1, %cx
	int	$0x10		# Write character
	pop	%bx
	mov	$0xb800, %ax
	mov	%ax, %es
	mov	%es: 80*0, %al
	mov	%al, 2*0(%bx)
	mov	%al, 2*8(%bx)
	mov	%es: 80*0+0x2000, %al
	mov	%al, 2*1(%bx)
	mov	%al, 2*9(%bx)
	mov	%es: 80*1, %al
	mov	%al, 2*2(%bx)
	mov	%al, 2*10(%bx)
	mov	%es: 80*1+0x2000, %al
	mov	%al, 2*3(%bx)
	mov	%al, 2*11(%bx)
	mov	%es: 80*2, %al
	mov	%al, 2*4(%bx)
	mov	%al, 2*12(%bx)
	mov	%es: 80*2+0x2000, %al
	mov	%al, 2*5(%bx)
	mov	%al, 2*13(%bx)
	mov	%es: 80*3, %al
	mov	%al, 2*6(%bx)
	mov	%al, 2*14(%bx)
	mov	%es: 80*3+0x2000, %al
	mov	%al, 2*7(%bx)
	mov	%al, 2*15(%bx)
	pop	%ax
	add	$32, %bx
	inc	%al
	jne	1b
	# 2. Create some Japanese 8x8 fonts from BIOS 8x8 fonts
	cld
	mov	$jasrc1, %si
3:	lodsw
	cmp	$0, %al
	je	1f
	mov	%ax, %cx
	mov	$32, %ah
	mul	%ah
	mov	$buf+1+2*8, %di
	add	%ax, %di
4:	lodsb
	mov	$32, %ah
	mul	%ah
	mov	$buf+1+2*8, %bx
	add	%ax, %bx
	push	%si
	mov	$2*8, %si
2:	sub	$2, %di
	sub	$2, %si
	mov	(%bx, %si), %al
	mov	%al, (%di)
	jne	2b
	pop	%si
	add	$2*8+2*16, %di
	cmp	%cl, %ch
	je	3b
	inc	%cl
	jmp	4b
1:	# 3. Fix enter mark (0x1b ESC)
	xor	%ax, %ax
	mov	$2*2, %si
	mov	%si, %di
	mov	$buf+1+32*0x1b, %bx
1:	or	(%bx, %si), %al
	cmp	%al, %ah
	je	2f
	mov	%al, %ah
	mov	%si, %di
2:	add	$2, %si
	cmp	$2*8, %si
	jb	1b
	not	%al
	inc	%al
	and	%ah, %al
	or	%al, -2(%bx, %di)
	or	%al, -4(%bx, %di)
	# 4. Create some Japanese 8x8 fonts from 8x16 fonts
	mov	$buf+32*0x7e, %bx
	mov	$0x7e, %cl
2:	xor	%si, %si
	lea	1(%si), %di
1:	mov	(%bx, %si), %al
	or	2(%bx, %si), %al
	mov	%al, (%bx, %di)
	add	$2*2, %si
	add	$2, %di
	cmp	$32, %si
	jb	1b
	add	%si, %bx
	inc	%cl
	jne	2b
	# 5. Fill Japanese 8x8 invalid mark fonts
	mov	$0x9f81, %cx
	call	mkinv
	mov	$0xfce0, %cx
	call	mkinv
	# 6. Fix 8x16 symbols
	cld
	mov	$jasrc2, %si
3:	lodsb
	cmp	$0, %al
	je	1f
	mov	$32, %ah
	mul	%ah
	mov	$buf+2*16, %di
	add	%ax, %di
	lodsb
	mov	$32, %ah
	mul	%ah
	mov	$buf, %bx
	add	%ax, %bx
	push	%si
	mov	$2*16, %si
2:	sub	$2, %di
	sub	$2, %si
	mov	(%bx, %si), %al
	mov	%al, (%di)
	jne	2b
	pop	%si
	jmp	3b
1:	# 7. Fix 8x16 line symbols
	mov	$buf+32, %bx
3:	xor	%si, %si
	xor	%di, %di
1:	mov	(%bx, %si), %al
	test	$0x40, %al
	je	2f
	or	$0x80, %al
2:	mov	%al, fixlin(%di)
	add	$2, %si
	inc	%di
	cmp	$16, %di
	jb	1b
	mov	fixlin+1, %al
	mov	%al, fixlin
	mov	fixlin+14, %al
	mov	%al, fixlin+15
	xor	%si, %si
	xor	%di, %di
1:	mov	fixlin(%si), %al
	mov	%al, %ah
	shl	%al
	or	%ah, %al
	shr	%ah
	or	%ah, %al
	mov	%al, (%bx, %di)
	inc	%si
	add	$2, %di
	cmp	$16, %si
	jb	1b
	mov	$1, %si
	mov	$2, %di
1:	mov	fixlin(%si), %al
	mov	%al, %ah
	shl	%al
	shr	%ah
	or	%ah, %al
	or	%al, -2(%bx, %di)
	or	%al, 2(%bx, %di)
	inc	%si
	add	$2, %di
	cmp	$14, %si
	jb	1b
	xor	%si, %si
	xor	%di, %di
1:	mov	fixlin(%si), %al
	xor	%al, (%bx, %di)
	inc	%si
	add	$2, %di
	cmp	$16, %si
	jb	1b
	add	$32, %bx
	cmp	$buf+32*0x1a, %bx
	jb	3b
	# 8. 8x16 arrow symbols - make arrow from line
	xor	%ax, %ax
	mov	$2*2, %si
	mov	%si, %di
	mov	$buf+32*0x1e, %bx	# 0x1e right arrow
1:	or	(%bx, %si), %al
	cmp	%al, %ah
	je	2f
	mov	%al, %ah
	mov	%si, %di
2:	add	$2, %si
	cmp	$2*14, %si
	jb	1b
	not	%al
	inc	%al
	and	%ah, %al
	shl	%al
	or	%al, -2(%bx, %di)
	or	%al, 2(%bx, %di)
	shl	%al
	or	%al, -4(%bx, %di)
	or	%al, 4(%bx, %di)
	xor	%ax, %ax
	mov	$2*2, %si
	mov	%si, %di
	mov	$buf+32*0x1f, %bx	# 0x1f left arrow
1:	or	(%bx, %si), %al
	cmp	%al, %ah
	je	2f
	mov	%al, %ah
	mov	%si, %di
2:	add	$2, %si
	cmp	$2*14, %si
	jb	1b
	mov	$0x80, %al
1:	test	%ah, %al
	jne	1f
	shr	%al
	jne	1b
1:	shr	%al
	or	%al, -2(%bx, %di)
	or	%al, 2(%bx, %di)
	shr	%al
	or	%al, -4(%bx, %di)
	or	%al, 4(%bx, %di)
	xor	%ax, %ax
	mov	$2*5, %si
	mov	%si, %di
	mov	$buf+32*0x1b, %bx	# 0x1b enter mark
1:	or	(%bx, %si), %al
	cmp	%al, %ah
	je	2f
	mov	%al, %ah
	mov	%si, %di
2:	add	$2, %si
	cmp	$2*14, %si
	jb	1b
	push	%ax
	mov	$0x80, %al
1:	test	%ah, %al
	jne	1f
	shr	%al
	jne	1b
1:	shr	%al
	or	%al, -2(%bx, %di)
	or	%al, 2(%bx, %di)
	shr	%al
	or	%al, -4(%bx, %di)
	or	%al, 4(%bx, %di)
	pop	%ax
	not	%al
	inc	%al
	and	%ah, %al
	or	%al, -2(%bx, %di)
	or	%al, -4(%bx, %di)
	or	%al, -6(%bx, %di)
	or	%al, -8(%bx, %di)
	or	%al, -10(%bx, %di)
	# 9. Write
	mov	hdl_8, %bx
	xor	%cx, %cx
	xor	%dx, %dx
	call	filesk
	mov	$buf, %dx
	mov	$8192, %cx
	call	filewr
	# Checksum
	# 80000-87FFF (87FFF)
	mov	hdl_8, %bx
	mov	$0, %ah
	xor	%dx, %dx
	mov	$0x7fff, %cx
	call	chksum
	call	wrcksm
	# 87800-87FFF = 9E000-9E7FF
	mov	hdl_9, %bx
	mov	$0xe7ff, %dx
	call	wrcksm
	# 90800-90FFF, 92800-92FFF, ..., AE800-AEFFF (90FFF)
	# 91000-917FF, 93000-937FF, ..., AF000-AF7FF (9101F)
	# 91800-91FFF, 93800-93FFF, ..., AF800-AFFFF (91FFF)
	mov	$0x800, %dx
	mov	$0x7ff, %cx
	mov	$0, %ah
	call	chksum
	mov	%ah, cksm1
	inc	%dx
	mov	$0x1f, %cx
	mov	$0, %ah
	call	chksum
	inc	%dx
	mov	$0x7e0, %cx
	call	chksum
	mov	%ah, cksm2
	mov	$0x7ff, %cx
	mov	$0, %ah
	call	chksum
	mov	%ah, cksm3
	inc	%dx
	inc	%cx
1:	add	%cx, %dx
	mov	cksm1, %ah
	call	chksum
	mov	%ah, cksm1
	mov	cksm2, %ah
	call	chksum
	mov	%ah, cksm2
	mov	cksm3, %ah
	call	chksum
	mov	%ah, cksm3
	test	%dx, %dx
	jne	1b
	cmp	hdl_a, %bx
	mov	hdl_a, %bx
	jne	1b
	mov	hdl_9, %bx
	mov	$0xfff, %dx
	mov	cksm1, %ah
	call	wrcksm
	mov	$0x101f, %dx
	mov	cksm2, %ah
	call	wrcksm
	mov	$0x1fff, %dx
	mov	cksm3, %ah
	call	wrcksm
	# 90000-9FFFF = B0000-BFFFF
	mov	hdl_9, %bx
	xor	%cx, %cx
	xor	%dx, %dx
	call	filesk
	mov	$8, %cx
cploop:	push	%cx
	mov	hdl_9, %bx
	mov	$buf, %dx
	mov	$8192, %cx
	call	filerd
	mov	hdl_b, %bx
	call	filewr
	pop	%cx
	loop	cploop
	mov	hdlsrc, %bx
	mov	$0x3e, %ah	# Close
	int	$0x21
	mov	hdl_8, %bx
	mov	$0x3e, %ah	# Close
	int	$0x21
	mov	hdl_9, %bx
	mov	$0x3e, %ah	# Close
	int	$0x21
	mov	hdl_a, %bx
	mov	$0x3e, %ah	# Close
	int	$0x21
	mov	hdl_b, %bx
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
filesk:	mov	$0x4200, %ax	# Seek
	int	$0x21
	jnc	1f
	mov	$seekm, %dx
	jmp	err
1:	ret
filerd:	mov	$0x3f, %ah	# Read
	int	$0x21
	jc	2f
	cmp	%ax, %cx
	je	1f
2:	mov	$readms, %dx
	jmp	err
1:	ret
filewr:	mov	$0x40, %ah	# Write
	int	$0x21
	jc	2f
	cmp	%ax, %cx
	je	1f
2:	mov	$writem, %dx
	jmp	err
1:	ret
fntsrc:	.string	"fontsrc.dat"
font_8:	.string	"FONT_8.ROM"
font_9:	.string	"FONT_9.ROM"
font_a:	.string	"FONT_A.ROM"
font_b:	.string	"FONT_B.ROM"
openms:	.ascii	"Open file failed\r\n$"
readms:	.ascii	"Read from file failed\r\n$"
writem:	.ascii	"Write to file failed\r\n$"
seekm:	.ascii	"Seek failed\r\n$"
endm:	.ascii	"Done\r\n$"
jasrc1:	.byte	0x01, 0x1f		# Control characters
	.byte	0xc9, 0xbb, 0xc8, 0xbc, 0xba, 0xcd, 0x00
	.byte	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	.byte	0xce, 0x00, 0x00, 0x00, 0x00, 0xca, 0xcb, 0xb9
	.byte	0x00, 0xcc, 0x00, 0x1b, 0x00, 0xb3, 0x1a, 0x1b
	.byte	0x5c, 0x5c, 0x9d	# Yen
	.byte	0x00			# End
jasrc2:	.byte	0xa0, 0x02, 0x01, 0x0d, 0x02, 0x0c, 0x03, 0x0e, 0x04, 0x0b
	.byte	0x05, 0x19, 0x06, 0x12, 0x1b, 0x13, 0x1e, 0x12, 0x1f, 0x12
	.byte	0x10, 0x0f, 0x11, 0x15, 0x12, 0x16, 0x13, 0x17, 0x14, 0x18
	.byte	0x15, 0x13, 0x16, 0x14, 0x17, 0x12, 0x1d, 0x19, 0x19, 0x11
	.byte	0x07, 0x00, 0x08, 0x00, 0x09, 0x00, 0x0a, 0x00, 0x0b, 0x00
	.byte	0x0c, 0x00, 0x0d, 0x00, 0x0e, 0x00, 0x0f, 0x00
	.byte	0x11, 0x00, 0x12, 0x00, 0x13, 0x00, 0x14, 0x00, 0x18, 0x00
	.byte	0x1a, 0x00, 0x1c, 0x00
	.byte	0x00
hdlsrc:	.word	0
hdl_8:	.word	0
hdl_9:	.word	0
hdl_a:	.word	0
hdl_b:	.word	0
pos:	.long	0
consec:	.word	0		# For consecutive index
repcnt:	.word	0		# For repeat index
outcnt:	.word	0
cksm1:	.byte	0
cksm2:	.byte	0
cksm3:	.byte	0
fixlin:	.space	16
buf:
