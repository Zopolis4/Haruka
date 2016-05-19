ARCH=-march=pentium-m
CXXFLAGS=-g -Wall -O2 $(ARCH) `sdl-config --cflags`

.PHONY : all clean
all : 5511emu

clean :
	rm jmain.o jmem.o sdlsound.o jvideo.o jioclass.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o

5511emu : jmain.o jmem.o sdlsound.o jvideo.o jioclass.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o
	$(CXX) -g -o 5511emu jmain.o jmem.o sdlsound.o jvideo.o jkey.o jioclass.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o `sdl-config --libs`

8088.o : 8088.c
	$(CC) -g -O3 $(ARCH) -Wall -o 8088.o -c 8088.c

8259a.o : 8259a.c
	$(CC) -g -O3 $(ARCH) -Wall -o 8259a.o -c 8259a.c

jmain.o : jmain.cc jmem.hh sdlsound.hh jvideo.hh jtype.hh
	$(CXX) -g $(CXXFLAGS) -O2 $(ARCH) -Wall -c jmain.cc

jres.o : jres.rc
	$(WINDRES) jres.rc jres.o

jmem.o : jmem.cc jmem.hh
	$(CXX) -g $(CXXFLAGS) -c jmem.cc

sdlsound.o : sdlsound.cc sdlsound.hh

jvideo.o : jvideo.cc jvideo.hh jtype.hh

jioclass.o : jioclass.cc jvideo.hh jmem.hh jtype.hh

jkey.o : jkey.cc jkey.hh jtype.hh

jfdc.o : jfdc.cc jfdc.hh jvideo.hh jtype.hh

stdfdc.o : stdfdc.cc stdfdc.hh jfdc.o jtype.hh

jevent.o : jevent.cc jtype.hh jkey.hh jevent.hh
