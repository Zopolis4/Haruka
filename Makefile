#ARCH=-march=pentium-m
CXXFLAGS=-g -Wall -O2 $(ARCH) `pkg-config sdl2 --cflags`

.PHONY : all clean
all : 5511emu

clean :
	rm jmain.o jmem.o sdlsound.o jvideo.o jioclass.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o

5511emu : jmain.o jmem.o sdlsound.o jvideo.o jioclass.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o
	$(CXX) -g -o 5511emu jmain.o jmem.o sdlsound.o jvideo.o jkey.o jioclass.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o `pkg-config sdl2 --libs`

8088.o : 8088.c
	$(CC) -g -O3 $(ARCH) -Wall -o 8088.o -c 8088.c

8259a.o : 8259a.c
	$(CC) -g -O3 $(ARCH) -Wall -o 8259a.o -c 8259a.c

jmain.o : jmain.cc jmem.hh sdlsound.hh jvideo.hh jtype.hh jkey.hh jfdc.hh stdfdc.hh jevent.hh jjoy.hh jioclass.hh
	$(CXX) -g $(CXXFLAGS) -O2 $(ARCH) -Wall -c jmain.cc

jmem.o : jmem.cc jmem.hh
	$(CXX) -g $(CXXFLAGS) -c jmem.cc

sdlsound.o : sdlsound.cc sdlsound.hh

jvideo.o : jvideo.cc jvideo.hh jtype.hh jmem.hh

jioclass.o : jioclass.cc jvideo.hh jmem.hh jtype.hh jkey.hh sdlsound.hh jfdc.hh jjoy.hh jioclass.hh

jkey.o : jkey.cc jkey.hh jtype.hh

jfdc.o : jfdc.cc jfdc.hh jvideo.hh jtype.hh jmem.hh

stdfdc.o : stdfdc.cc stdfdc.hh jfdc.o jtype.hh jmem.hh jvideo.hh

jevent.o : jevent.cc jtype.hh jkey.hh jevent.hh

jjoy.o : jjoy.cc jjoy.hh
