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

#include <cstring>
#include <cstdio>
#include "jtype.hh"
#include "jmem.hh"
#include "jvideo.hh"
#include "sdlsound.hh"
#include "jkey.hh"
#include "jfdc.hh"
#include "jioclass.hh"

extern "C"
{
  extern void trigger_irq8259 (unsigned int);
  extern void untrigger_irq8259 (unsigned int);
  extern t16 read8259 (t16);
  extern t16 write8259 (t16, t16);
  extern void printip8088 (void);
}

jioclass::jioclass (jvideo *d2, sdlsound *d3,
		    jmem *sys, jmem *prg, jmem *main, jmem *knj,
		    jkey *key, jmem *cart, jfdc *dsk)
{
  struct_regs1ff d[31] = {
    {0, 0, 0203, 0003, 0074, 0040}, // 00 IROM7
    {0, 0, 0357, 0157, 0020, 0000}, // 01 EROM2
    {0, 0, 0357, 0157, 0020, 0000}, // 02 EROM3
    {0, 0, 0217, 0017, 0060, 0040}, // 03 EROM4
    {0, 0, 0217, 0017, 0060, 0040}, // 04 EROM5
    {0, 0, 0217, 0017, 0060, 0040}, // 05 EROM6
    {0, 0, 0217, 0017, 0060, 0040}, // 06 EROM7
    {0, 0, 0277, 0177, 0000, 0000}, // 07 KJCS
    {0, 0, 0237, 0037, 0240/*0040*/, 0156/*40*/}, // 08 MAIN
    {0, 0, 0237, 0137, 0140, 0040}, // 09 VRAM1
    {0, 0, 0237, 0037, 0040, 0140}, // 0A VRAM2
    {0, 0, 0200, 0000, 0004, 0000}, // 80 8259
    {0, 0, 0200, 0000, 0010, 0000}, // 81 8253
    {0, 0, 0200, 0000, 0014, 0000}, // 82 8255
    {0, 0, 0200, 0000, 0024, 0000}, // 83 NMI
    {0, 0, 0200, 0000, 0030, 0000}, // 84 SOND
    {0, 0, 0377, 0177, 0000, 0000}, // 85 FDC
    {0, 0, 0200, 0000, 0100, 0000}, // 86 JOYW
    {0, 0, 0200, 0000, 0100, 0000}, // 87 JOYR
    {0, 0, 0200, 0000, 0157, 0000}, // 88 PRNT
    {0, 0, 0200, 0000, 0137, 0000}, // 89 8250
    {0, 0, 0200, 0000, 0172, 0000}, // 8A CRTC
    {0, 0, 0200, 0000, 0173, 0000}, // 8B GA01 (reserved)  0
    {0, 0, 0200, 0000, 0173, 0000}, // 8C GA2A  2
    {0, 0, 0200, 0000, 0173, 0000}, // 8D GA2B  2
    {0, 0, 0200, 0000, 0173, 0000}, // 8E GA03  5
    {0, 0, 0200, 0000, 0173, 0000}, // 8F LPGT  2 or 6
    {0, 0, 0200, 0000, 0173, 0000}, // 90 PG2   1
    {0, 0, 0200, 0000, 0173, 0000}, // 91 PG1   7
    {0, 0, 0200, 0000, 0177, 0000}, // 92 MODM
    {0, 0, 0200, 0000, 0000, 0177}, // 93 ETSC
  };
  //regs1ff = d;
  memcpy ((void *)regs1ff, (void *)d, sizeof regs1ff);
  videoclass = d2;
  soundclass = d3;
  systemrom = sys;
  program = prg;
  mainram = main;
  kanjirom = knj;
  kbd = key;
  joyx = joyy = joyb = 0;
  ignore21 = 0;
  flag20 = 0;
  timer2flag = 0;
  fdc_mode = 0;
  fdc_flag = 0;
  timerintflag = 0;
  cartrom = cart;
  fdc = dsk;
  pcjrmode = false;
}

t16
jioclass::memr_ (unsigned long adr)
{
  unsigned char dat;
  t16 tmp, tmp2;
  int i;

  tmp = (adr >> 15) & 037;
  tmp2 = 0;
  for (i = 0; i <= 10; i++)
    {
      if (regs1ff[i].reg1 & 128)
	{
	  if ((regs1ff[i].reg1 & 32) && (regs1ff[i].reg2 & 32))
	    {
	      tmp2 = regs1ff[i].reg2 & 037;
	      tmp2 ^= 037;
	      if ((tmp2 & regs1ff[i].reg1) == (tmp2 & tmp))
		break;
	    }
	}
    }
  switch (i)
    {
    case 0:
      dat = systemrom->read (adr & 0x1ffff);
      break;
    case 1:
      dat = cartrom->read (32768 * 0 + (adr & 0x7fff));
      break;
    case 2:
      dat = cartrom->read (32768 * 1 + (adr & 0x7fff));
      break;
    case 3:
      dat = cartrom->read (32768 * 2 + (adr & 0x7fff));
      break;
    case 4:
      dat = cartrom->read (32768 * 3 + (adr & 0x7fff));
      break;
    case 5:
      dat = cartrom->read (32768 * 4 + (adr & 0x7fff));
      break;
    case 6:
      dat = cartrom->read (32768 * 5 + (adr & 0x7fff));
      break;
    case 7:
      dat = kanjirom->read (adr & 0x3ffff);
      break;
    case 8:
      if (videoclass->pcjrmem ())
	{
	  if (adr < 0x20000)
	    dat = program->read (adr & 0x1ffff);
	  else
	    dat = mainram->read ((adr - 0x20000) % (384 * 1024));
	}
      else
	{
	  if (adr < 0x60000)
	    dat = mainram->read (adr % (384 * 1024));
	  else
	    dat = program->read ((adr - 0x60000) % (128 * 1024));
	}
      break;
    case 9:
      dat = videoclass->read (false, adr - ((tmp2 & tmp) << 15));
      break;
    case 10:
      dat = videoclass->read (true, adr - ((tmp2 & tmp) << 15));
      break;
    default:
      dat = 255;
    }
  return dat;
}

t16
jioclass::memr (unsigned long addr)
{
  clk += 4 * 3;
  return memr_ (addr);
}

t16
jioclass::memrc (unsigned long addr)
{
  clk += 9;
  return memr_ (addr);
}

void
jioclass::memw (unsigned long addr, t16 v)
{
  t16 tmp, tmp2;
  int i;

  clk += 4 * 3;
  tmp = (addr >> 15) & 037;
  tmp2 = 0;
  for (i = 0; i <= 10; i++)
    {
      if (regs1ff[i].reg1 & 128)
	{
	  if ((regs1ff[i].reg1 & 32) && (regs1ff[i].reg2 & 64))
	    {
	      tmp2 = regs1ff[i].reg2 & 037;
	      tmp2 ^= 037;
	      if ((tmp2 & regs1ff[i].reg1) == (tmp2 & tmp))
		break;
	    }
	}
    }
  switch (i)
    {
    case 0:
      break;
    case 1:
      cartrom->write (32768 * 0 + (addr & 0x7fff), v);
      break;
    case 2:
      cartrom->write (32768 * 1 + (addr & 0x7fff), v);
      break;
    case 7:
      {
	t16 x = (addr & 0x3ffff) + 0x80000;

	if (x >= 0x88000 && x <= 0x887ff)
	  kanjirom->write (addr & 0x3ffff, v);
      }
      break;
    case 8:
      if (videoclass->pcjrmem ())
	{
	  if (addr < 0x20000)
	    program->write (addr & 0x1ffff, v);
	  else
	    mainram->write ((addr - 0x20000) % (384 * 1024), v);
	}
      else
	{
	  if (addr < 0x60000)
	    mainram->write (addr % (384 * 1024), v);
	  else
	    program->write ((addr - 0x60000) % (128 * 1024), v);
	}
      break;
    case 9:
      videoclass->write (false, addr - ((tmp2 & tmp) << 15), v);
      break;
    case 10:
      videoclass->write (true, addr - ((tmp2 & tmp) << 15), v);
      break;
    }
}

t16
jioclass::in (t16 n)
{
  t16 tmp, tmp2;
  int i;

  clk += 4 * 3;
  tmp = (n >> 3) & 0177;
  for (i = 11; i <= 30; i++)
    {
      if (i == 17)
	continue;
      if ((regs1ff[i].reg1 & 128) || i == 22)
	{
	  tmp2 = regs1ff[i].reg2 & 0177;
	  tmp2 ^= 0177;
	  if ((tmp2 & regs1ff[i].reg1) == (tmp2 & tmp))
	    break;
	}
    }
  switch (i-11+0x80)
    {
    case 0x80:
      {
	t16 a;

	a = read8259 (n);
	return a;
      }
      break;
    case 0x81:
      return soundclass->in8253 (n & 3);
      break;
    case 0x82:
      tmp = n & 7;
      if (tmp <= 1)
	return dat8255[tmp];
      return 2 | (kbd->getnmiflag () ? 1 : 0) |
	(kbd->getkeydata () ? 64 : 0) | /*(timer2flag ? 32 : 0);*/
	(soundclass->gettimer2out () ? 0x20 : 0);
      break;
    case 0x83:
      if (n == 0xa0)
	{
	  return kbd->in ();
	}
      break;
    case 0x85:		// FDC
      switch (n & 15)
	{
	case 4:
	  return fdc->inf4 ();
	  break;
	case 5:
	  return fdc->inf5 ();
	  break;
	}
      break;
    case 0x8b:
      if (n == 0x3da)
	{
	  t16 r = 0;
	  if (regs1ff[23].reg1 & 128)
	    r = videoclass->in3da (false);
	  if (regs1ff[24].reg1 & 128)
	    r = videoclass->in3da (true);
	  return r;
	}
      break;
    case 0x87:
      {
	t16 d;
	d = joyb;
	if (joyx)
	  d |= 1;
	if (joyy)
	  d |= 2;
	return d          | 3;
      }
      break;
    }
  
  if (n == 0x1ff)
    {
      status1ff = 0;
      return 255 ^ (pcjrmode ? 32 : 0);
    }
  return 0;
}

void
jioclass::out (t16 n, t16 v)
{
  t16 tmp, tmp2;
  int i;

  clk += 4 * 3;
  tmp = (n >> 3) & 0177;
  for (i = 11; i <= 30; i++)
    {
      if ((regs1ff[i].reg1 & 128) || i == 22)
	{
	  tmp2 = regs1ff[i].reg2 & 0177;
	  tmp2 ^= 0177;
	  if ((tmp2 & regs1ff[i].reg1) == (tmp2 & tmp))
	    break;
	}
    }
  switch (i-11+0x80)
    {
    case 0x80:
      write8259 (n, v);
      break;
    case 0x81:
      soundclass->out8253 (n & 3, v);
      break;
    case 0x82:
      tmp = n & 7;
      if (tmp <= 2)
	dat8255[tmp] = v;
      if (tmp == 1)
	soundclass->set8255b (v);
      break;
    case 0x83:
      kbd->out (v);
      soundclass->selecttimer1in (((v & 0x20) != 0) ? true : false);
      break;
    case 0x85:
      switch (n & 15)
	{
	case 2:
	  fdc->outf2 (v);
	  break;
	case 5:
	  fdc->outf5 (v);
	  break;
	}
      break;
    case 0x8a:
      if ((n&0xfff9) == 0x3d0)
	{
	  //mode3d0 = v;
	  videoclass->out3d4 (v);
	  break;
	}
      if ((n&0xfff9) == 0x3d1)
	{
	  videoclass->out3d5 (v);
	  break;
	}
      break;
    case 0x8b:		// 0x3d?
      if (n == 0x3da)
	{
	  if (regs1ff[23].reg1 & 128)
	    videoclass->out3da (false, v & 255);
	  if (regs1ff[24].reg1 & 128)
	    videoclass->out3da (true, v & 255);
	}
      if (n == 0x3d9)
	if (regs1ff[27].reg1 & 128)
	  videoclass->out3d9 (v);
      if (n == 0x3df)
	if (regs1ff[28].reg1 & 128)
	  videoclass->out3df (v);
      break;
    case 0x86:
      {
	/* clk は合計すると 1 秒間で 14318180 */
	// パルス幅  時間=24.2μ秒 + 0.011 * Rμ秒
	// R は抵抗値 [Ω], 0 〜 100 [kΩ]
	joyb = 12;
	joyx = 50;
	joyy = 50;
#ifdef forWin
	if (GetAsyncKeyState (VK_NUMPAD4))
	  joyx = 20;	// パルス幅
	if (GetAsyncKeyState (VK_NUMPAD6))
	  joyx = 80;
	if (GetAsyncKeyState (VK_NUMPAD8))
	  joyy = 20;
	if (GetAsyncKeyState (VK_NUMPAD2))
	  joyy = 80;
	if (GetAsyncKeyState (/*VK_Z*/'Z'))
	  joyb |= 16;
	if (GetAsyncKeyState (/*VK_X*/'X'))
	  joyb |= 32;
	if (GetAsyncKeyState ('A'))
	  {
	    static int f = 0;
	    f ^= 16;
	    joyb |= f;
	  }
#endif
	joyb ^= 0xf0;
	joyx = ((242 + 110 * joyx) * 1431818LL) / 1000000LL;
	joyy = ((242 + 110 * joyy) * 1431818LL) / 1000000LL;
      }
      break;
    case 0x84:
      soundclass->iowrite (v);
      clk += 32 * 4;		// SN76489A needs 32 cycles
      break;
    }

  if (n == 0x1ff)
    {
      t16 tmp;

      switch (status1ff)
	{
	case -1:
	  status1ff = -2;
	  break;
	case 0:
	  if (v >= 0 && v <= 10)
	    block1ff = v;
	  else if (v >= 0x80 && v <= 0x93)
	    block1ff = v - 0x80 + 11;
	  else
	    status1ff = -2;
	  break;
	case 1:
	  tmp = v;
	  tmp &= regs1ff[block1ff].andreg1;
	  tmp |= regs1ff[block1ff].orreg1;
	  regs1ff[block1ff].reg1 = tmp;
	  break;
	case 2:
	  tmp = v;
	  tmp &= regs1ff[block1ff].andreg2;
	  tmp |= regs1ff[block1ff].orreg2;
	  regs1ff[block1ff].reg2 = tmp;
	  status1ff = -1;
	  break;
	}
      status1ff++;
    }
  if (n >= 0x10 && n <= 0x12)
    {
      if (n == 0x10 && v == 0xf1)
	;//allowdisplay = 1;
      if (n == 0x10 && v == 0xff)
	// Workaround for BIOS POST which accesses the joystick port
	joyb = 0xf0;
#define z(a,b) case 0x##a: fprintf (stderr, "MFG: %s\n", b); break
      if (n == 0x10)
	{
	  switch (v)
	    {
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
	    default:
	      fprintf (stderr, "MFG: port 0x%x code 0x%02x\n", n, v);
	    }
	}
#undef Z
      if (n == 0x11 || n == 0x12)
	fprintf (stderr, "MFG port 0x%x code 0x%02x\n", n, v);
    }
}

bool
jioclass::timerset (int clk, bool redraw)
{
  int d;
  bool retnmiflag;

  retnmiflag = false;
  d = clk;			// clk is 14.31818MHz
  videoclass->clk (d, redraw);
  retnmiflag = kbd->clkin (clk);
  if (d > joyx)
    joyx = 0;
  else
    joyx -= d;
  if (d > joyy)
    joyy = 0;
  else
    joyy -= d;
  soundclass->clk (d);
  return retnmiflag;
}

