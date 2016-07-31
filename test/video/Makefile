.PHONY : all
all : scroll.fdd

scroll.fdd : scroll.s
	gcc -nostdlib -nostdinc -Wl,--oformat=binary -Wl,-Ttext,0x7C00 -o scroll.fdd scroll.s
