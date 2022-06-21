all : rom2txt makesrc fontsrc.dat makefont.com checksumtest
.PHONY : all

fontsrc.dat : makesrc shnm7x14r.bdf shnmk16.bdf order.txt
	./makesrc shnm7x14r.bdf shnmk16.bdf < order.txt > fontsrc.dat

makefont.com : makefont.s
	gcc -nostdlib -nostdinc -Wl,--oformat=binary -Wl,-Ttext,0x100 -o makefont.com makefont.s
