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
#include <cstdio>
#include "SDL.h"
#include "jtype.hh"
#include "jmem.hh"
#include "jvideo.hh"
#include "jkey.hh"
#include "jfdc.hh"
#include "stdfdc.hh"
#include "jevent.hh"
#include "sdlsound.hh"
#include "jioclass.hh"

using std::cerr;
using std::endl;

extern "C"
{
  extern void reset8088 (void);
  extern int run8088 (void);
}

jkey keybd;
int keyconvtable[256];
jioclass *jiop;
SDL_Window *window;
SDL_Surface *surface;
#ifdef forWin
HANDLE mainthreadhandle;
DWORD mainthreadid;
HINSTANCE hinst;
COLORREF bkcl;
HDC hdcscreen;
#endif

extern "C"
{
  typedef unsigned int t20;
  extern void nmi8088 (int);
  //extern void printip8088 (void);

  t16
  memory_read (t20 addr, int *slow)
  {
    t16 r;

    r = jiop->memr (addr) & 255;
    *slow = 4;
    /* FIXME */ if (addr >= 0xa0000 && addr <= 0xbffff) *slow = 6;
    return r;
  }
  void
  memory_write (t20 addr, t16 v, int *slow)
  {
    jiop->memw (addr, v & 255);
    *slow = 4;
    /* FIXME */ if (addr >= 0xa0000 && addr <= 0xbffff) *slow = 6;
  }
  t16
  ioport_read (t20 addr, int *slow)
  {
    t16 r;

    r = jiop->in (addr & 65535);
    *slow = 6;
    return r;
  }
  void
  ioport_write (t20 addr, t16 v, int *slow)
  {
    jiop->out (addr & 65535, v & 255);
    *slow = 6;
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
};

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
  try{
  sdlsound soundclass (11025, 1024 * 4);
  int clk, clk2;

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

  {
    jvideo videoclass (window, surface, &program, &kanjirom);
    {
      stdfdc fdc (&videoclass);
      md->fdc = &fdc;
      {
	int i;
	for (i = 0; i < 4; i++)
	  if (md->fdfile[i] != NULL)
	    md->fdc->insert (i, md->fdfile[i]);
      }
      {
	jioclass jio (&videoclass, &soundclass, &systemrom,
		       &program, &mainram, &kanjirom, &keybd, &cartrom, &fdc);

	jiop = &jio;
	{
	  bool redraw = false;

	  clk = 0;
	  while (!md->endflag)
	    {
	      if (md->resetflag)
		{
		  int i;

		  md->resetflag = 0;
		  //emumain.reset ();
		  reset8088 ();
		  jio.out (0xa0, 0); // Disable all interrupts
		  jio.in (0x1ff);
		  jio.out (0x1ff, 0); // System ROM
		  jio.out (0x1ff, 0xbc); // E0000-FFFFF
		  jio.out (0x1ff, 0x23); //
		  for (i = 1 ; i <= 10 ; i++)
		    {
		      jio.out (0x1ff, i);
		      jio.out (0x1ff, 0);
		      jio.out (0x1ff, 0);
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
		      jio.out (0x1ff, 0x08); // Main RAM
		      jio.out (0x1ff, 0xa0); // 00000-1FFFF
		      jio.out (0x1ff, 0x63); // RW, Mask
		      jio.out (0x1ff, 0x09); // VRAM1
		      jio.out (0x1ff, 0xf7); // B8000-BFFFF
		      jio.out (0x1ff, 0x60); // RW, Mask
		      for (i = 0x80; i < 0x93; i++)
			{
			  jio.out (0x1ff, i); // I/O
			  switch (i)
			    {
			    case 0x8d:		     // GA2B
			    case 0x8e:		     // GA03
			    case 0x90:		     // PG2
			      jio.out (0x1ff, 0x00); // Disable
			      jio.out (0x1ff, 0x00);
			      break;
			    case 0x85:		     // FDC
			      jio.out (0x1ff, 0x9e); // 00F0-00FF
			      jio.out (0x1ff, 0x01);
			      break;
			    default:
			      jio.out (0x1ff, 0x80); // Enable
			      jio.out (0x1ff, 0x00);
			    }
			}
		      // Activate cartridge ROMs
		      jio.out (0x1ff, 0);    // System ROM
		      if (cart_exist[5])     // Cartridge in F8000-FFFFF
			{
			  jio.out (0x1ff, 0x00); // Disabled
			  jio.out (0x1ff, 0x00);
			}
		      else if (cart_exist[4]) // Cartridge in F0000-F7FFF
			{
			  jio.out (0x1ff, 0xbf); // F8000-FFFFF
			  jio.out (0x1ff, 0x20); //
			}
		      else
			{
			  jio.out (0x1ff, 0xbe); // F0000-FFFFF
			  jio.out (0x1ff, 0x21); //
			}
		      for (i = 0; i < 6; i++)
			{
			  jio.out (0x1ff, i + 1); // Cartridge ROM
			  if (cart_exist[i])
			    {
			      jio.out (0x1ff, 0xba + i); // Enabled
			      jio.out (0x1ff, 0x20);
			    }
			  else
			    {
			      jio.out (0x1ff, 0x00); // Disabled
			      jio.out (0x1ff, 0x00);
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
	      if (jio.timerset (clk2, redraw))
		nmi8088 (1);
	      clk += clk2;
	      if (clk >= 238955)
		{
		  redraw = !soundclass.get_hurry ();
		  clk -= 238955;
		}
	    }
	}
      }
    }
  }
}catch(char*p){fprintf(stderr,"%s\n",p);}catch(...){fprintf(stderr,"ERROR\n");}
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

  window = SDL_CreateWindow ("5511emu", SDL_WINDOWPOS_UNDEFINED,
			     SDL_WINDOWPOS_UNDEFINED, 640, 400, 0);
  surface = SDL_GetWindowSurface (window);
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

  jevent event (&keybd);
  md.event = &event;

  SDL_Thread *thread = SDL_CreateThread (sdlmainthread, "main", &md);

  while (!event.get_quit_flag ())
    event.handle_event ();

  md.endflag = 1;
  int status;
  SDL_WaitThread (thread, &status);
  return 0;
}
