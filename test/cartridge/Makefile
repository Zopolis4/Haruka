all : tetjr.rom tetjx.rom
.PHONY : all

tetjr.rom : tetjr.s calccrc
	gcc -nostdlib -nostdinc -Wl,--oformat,binary,-Ttext,0x7c00 tetjr.s -o tetjr.rom
	./calccrc < tetjr.rom >> tetjr.rom

tetjx.rom : tetjx.s calccrc
	gcc -nostdlib -nostdinc -Wl,--oformat,binary,-Ttext,0x7c00 tetjx.s -o tetjx.rom
	./calccrc < tetjx.rom >> tetjx.rom

calccrc :
