// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iomanip>
#include <iostream>

#include "SDL.h"

#include "8088.h"
#include "8259a.h"
#include "jbus.h"
#include "jevent.h"
#include "jfdc.h"
#include "jio1ff.h"
#include "jjoy.h"
#include "jkey.h"
#include "jmem.h"
#include "jrtc.h"
#include "jtype.h"
#include "jvideo.h"
#include "sdlsound.h"
#include "sdlvideo.h"
#include "stdfdc.h"

using std::cerr;
using std::endl;

jbus bus;

t16 memory_read (t20 addr, int* slow)
{
  *slow = 4;
  return bus.memory_read (addr, *slow);
}

void memory_write (t20 addr, t16 v, int* slow)
{
  *slow = 4;
  bus.memory_write (addr, v, *slow);
}

t16 ioport_read (t20 addr, int* slow)
{
  *slow = 6;
  return bus.ioport_read (addr, *slow);
}

void ioport_write (t20 addr, t16 v, int* slow)
{
  *slow = 6;
  bus.ioport_write (addr, v, *slow);
}

void interrupt_nmi (void)
{
  nmi8088 (0);
}

struct maindata
{
  int endflag;
  int resetflag;
  stdfdc* fdc;
  bool pcjrflag;
  bool warmflag;
  bool origpcjrflag;
  bool fastflag;
  int memsize;
  char* cart[6];  // D0, D8, E0, E8, F0, F8
  char* fdfile[4];
  jevent* event;
  jkey* keybd;
  SDL_Window* window;
};

class devrom : public jio1ffdev
{
  jmem& mem;
  const unsigned int offset;
  const unsigned int mask;

public:
  devrom (jbus& bus, conf c, jmem& mem, unsigned int offset, unsigned int mask)
      : jio1ffdev (bus, c), mem (mem), offset (offset), mask (mask){};
  void memory_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 4;
    val = mem.read (offset + (addr & mask));
  };
};

class devkjcs : public jio1ffdev
{
  jmem& mem;

public:
  devkjcs (jbus& bus, conf c, jmem& mem) : jio1ffdev (bus, c), mem (mem){};
  void memory_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 4;
    unsigned int x = (addr & 0x3ffff) + 0x80000;
    if (x >= 0x88000 && x <= 0x8ffff)
      val = mem.read (0x8000 | (addr & 0x7ff));
    else
      val = mem.read (addr & 0x3ffff);
  };
  void memory_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 4;
    unsigned int x = (addr & 0x3ffff) + 0x80000;
    if (x >= 0x88000 && x <= 0x8ffff)
      mem.write (0x8000 | (addr & 0x7ff), val);
  };
};

class devmain : public jio1ffdev
{
  jmem& program;

public:
  devmain (jbus& bus, conf c, jmem& program) : jio1ffdev (bus, c), program (program){};
  // Programmable RAM shared with VRAM takes average 2 wait cycles
  void memory_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = program.read (addr & 0x1ffff);
  };
  void memory_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    program.write (addr & 0x1ffff, val);
  };
};

class devvram12 : public jio1ffdev
{
  jvideo& video;
  bool vp2;

public:
  devvram12 (jbus& bus, conf c, jvideo& video, bool vp2)
      : jio1ffdev (bus, c), video (video), vp2 (vp2){};
  // FIXME: VRAM2 0x0000-0x7fff <-> CPU 0xb8000-0xbffff, 0xa8000-0xaffff
  // FIXME: VRAM2 0x8000-0xffff <-> CPU 0xa0000-0xa7fff (in the ex-video card)
  void memory_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = video.read (vp2, vp2 ? (addr & 0xffff) ^ 0x8000 : addr & 0x7fff);
  };
  void memory_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    video.write (vp2, vp2 ? (addr & 0xffff) ^ 0x8000 : addr & 0x7fff, val);
  };
};

class dev8259 : public jio1ffdev
{
public:
  dev8259 (jbus& bus, conf c) : jio1ffdev (bus, c){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = read8259 (addr);
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    write8259 (addr, val);
  };
};

class dev8253 : public jio1ffdev
{
  sdlsound& sound;

public:
  dev8253 (jbus& bus, conf c, sdlsound& sound) : jio1ffdev (bus, c), sound (sound){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = sound.in8253 (addr & 3);
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    sound.out8253 (addr & 3, val);
  };
};

class dev8255 : public jio1ffdev
{
  unsigned int dat8255[3];
  jkey& kbd;
  sdlsound& sound;
  bool exmem64k;

public:
  dev8255 (jbus& bus, conf c, jkey& kbd, sdlsound& sound, bool exmem64k)
      : jio1ffdev (bus, c), kbd (kbd), sound (sound), exmem64k (exmem64k)
  {
    dat8255[0] = dat8255[1] = dat8255[2] = 0;
  };
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    addr &= 7;
    if (addr <= 1)
      val = dat8255[addr];
    else
      val = 2 | (kbd.getnmiflag() ? 1 : 0) | (kbd.getkeydata() ? 64 : 0) |
            // Cassette motor on if (dat8255[1] & 0x18) == 0.
            // Bit 4 is same as bit 5 during cassette motor off.
            // This is tested by BIOS POST.
            (sound.gettimer2out() ? dat8255[1] & 0x18 ? 0x30 : 0x20 : 0) | (exmem64k ? 0 : 0x8);
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    addr &= 7;
    if (addr <= 2)
      dat8255[addr] = val;
    if (addr == 1)
      sound.set8255b (val);
  };
};

class devnmi : public jio1ffdev
{
  jkey& kbd;
  sdlsound& sound;

public:
  devnmi (jbus& bus, conf c, jkey& kbd, sdlsound& sound)
      : jio1ffdev (bus, c), kbd (kbd), sound (sound){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = kbd.in();
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    kbd.out (val);
    sound.selecttimer1in (((val & 0x20) != 0) ? true : false);
  };
};

class devsond : public jio1ffdev
{
  sdlsound& sound;

public:
  devsond (jbus& bus, conf c, sdlsound& sound) : jio1ffdev (bus, c), sound (sound){};
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    // SN76489A needs 32 cycles
    cycles = 32;  // FIXME: Is this correct?
    sound.iowrite (val);
  };
};

class devfdc : public jio1ffdev
{
  jfdc& fdc;

public:
  devfdc (jbus& bus, conf c, jfdc& fdc) : jio1ffdev (bus, c), fdc (fdc){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    switch (addr & 0xf)
    {
    case 4:
      val = fdc.inf4();
      break;
    case 5:
      val = fdc.inf5();
      break;
    }
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    switch (addr & 0xf)
    {
    case 2:
      fdc.outf2 (val);
      break;
    case 5:
      fdc.outf5 (val);
      break;
    }
  };
};

class devjoyw : public jio1ffdev
{
  jjoy& joy;

public:
  devjoyw (jbus& bus, conf c, jjoy& joy) : jio1ffdev (bus, c), joy (joy){};
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    joy.out201 (val);
  };
};

class devjoyr : public jio1ffdev
{
  jjoy& joy;

public:
  devjoyr (jbus& bus, conf c, jjoy& joy) : jio1ffdev (bus, c), joy (joy){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = joy.in201();
  };
};

class devprnt : public jio1ffdev
{
  // Dummy parallel port implementation
  unsigned int reg[4];

public:
  devprnt (jbus& bus, conf c) : jio1ffdev (bus, c) {}
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = reg[addr & 3];
  }
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    switch (addr & 3)
    {
    case 2:  // Control
      val &= 0x1f;
      // Fall through
    case 0:  // Data latch
      reg[addr & 3] = val;
      break;
    }
  }
};

class devcrtc : public jio1ffdev
{
  jvideo& video;

public:
  devcrtc (jbus& bus, conf c, jvideo& video) : jio1ffdev (bus, c), video (video){};
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    if (!(addr & 1))
      video.out3d4 (val);
    else
      video.out3d5 (val);
  };
};

class devga2ab : public jio1ffdev
{
  jvideo& video;
  bool vp2;

public:
  devga2ab (jbus& bus, conf c, jvideo& video, bool vp2)
      : jio1ffdev (bus, c), video (video), vp2 (vp2){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = video.in3da (vp2);
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    video.out3da (vp2, val);
  };
};

class devga03 : public jio1ffdev
{
  jvideo& video;

public:
  devga03 (jbus& bus, conf c, jvideo& video) : jio1ffdev (bus, c), video (video){};
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    val = video.in3dd();
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    video.out3dd (val);
  };
};

class devpg2 : public jio1ffdev
{
  jvideo& video;

public:
  devpg2 (jbus& bus, conf c, jvideo& video) : jio1ffdev (bus, c), video (video){};
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    video.out3d9 (val);
  };
};

class devpg1 : public jio1ffdev
{
  jvideo& video;

public:
  devpg1 (jbus& bus, conf c, jvideo& video) : jio1ffdev (bus, c), video (video){};
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    video.out3df (val);
  };
};

class devmfg : public jbus::io
{
public:
  devmfg (jbus& bus) : io (bus)
  {
    set_memory_iobmp (0);
    set_ioport_iobmp (0x0002);
  };
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    switch (addr == 0x10 ? val & 0xff : addr == 0x11 ? 0x100 : addr == 0x12 ? 0x101 : 0x102)
    {
    case 0x102:
      return;
#define z(a, b)                                                                                    \
  case 0x##a:                                                                                      \
    std::cerr << "MFG: " b << std::endl;                                                           \
    break
      z (ff, "8088 processor test");
      z (fe, "8255 initialization and test");
      z (fd, "set up 46505 and video gate array to get memory working");
      z (fc, "planar board ros checksum test");
      z (fb, "ram mapping");
      z (fa, "base 8k read/write storage test");
      z (f9, "(vram test)");
      z (f8, "rom cartridge configuration check");
      z (f7, "interrupts");
      z (f6, "initialize and test the 8259 interrupt controller chip");
      z (f5, "8253 timer checkout");
      z (f4, "crt attachment test");
      z (f3, "set up keyboard parameters");
      z (f2, "32k vram (vram2) test");
      z (f1, "kj-rom and gaiji sram test");
      z (f0, "memory size determine and test");
      z (ef, "keyboard test");
      z (ee, "cassette interface test");
      z (ed, "serial port (2fx) test");
      z (ec, "parallel port test");
      z (eb, "optional rom test");
      z (ea, "diskette attachment test");
      z (e9, "PCjr cartridge rom checksum test");
#undef z
    default:
      std::cerr << "MFG: port 0x" << std::setw (2) << std::setfill ('0') << std::hex << addr
                << " code 0x" << std::setw (2) << std::setfill ('0') << std::hex << val
                << std::endl;
      break;
    }
    cycles = 6;
  };
};

class devexmem : public jbus::io
{
  jmem& mainram;
  jvideo& video;
  const unsigned int exmemsize;

public:
  devexmem (jbus& bus, jmem& mainram, jvideo& video, unsigned int exmemsize)
      : io (bus), mainram (mainram), video (video), exmemsize (exmemsize)
  {
    // if (exmemsize == 128 * 1024 * 0) set_memory_iobmp (0);
    // if (exmemsize == 128 * 1024 * 1) set_memory_iobmp (0xf);
    // if (exmemsize == 128 * 1024 * 2) set_memory_iobmp (0x3f);
    // if (exmemsize == 128 * 1024 * 3) set_memory_iobmp (0xff);
    // if (exmemsize == 128 * 1024 * 4) set_memory_iobmp (0x3ff);
    set_memory_iobmp (exmemsize >= 0x20000 ? (4 << (exmemsize >> 16)) - 1 : 0);
    set_ioport_iobmp (0);
  };
  void memory_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    if (video.pcjrmem())
    {
      if (addr >= 0x20000 && addr < 0x20000 + exmemsize)
      {
        cycles = 4;
        val = mainram.read (addr - 0x20000);
      }
    }
    else
    {
      if (addr < exmemsize)
      {
        cycles = 4;
        val = mainram.read (addr);
      }
    }
  };
  void memory_write (unsigned int addr, unsigned int val, int& cycles)
  {
    if (video.pcjrmem())
    {
      if (addr >= 0x20000 && addr < 0x20000 + exmemsize)
      {
        cycles = 4;
        mainram.write (addr - 0x20000, val);
      }
    }
    else
    {
      if (addr < exmemsize)
      {
        cycles = 4;
        mainram.write (addr, val);
      }
    }
  };
};

class devrtc : public jbus::io
{
  jrtc& rtc;

public:
  devrtc (jbus& bus, jrtc& rtc) : io (bus), rtc (rtc)
  {
    set_memory_iobmp (0);
    set_ioport_iobmp (0x0040);
  }
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles)
  {
    cycles = 6;
    if ((addr & 0xfff0) == 0x360)
      val = rtc.inb (addr);
    if (false && (addr & 0xfff0) == 0x360)
      std::cerr << "RTC Read " << std::hex << addr << ',' << val << std::endl;
  }
  void ioport_write (unsigned int addr, unsigned int val, int& cycles)
  {
    cycles = 6;
    if ((addr & 0xfff0) == 0x360)
      rtc.outb (addr, val);
    if (false && (addr & 0xfff0) == 0x360)
      std::cerr << "RTC Write " << std::hex << addr << ',' << val << std::endl;
  }
};

int sdlmainthread (void* p)
{
  maindata* md = static_cast<maindata*> (p);
  jmem systemrom (131072);
  jmem kanjirom (262144);
  jmem mainram (512 * 1024);  // extended 128KB x 4 (last 128KB for PCjr)
  jmem program (128 * 1024);  // base 64KB + extended 64KB
  jmem cartrom (192 * 1024);  // D0000-FFFFF
  bool cart_exist[6] = {false, false, false, false, false, false};
  try
  {
    if (md->origpcjrflag)
    {
      systemrom.loadrom (65536, "bios.rom", 65536);
      // Extract font from the BIOS
      int i;
      for (i = 0; i < 128; i++)
      {
        int j;
        for (j = 0; j < 8; j++)
        {
          // Code 128-255 from F000:E05E-
          kanjirom.write (i * 32 + j * 2 + 0x1011, systemrom.read (0x1e05e + i * 8 + j));
          // Code 0-127 from F000:FA6E-
          kanjirom.write (i * 32 + j * 2 + 0x11, systemrom.read (0x1fa6e + i * 8 + j));
        }
      }
    }
    else
    {
      systemrom.loadrom (0, "BASE_E.ROM", 65536);
      systemrom.loadrom (65536, "BASE_F.ROM", 65536);
      kanjirom.loadrom (65536 * 0, "FONT_8.ROM", 65536);
      kanjirom.loadrom (65536 * 1, "FONT_9.ROM", 65536);
      kanjirom.loadrom (65536 * 2, "FONT_A.ROM", 65536);
      kanjirom.loadrom (65536 * 3, "FONT_B.ROM", 65536);
    }

    sdlvideo videohw (md->window);
    sdlsound soundclass (11025, 1024 * 4, 128, videohw);
    jvideo videoclass (videohw, program, kanjirom);
    jjoy joy;
    stdfdc fdc (videoclass);
    jrtc rtc;
    devrom d_irom7 (bus, jio1ffdev::conf (0x00, 0203, 0003, 0074, 0040), systemrom, 0, 0x1ffff);
    devrom d_erom2 (bus, jio1ffdev::conf (0x01, 0357, 0157, 0020, 0000), cartrom, 32768 * 0,
                    0x7fff);  // Writable?
    devrom d_erom3 (bus, jio1ffdev::conf (0x02, 0357, 0157, 0020, 0000), cartrom, 32768 * 1,
                    0x7fff);  // Writable?
    devrom d_erom4 (bus, jio1ffdev::conf (0x03, 0217, 0017, 0060, 0040), cartrom, 32768 * 2,
                    0x7fff);
    devrom d_erom5 (bus, jio1ffdev::conf (0x04, 0217, 0017, 0060, 0040), cartrom, 32768 * 3,
                    0x7fff);
    devrom d_erom6 (bus, jio1ffdev::conf (0x05, 0217, 0017, 0060, 0040), cartrom, 32768 * 4,
                    0x7fff);
    devrom d_erom7 (bus, jio1ffdev::conf (0x06, 0217, 0017, 0060, 0040), cartrom, 32768 * 5,
                    0x7fff);
    devkjcs d_kjcs (bus, jio1ffdev::conf (0x07, 0277, 0177, 0000, 0000), kanjirom);
    devmain d_main (bus, jio1ffdev::conf (0x08, 0237, 0037, 0040, 0140), program);
    devvram12 d_vram1 (bus, jio1ffdev::conf (0x09, 0237, 0137, 0140, 0040), videoclass, false);
    devvram12 d_vram2 (bus, jio1ffdev::conf (0x0A, 0237, 0037, 0040, 0140), videoclass, true);
    dev8259 d_8259 (bus, jio1ffdev::conf (0x80, 0200, 0000, 0004, 0000));
    dev8253 d_8253 (bus, jio1ffdev::conf (0x81, 0200, 0000, 0010, 0000), soundclass);
    dev8255 d_8255 (bus, jio1ffdev::conf (0x82, 0200, 0000, 0014, 0000), *md->keybd, soundclass,
                    md->memsize > 64);
    devnmi d_nmi (bus, jio1ffdev::conf (0x83, 0200, 0000, 0024, 0000), *md->keybd, soundclass);
    devsond d_sond (bus, jio1ffdev::conf (0x84, 0200, 0000, 0030, 0000), soundclass);
    devfdc d_fdc (bus, jio1ffdev::conf (0x85, 0377, 0177, 0000, 0000), fdc);
    devjoyw d_joyw (bus, jio1ffdev::conf (0x86, 0200, 0000, 0100, 0000), joy);
    devjoyr d_joyr (bus, jio1ffdev::conf (0x87, 0200, 0000, 0100, 0000), joy);
    devprnt d_prnt (bus, jio1ffdev::conf (0x88, 0200, 0000, 0157, 0000));
    jio1ffdev d_8250 (bus, jio1ffdev::conf (0x89, 0200, 0000, 0137, 0000));
    devcrtc d_crtc (bus, jio1ffdev::conf (0x8A, 0200, 0000, 0172, 0000), videoclass);
    jio1ffdev d_ga01 (bus, jio1ffdev::conf (0x8B, 0200, 0000, 0173, 0000, 07, 00));  // (reserved)
    devga2ab d_ga2a (bus, jio1ffdev::conf (0x8C, 0200, 0000, 0173, 0000, 07, 02), videoclass,
                     false);
    devga2ab d_ga2b (bus, jio1ffdev::conf (0x8D, 0200, 0000, 0173, 0000, 07, 02), videoclass, true);
    devga03 d_ga03 (bus, jio1ffdev::conf (0x8E, 0200, 0000, 0173, 0000, 07, 05), videoclass);
    jio1ffdev d_lpgt (bus, jio1ffdev::conf (0x8F, 0200, 0000, 0173, 0000, 03, 02));
    devpg2 d_pg2 (bus, jio1ffdev::conf (0x90, 0200, 0000, 0173, 0000, 07, 01), videoclass);
    devpg1 d_pg1 (bus, jio1ffdev::conf (0x91, 0200, 0000, 0173, 0000, 07, 07), videoclass);
    jio1ffdev d_modm (bus, jio1ffdev::conf (0x92, 0200, 0000, 0177, 0000));
    jio1ffdev d_etsc (bus, jio1ffdev::conf (0x93, 0200, 0000, 0000, 0177));
    devmfg d_mfg (bus);
    jio1ffstatus d_1ff (bus);
    devexmem d_exmem (bus, mainram, videoclass, md->memsize > 128 ? (md->memsize - 128) * 1024 : 0);
    devrtc d_rtc (bus, rtc);

    md->fdc = &fdc;
    {
      int i;
      for (i = 0; i < 4; i++)
        if (md->fdfile[i] != NULL)
          md->fdc->insert (i, md->fdfile[i]);
    }

    // CPU speed choice
    // Normal mode: 14.318MHz / 3 = 4.77MHz
    // Fast mode:   14.318MHz / 2 = 7.16MHz
    // Fast mode corresponds to JX-5 extended (kakucho) mode
    int cpuclkdiv = md->fastflag ? 2 : 3;
    while (!md->endflag)
    {
      if (md->resetflag)
      {
        int i;

        md->resetflag = 0;
        // emumain.reset ();
        reset8088();
        bus.ioport_write (0xa0, 0);  // Disable all interrupts
        bus.ioport_read (0x1ff);
        bus.ioport_write (0x1ff, 0);     // System ROM
        bus.ioport_write (0x1ff, 0xbc);  // E0000-FFFFF
        bus.ioport_write (0x1ff, 0x23);  //
        for (i = 1; i <= 10; i++)
        {
          bus.ioport_write (0x1ff, i);
          bus.ioport_write (0x1ff, 0);
          bus.ioport_write (0x1ff, 0);
        }
        // jio.memw (0x473, 0x12); // for warm start
        // jio.memw (0x472, 0x34);
        if (md->warmflag)
        {
          if (md->memsize > 128)
          {
            mainram.write (0x473, 0x12);
            mainram.write (0x472, 0x34);
          }
          else
          {
            program.write (0x473, 0x12);
            program.write (0x472, 0x34);
          }
        }
        cartrom.clearrom();
        d_1ff.set_base1_rom (false);
        d_1ff.set_base2_rom (false);
        d_1ff.set_ex_video (true);
        for (i = 0; i < 6; i++)
        {
          if (md->cart[i])
          {
            int remain = 192 * 1024 - i * 32768;
            if (remain > 96 * 1024)  // Cartridge max 96KB
              remain = 96 * 1024;
            int size = cartrom.loadrom2 (i * 32768, md->cart[i], remain);
            cart_exist[i] = true;
            if (size > 32768 * 1 && i + 1 < 6)
              i++;
            cart_exist[i] = true;
            if (size > 32768 * 2 && i + 1 < 6)
              i++;
            cart_exist[i] = true;
          }
        }
        if (cart_exist[4])
          d_1ff.set_base2_rom (true);
        if (cart_exist[5])
          d_1ff.set_base1_rom (true);
        if (md->pcjrflag)
        {
          cartrom.loadrom (65536 * 1, "PCJR_E.ROM", 65536);
          cartrom.loadrom (65536 * 2, "PCJR_F.ROM", 65536);
          d_1ff.set_base1_rom (true);
          d_1ff.set_base2_rom (true);
        }
        if (md->origpcjrflag)
        {
          // Set up memory and I/O space for PCjr BIOS
          bus.ioport_write (0x1ff, 0x08);  // Main RAM
          bus.ioport_write (0x1ff, 0xa0);  // 00000-1FFFF
          bus.ioport_write (0x1ff, 0x63);  // RW, Mask
          bus.ioport_write (0x1ff, 0x09);  // VRAM1
          bus.ioport_write (0x1ff, 0xf7);  // B8000-BFFFF
          bus.ioport_write (0x1ff, 0x60);  // RW, Mask
          for (i = 0x80; i < 0x93; i++)
          {
            bus.ioport_write (0x1ff, i);  // I/O
            switch (i)
            {
            case 0x8d:                         // GA2B
            case 0x8e:                         // GA03
            case 0x90:                         // PG2
              bus.ioport_write (0x1ff, 0x00);  // Disable
              bus.ioport_write (0x1ff, 0x00);
              break;
            case 0x85:                         // FDC
              bus.ioport_write (0x1ff, 0x9e);  // 00F0-00FF
              bus.ioport_write (0x1ff, 0x01);
              break;
            default:
              bus.ioport_write (0x1ff, 0x80);  // Enable
              bus.ioport_write (0x1ff, 0x00);
            }
          }
          // Activate cartridge ROMs
          bus.ioport_write (0x1ff, 0);  // System ROM
          if (cart_exist[5])            // Cartridge in F8000-FFFFF
          {
            bus.ioport_write (0x1ff, 0x00);  // Disabled
            bus.ioport_write (0x1ff, 0x00);
          }
          else if (cart_exist[4])  // Cartridge in F0000-F7FFF
          {
            bus.ioport_write (0x1ff, 0xbf);  // F8000-FFFFF
            bus.ioport_write (0x1ff, 0x20);  //
          }
          else
          {
            bus.ioport_write (0x1ff, 0xbe);  // F0000-FFFFF
            bus.ioport_write (0x1ff, 0x21);  //
          }
          for (i = 0; i < 6; i++)
          {
            bus.ioport_write (0x1ff, i + 1);  // Cartridge ROM
            if (cart_exist[i])
            {
              bus.ioport_write (0x1ff, 0xba + i);  // Enabled
              bus.ioport_write (0x1ff, 0x20);
            }
            else
            {
              bus.ioport_write (0x1ff, 0x00);  // Disabled
              bus.ioport_write (0x1ff, 0x00);
            }
          }
          videoclass.in3da (true);
          videoclass.out3da (true, 3);     // Mode control 2
          videoclass.out3da (true, 0x10);  // Set PCjr memory map
          if (md->warmflag)
          {
            program.write (0x473, 0x12);
            program.write (0x472, 0x34);
          }
        }
      }
      int clk = run8088() * cpuclkdiv;
      videoclass.clk (clk);
      bool nmiflag = md->keybd->clkin (clk);
      joy.clk (clk);
      soundclass.clk (clk);
      fdc.clk (clk);
      rtc.clk (clk);
      if (nmiflag)
        nmi8088 (1);
    }
  }
  catch (char* p)
  {
    std::cerr << p << std::endl;
  }
  catch (...)
  {
    std::cerr << "ERROR" << std::endl;
  }
  md->event->push_quit_event();
  return 0;
}

int main (int argc, char** argv)
{
  maindata md;
  int i, j;

  bool exsize = false;
  bool novsync = false;
  md.endflag = 0;
  md.resetflag = 1;
  md.fdc = 0;
  md.pcjrflag = false;
  md.warmflag = false;
  md.origpcjrflag = false;
  md.fastflag = false;
  for (i = 0; i < 6; i++)
    md.cart[i] = NULL;
  for (i = 0; i < 4; i++)
    md.fdfile[i] = NULL;
  char* memsize = NULL;
  for (i = 1, j = 0; i < argc; i++)
  {
    if (strcmp (argv[i], "-j") == 0)
      md.pcjrflag = true;
    else if (strcmp (argv[i], "-w") == 0)
      md.warmflag = true;
    else if (strcmp (argv[i], "-o") == 0)
      md.origpcjrflag = true;
    else if (strcmp (argv[i], "-e") == 0)
      exsize = true;
    else if (strcmp (argv[i], "-f") == 0)
      md.fastflag = true;
    else if (strcmp (argv[i], "-s") == 0)
      novsync = true;
    else if (strcmp (argv[i], "-m") == 0 && i + 1 < argc)
      memsize = argv[++i];
    else if (strcmp (argv[i], "-d0") == 0 && i + 1 < argc)
      md.cart[0] = argv[++i];
    else if (strcmp (argv[i], "-d8") == 0 && i + 1 < argc)
      md.cart[1] = argv[++i];
    else if (strcmp (argv[i], "-e0") == 0 && i + 1 < argc)
      md.cart[2] = argv[++i];
    else if (strcmp (argv[i], "-e8") == 0 && i + 1 < argc)
      md.cart[3] = argv[++i];
    else if (strcmp (argv[i], "-f0") == 0 && i + 1 < argc)
      md.cart[4] = argv[++i];
    else if (strcmp (argv[i], "-f8") == 0 && i + 1 < argc)
      md.cart[5] = argv[++i];
    else if (j < 4)
      md.fdfile[j++] = argv[i];
  }
  if (memsize)
  {
    if (!strcmp (memsize, "64"))
      md.memsize = 64;
    else if (!strcmp (memsize, "128"))
      md.memsize = 128;
    else if (!strcmp (memsize, "256"))
      md.memsize = 256;
    else if (!strcmp (memsize, "384"))
      md.memsize = 384;
    else if (!strcmp (memsize, "512"))
      md.memsize = 512;
    else if (!strcmp (memsize, "640") && md.origpcjrflag)
      md.memsize = 640;
    else
    {
      std::cerr << "Invalid memory size: " << memsize << std::endl;
      return 1;
    }
  }
  else
  {
    if (md.origpcjrflag)
      md.memsize = 640;
    else
      md.memsize = 512;
  }
  if (!novsync)
  {
    SDL_SetHint (SDL_HINT_FRAMEBUFFER_ACCELERATION, "1");
    SDL_SetHint (SDL_HINT_RENDER_VSYNC, "1");
  }
  if (SDL_Init (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
  {
    cerr << "SDL_Init failed: " << SDL_GetError() << endl;
    return 1;
  }
  atexit (SDL_Quit);
  md.window = SDL_CreateWindow ("5511emu", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                exsize ? 752 : 672, exsize ? 557 : 432, SDL_WINDOW_RESIZABLE);

  jkey keybd;
  jevent event (keybd);
  md.keybd = &keybd;
  md.event = &event;

  SDL_Thread* thread = SDL_CreateThread (sdlmainthread, "main", &md);

  while (!event.get_quit_flag())
    event.handle_event();

  md.endflag = 1;
  int status;
  SDL_WaitThread (thread, &status);
  return 0;
}
