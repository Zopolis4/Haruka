#ARCH=-march=pentium-m
CXXFLAGS=-g -Wall -O2 $(ARCH) `pkg-config sdl2 --cflags`

.PHONY : all clean
all : 5511emu

clean :
	rm jmain.o jmem.o sdlsound.o jvideo.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o jbus.o jio1ff.o jrtc.o sdlvideo.o

5511emu : jmain.o jmem.o sdlsound.o jvideo.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o jbus.o jio1ff.o jrtc.o sdlvideo.o
	$(CXX) -g -o 5511emu jmain.o jmem.o sdlsound.o jvideo.o jkey.o jfdc.o stdfdc.o 8088.o 8259a.o jevent.o jjoy.o jbus.o jio1ff.o jrtc.o sdlvideo.o `pkg-config sdl2 --libs`

8088.o : 8088.cc 8088.hh 8259a.hh

8259a.o : 8259a.cc 8259a.hh 8088.hh

jmain.o : jmain.cc jmem.hh sdlsound.hh jvideo.hh jtype.hh jkey.hh jfdc.hh stdfdc.hh jevent.hh jjoy.hh jbus.hh jio1ff.hh jrtc.hh sdlvideo.hh 8088.hh 8259a.hh
	$(CXX) -g $(CXXFLAGS) -O2 $(ARCH) -Wall -c jmain.cc

jmem.o : jmem.cc jmem.hh
	$(CXX) -g $(CXXFLAGS) -c jmem.cc

sdlsound.o : sdlsound.cc jmem.hh jvideo.hh sdlsound.hh 8259a.hh

jvideo.o : jvideo.cc jvideo.hh jtype.hh jmem.hh 8259a.hh

jkey.o : jkey.cc jkey.hh jtype.hh

jfdc.o : jfdc.cc jfdc.hh jvideo.hh jtype.hh jmem.hh 8259a.hh

stdfdc.o : stdfdc.cc stdfdc.hh jfdc.o jtype.hh jmem.hh jvideo.hh

jevent.o : jevent.cc jtype.hh jkey.hh jevent.hh 8088.hh

jjoy.o : jjoy.cc jjoy.hh

jbus.o : jbus.cc jbus.hh

jio1ff.o : jio1ff.cc jbus.hh jio1ff.hh

jrtc.o : jrtc.cc jrtc.hh

sdlvideo.o : sdlvideo.cc jmem.hh jvideo.hh sdlvideo.hh
