/*
  Copyright (C) 2000-2016  Hideki EIRAKU

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <iostream>
#include "SDL.h"
#include "jtype.hh"
#include "jmem.hh"
#include "jvideo.hh"
#include "jkey.hh"
#include "jfdc.hh"
#include "stdfdc.hh"
#include "jevent.hh"
#include "sdlsound.hh"
#include "jjoy.hh"
#include "jioclass.hh"
#include "jbus.hh"

using std::cerr;
using std::endl;

extern "C"
{
  extern void reset8088 (void);
  extern int run8088 (void);
}

jbus bus;

extern "C"
{
  typedef unsigned int t20;
  extern void nmi8088 (int);
  //extern void printip8088 (void);

  t16
  memory_read (t20 addr, int *slow)
  {
    *slow = 4;
    return bus.memory_read (addr, *slow);
  }
  void
  memory_write (t20 addr, t16 v, int *slow)
  {
    *slow = 4;
    bus.memory_write (addr, v, *slow);
  }
  t16
  ioport_read (t20 addr, int *slow)
  {
    *slow = 6;
    return bus.ioport_read (addr, *slow);
  }
  void
  ioport_write (t20 addr, t16 v, int *slow)
  {
    *slow = 6;
    bus.ioport_write (addr, v, *slow);
  }
  void
  interrupt_nmi (void)
  {
    nmi8088 (0);
  }
}

struct maindata
{
  int endflag;
  int resetflag;
  stdfdc *fdc;
  bool pcjrflag;
  bool warmflag;
  bool origpcjrflag;
  char *cart[6];		// D0, D8, E0, E8, F0, F8
  char *fdfile[4];
  jevent *event;
  jkey *keybd;
  SDL_Window *window;
  SDL_Surface *surface;
};

class jioclass_io : public jbus::io
{
  jioclass &obj;
public:
  jioclass_io (jbus &bus, jioclass &ioobj);
  void memory_read (unsigned int addr, unsigned int &val, int &cycles);
  void memory_write (unsigned int addr, unsigned int val, int &cycles);
  void ioport_read (unsigned int addr, unsigned int &val, int &cycles);
  void ioport_write (unsigned int addr, unsigned int val, int &cycles);
};

jioclass_io::jioclass_io (jbus &bus, jioclass &ioobj)
  : io (bus), obj (ioobj)
{
}

void
jioclass_io::memory_read (unsigned int addr, unsigned int &val, int &cycles)
{
  val = obj.memr (addr);
  cycles = 4;
  /* FIXME */ if (addr >= 0xa0000 && addr <= 0xbffff) cycles = 6;
}

void
jioclass_io::memory_write (unsigned int addr, unsigned int val, int &cycles)
{
  obj.memw (addr, val);
  cycles = 4;
  /* FIXME */ if (addr >= 0xa0000 && addr <= 0xbffff) cycles = 6;
}

void
jioclass_io::ioport_read (unsigned int addr, unsigned int &val, int &cycles)
{
  val = obj.in (addr);
  cycles = 6;
}

void
jioclass_io::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
  obj.out (addr, val);
  cycles = 6;
}

int
sdlmainthread (void *p)
{
  maindata *md = static_cast <maindata *> (p);
  jmem systemrom (131072);
  jmem kanjirom (262144);
  jmem mainram (384 * 1024);	// extended 128KB x 3
  jmem program (128 * 1024);	// base 64KB + extended 64KB
  jmem cartrom (192 * 1024);	// D0000-FFFFF
  bool cart_exist[6] = { false, false, false, false, false, false };
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
		  kanjirom.write (i * 32 + j * 2 + 0x1011,
				  systemrom.read (0x1e05e + i * 8 + j));
		  // Code 0-127 from F000:FA6E-
		  kanjirom.write (i * 32 + j * 2 + 0x11,
				  systemrom.read (0x1fa6e + i * 8 + j));
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

      sdlsound soundclass (11025, 1024 * 4);
      jvideo videoclass (md->window, md->surface, program, kanjirom);
      jjoy joy;
      stdfdc fdc (videoclass);
      jioclass jio (videoclass, soundclass, systemrom, program, mainram,
		    kanjirom, *md->keybd, cartrom, fdc, joy);
      jioclass_io jio_io (bus, jio);
      int clk, clk2;
      bool redraw = false;

      md->fdc = &fdc;
      {
	int i;
	for (i = 0; i < 4; i++)
	  if (md->fdfile[i] != NULL)
	    md->fdc->insert (i, md->fdfile[i]);
      }

      clk = 0;
      while (!md->endflag)
	{
	  if (md->resetflag)
	    {
	      int i;

	      md->resetflag = 0;
	      //emumain.reset ();
	      reset8088 ();
	      bus.ioport_write (0xa0, 0); // Disable all interrupts
	      bus.ioport_read (0x1ff);
	      bus.ioport_write (0x1ff, 0); // System ROM
	      bus.ioport_write (0x1ff, 0xbc); // E0000-FFFFF
	      bus.ioport_write (0x1ff, 0x23); //
	      for (i = 1 ; i <= 10 ; i++)
		{
		  bus.ioport_write (0x1ff, i);
		  bus.ioport_write (0x1ff, 0);
		  bus.ioport_write (0x1ff, 0);
		}
	      //jio.memw (0x473, 0x12); // for warm start
	      //jio.memw (0x472, 0x34);
	      if (md->warmflag)
		{
		  mainram.write (0x473, 0x12);
		  mainram.write (0x472, 0x34);
		}
	      cartrom.clearrom ();
	      jio.set_base1_rom (false);
	      jio.set_base2_rom (false);
	      for (i = 0 ; i < 6 ; i++)
		{
		  if (md->cart[i])
		    {
		      int remain = 192 * 1024 - i * 32768;
		      if (remain > 96 * 1024) // Cartridge max 96KB
			remain = 96 * 1024;
		      int size = cartrom.loadrom2 (i * 32768,
						   md->cart[i],
						   remain);
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
		jio.set_base2_rom (true);
	      if (cart_exist[5])
		jio.set_base1_rom (true);
	      if (md->pcjrflag)
		{
		  cartrom.loadrom (65536 * 1, "PCJR_E.ROM", 65536);
		  cartrom.loadrom (65536 * 2, "PCJR_F.ROM", 65536);
		  jio.set_base1_rom (true);
		  jio.set_base2_rom (true);
		}
	      if (md->origpcjrflag)
		{
		  // Set up memory and I/O space for PCjr BIOS
		  bus.ioport_write (0x1ff, 0x08); // Main RAM
		  bus.ioport_write (0x1ff, 0xa0); // 00000-1FFFF
		  bus.ioport_write (0x1ff, 0x63); // RW, Mask
		  bus.ioport_write (0x1ff, 0x09); // VRAM1
		  bus.ioport_write (0x1ff, 0xf7); // B8000-BFFFF
		  bus.ioport_write (0x1ff, 0x60); // RW, Mask
		  for (i = 0x80; i < 0x93; i++)
		    {
		      bus.ioport_write (0x1ff, i); // I/O
		      switch (i)
			{
			case 0x8d:		     // GA2B
			case 0x8e:		     // GA03
			case 0x90:		     // PG2
			  bus.ioport_write (0x1ff, 0x00); // Disable
			  bus.ioport_write (0x1ff, 0x00);
			  break;
			case 0x85:		     // FDC
			  bus.ioport_write (0x1ff, 0x9e); // 00F0-00FF
			  bus.ioport_write (0x1ff, 0x01);
			  break;
			default:
			  bus.ioport_write (0x1ff, 0x80); // Enable
			  bus.ioport_write (0x1ff, 0x00);
			}
		    }
		  // Activate cartridge ROMs
		  bus.ioport_write (0x1ff, 0);    // System ROM
		  if (cart_exist[5])     // Cartridge in F8000-FFFFF
		    {
		      bus.ioport_write (0x1ff, 0x00); // Disabled
		      bus.ioport_write (0x1ff, 0x00);
		    }
		  else if (cart_exist[4]) // Cartridge in F0000-F7FFF
		    {
		      bus.ioport_write (0x1ff, 0xbf); // F8000-FFFFF
		      bus.ioport_write (0x1ff, 0x20); //
		    }
		  else
		    {
		      bus.ioport_write (0x1ff, 0xbe); // F0000-FFFFF
		      bus.ioport_write (0x1ff, 0x21); //
		    }
		  for (i = 0; i < 6; i++)
		    {
		      bus.ioport_write (0x1ff, i + 1); // Cartridge ROM
		      if (cart_exist[i])
			{
			  bus.ioport_write (0x1ff, 0xba + i); // Enabled
			  bus.ioport_write (0x1ff, 0x20);
			}
		      else
			{
			  bus.ioport_write (0x1ff, 0x00); // Disabled
			  bus.ioport_write (0x1ff, 0x00);
			}
		    }
		  videoclass.in3da (true);
		  videoclass.out3da (true, 3); // Mode control 2
		  videoclass.out3da (true, 0x10); // Set PCjr memory map
		  if (md->warmflag)
		    {
		      program.write (0x473, 0x12);
		      program.write (0x472, 0x34);
		    }
		}
	    }
	  clk2 = run8088 () * 3;
	  videoclass.clk (clk2, redraw);
	  bool nmiflag = md->keybd->clkin (clk2);
	  joy.clk (clk2);
	  soundclass.clk (clk2);
	  if (nmiflag)
	    nmi8088 (1);
	  clk += clk2;
	  if (clk >= 238955)
	    {
	      redraw = !soundclass.get_hurry ();
	      clk -= 238955;
	    }
	}
    }
  catch (char *p)
    {
      std::cerr << p << std::endl;
    }
  catch (...)
    {
      std::cerr << "ERROR" << std::endl;
    }
  md->event->push_quit_event ();
  return 0;
}

int
main (int argc, char **argv)
{
  maindata md;
  int i, j;

  if (SDL_Init (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK) < 0)
    {
      cerr << "SDL_Init failed: " << SDL_GetError () << endl;
      return 1;
    }

  atexit(SDL_Quit);

  md.window = SDL_CreateWindow ("5511emu", SDL_WINDOWPOS_UNDEFINED,
				SDL_WINDOWPOS_UNDEFINED, 640, 400, 0);
  md.surface = SDL_GetWindowSurface (md.window);
  md.endflag = 0;
  md.resetflag = 1;
  md.fdc = 0;
  md.pcjrflag = false;
  md.warmflag = false;
  md.origpcjrflag = false;
  for (i = 0; i < 6; i++)
    md.cart[i] = NULL;
  for (i = 0; i < 4; i++)
    md.fdfile[i] = NULL;
  for (i = 1, j = 0; i < argc; i++)
    {
      if (strcmp (argv[i], "-j") == 0)
	md.pcjrflag = true;
      else if (strcmp (argv[i], "-w") == 0)
	md.warmflag = true;
      else if (strcmp (argv[i], "-o") == 0)
	md.origpcjrflag = true;
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

  jkey keybd;
  jevent event (keybd);
  md.keybd = &keybd;
  md.event = &event;

  SDL_Thread *thread = SDL_CreateThread (sdlmainthread, "main", &md);

  while (!event.get_quit_flag ())
    event.handle_event ();

  md.endflag = 1;
  int status;
  SDL_WaitThread (thread, &status);
  return 0;
}
