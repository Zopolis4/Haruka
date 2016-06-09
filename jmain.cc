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

using namespace std;

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

extern "C"
{
  extern void reset8088 (void);
  extern int run8088 (void);
}

struct threaddata
{
#ifdef forWin
  HWND hwnd;
  HWND hwndaccess;
#endif
  int endflag;
  int resetflag;
  stdfdc *fdc;
  bool pcjrflag;
  bool warmflag;
  bool origpcjrflag;
  char *cart[6];		// D0, D8, E0, E8, F0, F8
};

threaddata mainthreaddata;
jkey keybd;
int keyconvtable[256];
jioclass *jiop;
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
    /* FIXME */ if (addr >= 0xa0000 && addr <= 0xbffff) *slow += 6;
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
  char *fdfile[4];
};

void
mainthreadplus (void *p)
{
  struct maindata *md = (struct maindata *)p;
  jmem systemrom (131072);
  jmem kanjirom (262144);
  jmem mainram (384 * 1024);	// extended 128KB x 3
  jmem program (128 * 1024);	// base 64KB + extended 64KB
  jmem cartrom (192 * 1024);	// D0000-FFFFF
  bool cart_exist[6] = { false, false, false, false, false, false };
  try{
  sdlsound soundclass (11025, 1024 * 4);
  int clk, clk2;

  if (mainthreaddata.origpcjrflag)
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
    jvideo videoclass (surface, &program, &kanjirom);
    {
      stdfdc fdc (&videoclass);
      mainthreaddata.fdc = &fdc;
      {
	int i;
	for (i = 0; i < 4; i++)
	  if (md->fdfile[i] != NULL)
	    mainthreaddata.fdc->insert (i, md->fdfile[i]);
      }
      {
	jioclass jio (&videoclass, &soundclass, &systemrom,
		       &program, &mainram, &kanjirom, &keybd, &cartrom, &fdc);

	jiop = &jio;
	{
	  bool redraw = false;
	  Uint32 tickcount, d;

	  tickcount = SDL_GetTicks ();
	  clk = 0;
	  while (!mainthreaddata.endflag)
	    {
	      if (mainthreaddata.resetflag)
		{
		  int i;

		  mainthreaddata.resetflag = 0;
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
		  if (mainthreaddata.warmflag)
		    {
		      mainram.write (0x473, 0x12);
		      mainram.write (0x472, 0x34);
		    }
		  cartrom.clearrom ();
		  jio.set_base1_rom (false);
		  jio.set_base2_rom (false);
		  for (i = 0 ; i < 6 ; i++)
		    {
		      if (mainthreaddata.cart[i])
			{
			  int remain = 192 * 1024 - i * 32768;
			  if (remain > 96 * 1024) // Cartridge max 96KB
			    remain = 96 * 1024;
			  int size = cartrom.loadrom2 (i * 32768,
						       mainthreaddata.cart[i],
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
		  if (mainthreaddata.pcjrflag)
		    {
		      cartrom.loadrom (65536 * 1, "PCJR_E.ROM", 65536);
		      cartrom.loadrom (65536 * 2, "PCJR_F.ROM", 65536);
		      jio.set_base1_rom (true);
		      jio.set_base2_rom (true);
		    }
		  if (mainthreaddata.origpcjrflag)
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
		      if (mainthreaddata.warmflag)
			{
			  program.write (0x473, 0x12);
			  program.write (0x472, 0x34);
			}
		    }
		}
#if 0
	      intclass.intcalland (intclass.getintmask () ^ 255);
	      if ((intclass.getintcall () & (intclass.getintmask () ^ 255))
		  && intclass.getlastint () == 0)
		intclass.setlastint (emumain.hardint ((t16)intclass.
						      getintcall () &
						      (intclass.getintmask
						       () ^ 255),
						      (t16)8));
#endif
	      jio.zeroclk ();
	      clk2 = 0;
	      //clk2 += emumain.run () * 3;
	      clk2 += run8088 () * 3;
	      //clk2 += emumain.run () * 3;
	      //clk2 += emumain.run () * 3;
	      //clk2 += jio.getclk ();
	      if (jio.timerset (clk2, redraw))
		//emumain.nmiint ();
		nmi8088 (1);
	      clk += clk2;
	      if (clk >= 238955)
		{
		  redraw = !soundclass.get_hurry ();
		  clk -= 238955;
		}
#if 0
	      if (clk >= 4770 * 3 * 17)
		{
#if 0
		  for (;;)
		    {
		      d = GetTickCount () - tickcount;
		      if (d >= 0 && d < 17)
			Sleep (17 - d);
		      if (d >= 17)
			break;
		    }
#endif
#if 1
		  const Uint32 speed = 17;
		  for (;;)
		    {
		      d = SDL_GetTicks () - tickcount;
		      if (d >= 0 && d < speed)
			;//SDL_Delay (speed - d);
		      if (d >= speed)
			break;
		    }
		  clk -= 4770 * 3 * 17; //5500 /*4770*/ * 3 * 17;//d;
		  if (d >= 34)
		    redraw = false;
		  else
		    redraw = true;
		  tickcount += speed;
#endif
		}
#endif
	      //Sleep (10);
	    }
	}
      }
    }
  }
}catch(char*p){fprintf(stderr,"%s\n",p);}catch(...){fprintf(stderr,"ERROR\n");}
}

int
sdlmainthread (void *p)
{
  mainthreadplus (p);
  return 0;
}

static int
keyconv (SDLKey key)
{
  switch (key)
    {
    case SDLK_BACKSPACE: return 0xe;
    case SDLK_TAB: return 0xf;
    case SDLK_CLEAR: break;
    case SDLK_RETURN: return 0x1c;
    case SDLK_PAUSE: break;
    case SDLK_ESCAPE: return 0x1;
    case SDLK_SPACE: return 0x39;
    case SDLK_EXCLAIM: break;
    case SDLK_QUOTEDBL: break;
    case SDLK_HASH: break;
    case SDLK_DOLLAR: break;
    case SDLK_AMPERSAND: break;
    case SDLK_QUOTE: return 0x28;
    case SDLK_LEFTPAREN: break;
    case SDLK_RIGHTPAREN: break;
    case SDLK_ASTERISK: break;
    case SDLK_PLUS: break;
    case SDLK_COMMA: return 0x33;
    case SDLK_MINUS: return 0xc;
    case SDLK_PERIOD: return 0x34;
    case SDLK_SLASH: return 0x35;
    case SDLK_0: return 0xb;
    case SDLK_1: return 0x2;
    case SDLK_2: return 0x3;
    case SDLK_3: return 0x4;
    case SDLK_4: return 0x5;
    case SDLK_5: return 0x6;
    case SDLK_6: return 0x7;
    case SDLK_7: return 0x8;
    case SDLK_8: return 0x9;
    case SDLK_9: return 0xa;
    case SDLK_COLON: break;
    case SDLK_SEMICOLON: return 0x27;
    case SDLK_LESS: break;
    case SDLK_EQUALS: return 0xd;
    case SDLK_GREATER: break;
    case SDLK_QUESTION: break;
    case SDLK_AT: break;
    case SDLK_LEFTBRACKET: return 0x1a;
    case SDLK_BACKSLASH: return 0x2b;
    case SDLK_RIGHTBRACKET: return 0x1b;
    case SDLK_CARET: break;
    case SDLK_UNDERSCORE: break;
    case SDLK_BACKQUOTE: return 0x29;
    case SDLK_a: return 0x1e;
    case SDLK_b: return 0x30;
    case SDLK_c: return 0x2e;
    case SDLK_d: return 0x20;
    case SDLK_e: return 0x12;
    case SDLK_f: return 0x21;
    case SDLK_g: return 0x22;
    case SDLK_h: return 0x23;
    case SDLK_i: return 0x17;
    case SDLK_j: return 0x24;
    case SDLK_k: return 0x25;
    case SDLK_l: return 0x26;
    case SDLK_m: return 0x32;
    case SDLK_n: return 0x31;
    case SDLK_o: return 0x18;
    case SDLK_p: return 0x19;
    case SDLK_q: return 0x10;
    case SDLK_r: return 0x13;
    case SDLK_s: return 0x1f;
    case SDLK_t: return 0x14;
    case SDLK_u: return 0x16;
    case SDLK_v: return 0x2f;
    case SDLK_w: return 0x11;
    case SDLK_x: return 0x2d;
    case SDLK_y: return 0x15;
    case SDLK_z: return 0x2c;
    case SDLK_DELETE: return 0x53;
    case SDLK_KP0: break;
    case SDLK_KP1: break;
    case SDLK_KP2: break;
    case SDLK_KP3: break;
    case SDLK_KP4: break;
    case SDLK_KP5: break;
    case SDLK_KP6: break;
    case SDLK_KP7: break;
    case SDLK_KP8: break;
    case SDLK_KP9: break;
    case SDLK_KP_PERIOD: break;
    case SDLK_KP_DIVIDE: break;
    case SDLK_KP_MULTIPLY: break;
    case SDLK_KP_MINUS: return 0x4a;
    case SDLK_KP_PLUS: return 0x4e;
    case SDLK_KP_ENTER: break;
    case SDLK_KP_EQUALS: break;
    case SDLK_UP: return 0x48;
    case SDLK_DOWN: return 0x50;
    case SDLK_RIGHT: return 0x4d;
    case SDLK_LEFT: return 0x4b;
    case SDLK_INSERT: return 0x52;
    case SDLK_HOME: return 0x47;
    case SDLK_END: break;
    case SDLK_PAGEUP: break;
    case SDLK_PAGEDOWN: break;
    case SDLK_F1: return 0x3b;
    case SDLK_F2: return 0x3c;
    case SDLK_F3: return 0x3d;
    case SDLK_F4: return 0x3e;
    case SDLK_F5: return 0x3f;
    case SDLK_F6: return 0x40;
    case SDLK_F7: return 0x41;
    case SDLK_F8: return 0x42;
    case SDLK_F9: return 0x43;
    case SDLK_F10: return 0x44;
    case SDLK_F11: return 0x54;	// Fn
    case SDLK_F12: return 0x6a;	// yen
    case SDLK_F13: break;
    case SDLK_F14: break;
    case SDLK_F15: break;
    case SDLK_NUMLOCK: break;
    case SDLK_CAPSLOCK: return 0x3a;
    case SDLK_SCROLLOCK: return 0x46;
    case SDLK_RSHIFT: return 0x36;
    case SDLK_LSHIFT: return 0x2a;
    case SDLK_RCTRL: return 0x6d; // henkan
    case SDLK_LCTRL: return 0x1d;
    case SDLK_RALT: return 0x6b; // kanji
    case SDLK_LALT: return 0x38;
    case SDLK_RMETA: break;
    case SDLK_LMETA: break;
    case SDLK_LSUPER: break;
    case SDLK_RSUPER: return 0x6c; // mu-henkan
    case SDLK_MODE: break;
    case SDLK_HELP: break;
    case SDLK_PRINT: return 0x37;
    case SDLK_SYSREQ: break;
    case SDLK_BREAK: break;
    case SDLK_MENU: break;
    case SDLK_POWER: break;
    case SDLK_EURO: break;
    default: break;
    }
  return 0;
}

int
main (int argc, char **argv)
{
  struct maindata md;
  int i, j;

  if (SDL_Init (SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
		SDL_INIT_JOYSTICK) < 0)
    {
      cerr << "SDL_Init failed: " << SDL_GetError () << endl;
      return 1;
    }

  atexit(SDL_Quit);

  surface = SDL_SetVideoMode (640, 400, 8, SDL_SWSURFACE | SDL_HWPALETTE);
  mainthreaddata.endflag = 0;
  mainthreaddata.resetflag = 1;
  mainthreaddata.fdc = 0;
  mainthreaddata.pcjrflag = false;
  mainthreaddata.warmflag = false;
  mainthreaddata.origpcjrflag = false;
  for (i = 0; i < 6; i++)
  mainthreaddata.cart[i] = NULL;
  for (i = 0; i < 4; i++)
    md.fdfile[i] = NULL;
  for (i = 1, j = 0; i < argc; i++)
    {
      if (strcmp (argv[i], "-j") == 0)
	mainthreaddata.pcjrflag = true;
      else if (strcmp (argv[i], "-w") == 0)
	mainthreaddata.warmflag = true;
      else if (strcmp (argv[i], "-o") == 0)
	mainthreaddata.origpcjrflag = true;
      else if (strcmp (argv[i], "-d0") == 0 && i + 1 < argc)
	mainthreaddata.cart[0] = argv[++i];
      else if (strcmp (argv[i], "-d8") == 0 && i + 1 < argc)
	mainthreaddata.cart[1] = argv[++i];
      else if (strcmp (argv[i], "-e0") == 0 && i + 1 < argc)
	mainthreaddata.cart[2] = argv[++i];
      else if (strcmp (argv[i], "-e8") == 0 && i + 1 < argc)
	mainthreaddata.cart[3] = argv[++i];
      else if (strcmp (argv[i], "-f0") == 0 && i + 1 < argc)
	mainthreaddata.cart[4] = argv[++i];
      else if (strcmp (argv[i], "-f8") == 0 && i + 1 < argc)
	mainthreaddata.cart[5] = argv[++i];
      else if (j < 4)
	md.fdfile[j++] = argv[i];
    }

  SDL_CreateThread (sdlmainthread, &md);

  jevent event (&keybd);

  while (!event.get_quit_flag ())
    event.handle_event ();
  return 0;
}

