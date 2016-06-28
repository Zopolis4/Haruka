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

#include "jtype.hh"
#include "jmem.hh"
#include "jvideo.hh"

using std::cerr;
using std::endl;

extern "C"
{
  extern void trigger_irq8259 (unsigned int irq_no);
  extern void untrigger_irq8259 (unsigned int irq_no);
}

jvideo::~jvideo ()
{
  SDL_FreeSurface (mysurface2);
  SDL_FreeSurface (mysurface);
  SDL_FreePalette (mypalette);
}

void
jvideo::clk (int clockcount, bool drawflag)
{
  // 14.31818 MHz  59.92 Hz  15.700 KHz
  // +------------------------------------+ -------
  // |                                    |  912*31?
  // |                                    |
  // |     +------------------------+     | -------
  // |     |                        |     |
  // |     |                        |     | 912*200
  // |     |                        |     |
  // |     +------------------------+     | -------
  // |                                    |
  // |                                    |  912*31?
  // +------------------------------------+ -------
  // | 136 |           640          | 136 |
  // 0 ... 500  vertical interrupt
  // 0 ... 912*10  vertical retrace on
  // 912*10 ... 912*31-68  vertical off
  // 912*n-68 ... 912*n+68  enables on (n = 31 ... 230)
  // 912*n+68 ... 912*n+136  enables off (n = 31 ... 230)
  // 912*n+136 ... 912*n+776  dots (n = 31 ... 230)
  // 912*n+776 ... 912*n+844  dummy (n = 31 ... 230)
  // 912*231-68 ... 912*262  dummy
  conv (clockcount, drawflag);
  blinkcount += clockcount;
  while (blinkcount >= (14318180 / 2))
    blinkcount -= (14318180 / 2);
}

unsigned int
jvideo::in3da (bool vp2)
{
  flag3da[vp2] = 0;		// Set address state; FIXME: Is the
				// state managed in VP1 and VP2
				// separately?
  unsigned int ret = 0x6;	// FIXME: Light pen not implemented
  // Maybe some bits in VP2 status register are not available.
  if (crtc.get_disp ())
    ret |= 0x1;
  if (crtc.get_vsync ())
    ret |= 0x8;
  if (last_color & (1 << (v3da & 0x3)))
    ret |= 0x10;
  return ret;
}

void
jvideo::out3da (bool vp2, unsigned char data)
{
  int vp;

  vp = 0;
  if (vp2)
    vp = 1;
  if (!flag3da[vp])
    {
      v3da = data;
      flag3da[vp] = 1;
      return;
    }
  flag3da[vp] = 0;
  switch (v3da)
    {
    case 0:
      mode1[vp] = data;
      break;
    case 1:
      palettemask[vp] = data;
      break;
    case 2:
      bordercolor = data & 0xf;
      break;
    case 3:
      mode2[vp] = data;
      break;
    case 4:
      reset = data;
      break;
    case 5:
      transpalette = data;
      break;
    case 6:
      superimpose = data;
      break;
    case 0x10 ... 0x1f:
      palette[v3da & 15] = data & 15;
      break;
    }
}

static void
draw1 (unsigned char **p, unsigned char d)
{
  *(*p)++ = d;
}

static void
draw2 (unsigned char **p, unsigned char d)
{
  draw1 (p, d);
  draw1 (p, d);
}

static unsigned char *
draw_ma (unsigned char *p, int vp, unsigned int ma, unsigned int ra,
	 unsigned int gma, unsigned int gra, bool cursor, jmem &readmem,
	 jmem &kanjirom, unsigned int readtop, unsigned int mode1,
	 unsigned int mode2, unsigned int blinkcount)
{
  if (!(mode1 & 2))		// Text mode
    {
      if (vp && (mode1 & 0x40))	// Kanji mode
	{
	  unsigned int voff = readtop | ((ma << 1) & 0x3fff);
	  unsigned int d1 = readmem.read (voff);
	  unsigned int d2 = readmem.read (voff | 1);
	  unsigned int bg = (d2 / 16) % 8;
	  unsigned int fg = d2 % 8;
	  unsigned int addr;
	  if (!(d2 & 128))
	    addr = d1 << 5;
	  else
	    {
	      int code, right;
	      unsigned int d3;
	      if (!(d2 & 8))
		{
		  d3 = readmem.read (readtop | (((ma << 1) + 2) & 0x3fff));
		  code = d1 * 256 + d3;
		  right = 0;
		}
	      else
		{
		  d3 = readmem.read (readtop | (((ma << 1) - 2) & 0x3fff));
		  code = d3 * 256 + d1;
		  right = 1;
		}
	      if (code >= 0xf000) // Gaiji support
		{	// FIXME: hardware compares address?
		  code &= 0x3f;
		  code |= 0x8400;
		}
	      addr = ((code & 0x1fff) << 5) + right;
	    }
	  unsigned int d = 0;
	  if (cursor)
	    d = 0xff;
	  else if (ra < 16)
	    d = kanjirom.read (addr | (ra << 1));
	  if (mode1 & 1)
	    for (int n = 0; n < 8; n++, d <<= 1)
	      draw1 (&p, (d & 128) ? fg : bg);
	  else
	    for (int n = 0; n < 8; n++, d <<= 1)
	      draw2 (&p, (d & 128) ? fg : bg);
	}
      else			// 25 lines
	{
	  unsigned int voff = readtop | ((ma << 1) & 0x3fff);
	  unsigned int d1 = readmem.read (voff);
	  unsigned int d2 = readmem.read (voff | 1);
	  unsigned int bg = (d2 / 16) % 16;
	  unsigned int fg = d2 % 16;
	  if (mode2 & 2)	// Blinking enabled
	    {
	      bg %= 8;
	      // FIXME: blink rate and timing
	      if ((d2 & 0x80) && blinkcount < (14318180 / 4))
		fg = bg;
	    }
	  unsigned int addr = (d1 << 5) + 1 + (vp ? 0 : 16);
	  unsigned int d = 0;
	  if (cursor)
	    d = 0xff;
	  else if (ra < 8)
	    d = kanjirom.read (addr | (ra << 1));
	  if (mode1 & 1)
	    for (int n = 0; n < 8; n++, d <<= 1)
	      draw1 (&p, (d & 128) ? fg : bg);
	  else
	    for (int n = 0; n < 8; n++, d <<= 1)
	      draw2 (&p, (d & 128) ? fg : bg);
	}
    }
  else			// Graphic mode
    {
      int c;
      if (mode1 & 16)
	c = 4;
      else if (mode2 & 8)
	c = 1;
      else
	c = 2;
      unsigned int voff;
      if (mode1 & 1)
	voff = (readtop & ~0x7fff) | ((gra << 13) & 0x6000) |
	  ((gma << 1) & 0x1fff);
      else
	voff = (readtop & ~0x3fff) | ((gra << 13) & 0x2000) |
	  ((gma << 1) & 0x1fff);
      unsigned int d1 = readmem.read (voff);
      unsigned int d2 = readmem.read (voff | 1);
      switch (c)
	{
	case 1:
	  for (int n = 0; n < 8; n++, d1 <<= 1)
	    draw1 (&p, (d1 & 0x80) >> 7);
	  for (int n = 0; n < 8; n++, d2 <<= 1)
	    draw1 (&p, (d2 & 0x80) >> 7);
	  break;
	case 2:
	  if (mode1 & 1)
	    for (int n = 0; n < 8; n++, d1 <<= 1, d2 <<= 1)
	      draw1 (&p, ((d1 & 0x80) >> 7) | ((d2 & 0x80) >> 6));
	  else
	    {
	      for (int n = 0; n < 4; n++, d1 <<= 2)
		draw2 (&p, (d1 & 0xc0) >> 6);
	      for (int n = 0; n < 4; n++, d2 <<= 2)
		draw2 (&p, (d2 & 0xc0) >> 6);
	    }
	  break;
	case 4:
	  if (mode1 & 1)
	    {
	      draw2 (&p, d1 >> 4);
	      draw2 (&p, d1 & 0xf);
	      draw2 (&p, d2 >> 4);
	      draw2 (&p, d2 & 0xf);
	    }
	  else
	    {
	      draw2 (&p, d1 >> 4);
	      draw2 (&p, d1 >> 4);
	      draw2 (&p, d1 & 0xf);
	      draw2 (&p, d1 & 0xf);
	      draw2 (&p, d2 >> 4);
	      draw2 (&p, d2 >> 4);
	      draw2 (&p, d2 & 0xf);
	      draw2 (&p, d2 & 0xf);
	    }
	  break;
	}
    }
  return p;
}

void
jvideo::convtick (bool disp)
{
  // Manage graphic screen address itself instead of using CRTC
  // output directly to support superimpose with VP2 text screen
  crtc.tick ();
  if (disp)
    {
      gma1++;
      gma2++;
      if (!crtc.get_disp ())
	{
	  gra1++;
	  gra2++;
	  if (gra1 == (mode1[0] & 1 ? 4 : 2))
	    {
	      gma10 = gma1;
	      gra1 = 0;
	    }
	  else
	    gma1 = gma10;
	  if (gra2 == (mode1[1] & 1 ? 4 : 2))
	    {
	      gma20 = gma2;
	      gra2 = 0;
	    }
	  else
	    gma2 = gma20;
	}
    }
}

void
jvideo::convsub (int readtop1, int readtop2, int enable1, int enable2, int si,
		 int len, unsigned char *p, bool disp)
{
  if (disp)
    {
      unsigned char buf1[16], buf2[16];
      if ((!enable1 && si == 0x0) || (!enable2 && si == 0x1))
	{
	  memset (p, bordercolor, len);
	  goto disabled;
	}
      if (enable1 && si != 0x1)
	draw_ma (buf1, 0, crtc.get_ma (), crtc.get_ra (), gma1, gra1,
		 crtc.get_cursor (true), program, kanjirom, readtop1,
		 mode1[0], mode2[0], blinkcount);
      if (enable2 && si != 0x0)
	draw_ma (buf2, 1, crtc.get_ma (), crtc.get_ra (), gma2, gra2,
		 crtc.get_cursor (true), vram, kanjirom, readtop2,
		 mode1[1], mode2[1], blinkcount);
      if (si & 0xe)
	{
	  if (!enable1)
	    memset (&buf1[0], 0, len);
	  if (!enable2)
	    memset (&buf2[0], 0, len);
	}
      switch (si)
	{
	case 0x0:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i]];
	  break;
	case 0x1:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf2[i]];
	  break;
	case 0x2:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i] == transpalette ? buf2[i] : buf1[i]];
	  break;
	case 0x3:		// FIXME: Is this correct?
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i] != transpalette ? buf2[i] : buf1[i]];
	  break;
	case 0x6:
	  if (mode1[1] & 0x80)
	    {
	      for (int i = 0; i < len; i++)
		p[i] = palette[(buf1[i] | (buf2[i] << 2)) ^ 0xa];
	      break;
	    }
	  // Fall through
	case 0x4:
	case 0x5:
	case 0x7:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i] ^ buf2[i]];
	  break;
	case 0x8:
	case 0x9:
	case 0xa:
	case 0xb:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i] & buf2[i]];
	  break;
	case 0xc:
	case 0xd:
	case 0xe:
	case 0xf:
	  for (int i = 0; i < len; i++)
	    p[i] = palette[buf1[i] | buf2[i]];
	  break;
	}
    disabled:
      last_color = p[len - 1];
      convtick (true);
    }
  else
    {
      if (p)
	memset (p, bordercolor, len);
      convtick (crtc.get_disp ());
    }
}

void
jvideo::conv (int clockcount, bool drawflag)
{
  int readtop1 = (pagereg[0] & 7) * 16384;
  int readtop2 = (pagereg[1] & 3) * 16384;
  int enable1 = mode1[0] & 8;
  int enable2 = mode1[1] & 8;
  int si = superimpose & 0xf;
  bool len8 = (si != 0x1 && (mode1[0] & 1)) || (si != 0x0 && (mode1[1] & 1));
  unsigned int len = len8 ? 8 : 16;
  convcount += clockcount;
  if (convcount < len)
    return;
  if (SDL_MUSTLOCK (mysurface))
    SDL_LockSurface (mysurface);
  while (convcount >= len)
    {
      bool hsync = crtc.get_hsync ();
      bool vsync = crtc.get_vsync ();
      bool disp = crtc.get_disp ();
      if (disp && gma_reset)
	{
	  gma1 = gma2 = gma10 = gma20 = crtc.get_ma ();
	  gra1 = gra2 = 0;
	  gma_reset = false;
	}
      if (disp && draw_y < 200 && draw_x + len <= 640)
	{
	  convsub (readtop1, readtop2, enable1, enable2, si, len,
		   static_cast<unsigned char *> (mysurface->pixels) +
		   draw_y * mysurface->pitch + draw_x, true);
	  draw_x += len;
	}
      else
	convsub (readtop1, readtop2, enable1, enable2, si, len, NULL, false);
      if (draw_x && hsync)
	{
	  for (; draw_x < 640; draw_x++)
	    *(static_cast<unsigned char *> (mysurface->pixels) +
	      draw_y * mysurface->pitch + draw_x) = bordercolor;
	  draw_x = 0;
	  draw_y++;
	}
      if (vsync)
	{
	  if (!vsynccount)
	    {
	      if (drawflag)
		{
		  for (; draw_y < 200; draw_y++, draw_x = 0)
		    for (; draw_x < 640; draw_x++)
		      *(static_cast<unsigned char *> (mysurface->pixels) +
			draw_y * mysurface->pitch + draw_x) = bordercolor;
		  if (SDL_MUSTLOCK (mysurface))
		    SDL_UnlockSurface (mysurface);
		  draw ();
		  if (SDL_MUSTLOCK (mysurface))
		    SDL_LockSurface (mysurface);
		}
	      trigger_irq8259 (5);
	      draw_x = draw_y = 0;
	      gma_reset = true;
	    }
	  vsynccount += len;
	  if (vsynccount == 0x200) // FIXME: Interrupt trigger for 512 cycles?
	    untrigger_irq8259 (5);
	}
      else
	{
	  if (vsynccount)
	    {
	      if (vsynccount < 0x200)
		untrigger_irq8259 (5);
	      vsynccount = 0;
	    }
	}
      convcount -= len;
    }
  if (SDL_MUSTLOCK (mysurface))
    SDL_UnlockSurface (mysurface);
}

void
jvideo::floppyaccess (int n)
{
}

jvideo::jvideo (SDL_Window *window, SDL_Surface *surface, jmem &program_arg,
		jmem &kanjirom_arg) throw (char *)
  : program (program_arg), vram (65536), kanjirom (kanjirom_arg)
{
  static SDL_Color cl[16]={
    {r:0x00,g:0x00,b:0x00},
    {r:0x00,g:0x00,b:0xdd},
    {r:0x00,g:0xdd,b:0x00},
    {r:0x00,g:0xdd,b:0xdd},
    {r:0xdd,g:0x00,b:0x00},
    {r:0xdd,g:0x00,b:0xdd},
    {r:0xdd,g:0xdd,b:0x00},
    {r:0xdd,g:0xdd,b:0xdd},
    {r:0x88,g:0x88,b:0x88},
    {r:0x88,g:0x88,b:0xff},
    {r:0x88,g:0xff,b:0x88},
    {r:0x88,g:0xff,b:0xff},
    {r:0xff,g:0x88,b:0x88},
    {r:0xff,g:0x88,b:0xff},
    {r:0xff,g:0xff,b:0x88},
    {r:0xff,g:0xff,b:0xff},
  };
  int i;

  blinkcount = 0;
  flag3da[0] = flag3da[1] = 0;
  pagereg[0] = pagereg[1] = 0;
  for (i = 0 ; i < 16 ; i++)
    palette[i] = 0;
  convcount = 0;
  draw_x = draw_y = 0;
  gma_reset = true;
  vsynccount = 0;

  jvideo::window = window;
  jvideo::surface = surface;
  if (!surface)
    {
      cerr << "SDL_SetVideoMode failed: " << SDL_GetError () << endl;
      throw ("video");
    }
  mypalette = SDL_AllocPalette (256);
  mysurface = SDL_CreateRGBSurface (0, 640, 200, 8, 0, 0, 0, 0);
  SDL_SetSurfacePalette (mysurface, mypalette);
  SDL_SetPaletteColors (mypalette, cl, 0, 16);
  // SDL_BlitScaled seems not working with 8bit depth surface so
  // create another 32bit depth surface and copy twice to blit :-)
  mysurface2 = SDL_CreateRGBSurface (0, 640, 200, 32, 0, 0, 0, 0);
}

void
jvideo::draw ()
{
  SDL_BlitSurface (mysurface, NULL, mysurface2, NULL);
  SDL_BlitScaled (mysurface2, NULL, surface, NULL);
  SDL_UpdateWindowSurface (window);
}

////////////////////////////////////////////////////////////
// Video
//
// I/O ports
//   Page register 1: 0x3df
//   Page register 2: 0x3d9
// VSYNC ... from 46505
// HSYNC ... from 46505
// 46505.MA 13bit 0-0x1fff
// 46505.RA  5bit 0-0x1f
// VP1
//   CRT page select: page register 1 bit0-2
//   CPU page select: page register 1 bit3-5
//   No video output (mode control 1 bit3 = 0)
//   Graphic mode (mode control 1 bit1 = 1)
//     VRAM offset: ((CRT page) * 0x4000) + (46505.RA * 0x2000) + 46505.MA
//     Low bandwidth  (page register 1 bit7,6 = 0,1 & mode control 1 bit0 = 0)
//       46505.RA 0-1
//     High bandwidth (page register 1 bit7,6 = 1,1 & mode control 1 bit0 = 1)
//       46505.RA 0-3
//       64KB RAM card requirement
//     16 colors (mode control 1 bit4 = 1)
//   Text mode (mode control 1 bit1 = 0)
//     Page register 1 bit 7,6 = 0,0
//     VRAM offset (character): ((CRT page) * 0x4000) + (46505.MA * 2)
//     VRAM offset (attribute): ((CRT page) * 0x4000) + (46505.MA * 2) + 1
//     FONT ROM (CG1) offset: (VRAM character code) * 0x8 + 46505.RA
//     46505.RA 0-7
//     Low bandwidth  (mode control 1 bit0 = 0)
//     High bandwidth (mode control 1 bit0 = 1)
//       64KB RAM card requirement
// VP2
//   VP1-VRAM1 (palette mask register bit4 = 1)
//   VP2-VRAM2 (palette mask register bit5 = 1)
//   VP3-VRAM2 (palette mask register bit6 = 1)
//   Extended mode (palette mask register bit7 = 1)
//   CRT page select: page register 1 bit0-1
//   CPU page select: page register 1 bit3-4
//   No video output (mode control 1 bit3 = 0)
//   Graphic mode (mode control 1 bit1 = 1)
//     VRAM offset: ((CRT page) * 0x4000) + (46505.RA * 0x2000) + 46505.MA
//     Low bandwidth  (page register 1 bit7,6 = 0,1 & mode control 1 bit0 = 0)
//       46505.RA 0-1
//     High bandwidth (page register 1 bit7,6 = 1,1 & mode control 1 bit0 = 1)
//       46505.RA 0-3
//     16 colors (except 640x200) (mode control 1 bit4 = 1)
//     640x200 16 colors (mode control 1 bit7 = 1)
//   Text mode (mode control 1 bit1 = 0)
//     Page register 1 bit 7,6 = 0,0
//     VRAM offset (character): ((CRT page) * 0x4000) + (46505.MA * 2)
//     VRAM offset (attribute): ((CRT page) * 0x4000) + (46505.MA * 2) + 1
//     CG2 offset: complex
//     Low bandwidth  (mode control 1 bit0 = 0)
//     High bandwidth (mode control 1 bit0 = 1)
//     Kanji (mode control 1 bit6 = 1)



////////////////////////////////////////////////////////////
// HD46505
//
//  1. GND     2. /RESET  3. LPSTB   4. MA0     5. MA1
//  6. MA2     7. MA3     8. MA4     9. MA5    10. MA6
// 11. MA7    12. MA8    13. MA9    14. MA10   15. MA11
// 16. MA12   17. ---    18. DSPT   19. CURSOR 20. Vcc
// 21. CLK    22. R / /W 23. E      24. RS     25. /CS
// 26. D7     27. D6     28. D5     29. D4     30. D3
// 31. D2     32. D1     33. D0     34. RA4    35. RA3
// 36. RA2    37. RA1    38. RA0    39. HSYNC  40. VSYNC
//
//  2: pull up
// 17: connected to nothing

enum
  {			      // Registers
    HTOTAL,		      // 00: Horizontal Total
    HDISP,		      // 01: Horizontal Displayed
    HSYNCPOS,		      // 02: Horizontal Sync Position
    HSYNCWIDTH,		      // 03: Horizontal Sync Width
    VTOTAL,		      // 04: Vertical Total
    VTOTALADJ,		      // 05: Vertical Total Adjust
    VDISP,		      // 06: Vertical Displayed
    VSYNCPOS,		      // 07: Vertical SyncPosition
    INTERMODE,		      // 08: Interlace Mode
    MAXSCANLINE,	      // 09: Maximum Scan Line
    CURSTART,		      // 0A: Cursor Start
    CUREND,		      // 0B: Cursor End
    SADDRHIGH,		      // 0C: Start Address High
    SADDRLOW,		      // 0D: Start Address Low
    CURLOCHIGH,		      // 0E: Cursor Location High
    CURLOCLOW,		      // 0F: Cursor Location Low
    LPENHIGH,		      // 10: Light Pen High
    LPENLOW,		      // 11: Light Pen Low
  };

jvideo::j46505::j46505 ()
{
  int i;

  for (i = 0; i < NREG; i++)
    reg[i] = 0;
  ix = 0;
  iy = 0;
  ira = 0;
  cur_addr = 0;
}

unsigned int
jvideo::j46505::inb (unsigned int addr)
{
  if ((addr & 1) != 0)
    return get_reg (cur_addr);
  else
    return cur_addr;
}

void
jvideo::j46505::outb (unsigned int addr, unsigned int val)
{
  if ((addr & 1) != 0)
    set_reg (cur_addr, val);
  else
    cur_addr = val & 0xff;
}

unsigned int
jvideo::j46505::get_reg (unsigned int idx)
{
  switch (idx)
    {
    case SADDRHIGH:
    case SADDRLOW:
    case CURLOCHIGH:
    case CURLOCLOW:
    case LPENHIGH:
    case LPENLOW:
      return reg[idx];
    }
  return 0xff;
}

void
jvideo::j46505::set_reg (unsigned int idx, unsigned int val)
{
  switch (idx)
    {
    case HTOTAL:
    case HDISP:
    case HSYNCPOS:
    case SADDRLOW:
    case CURLOCLOW:
      reg[idx] = val & 0xff;
      break;
    case VTOTAL:
    case VDISP:
    case VSYNCPOS:
    case CURSTART:
      reg[idx] = val & 0x7f;
      break;
    case SADDRHIGH:
    case CURLOCHIGH:
      reg[idx] = val & 0x3f;
      break;
    case VTOTALADJ:
    case MAXSCANLINE:
    case CUREND:
      reg[idx] = val & 0x1f;
      break;
    case HSYNCWIDTH:
      reg[idx] = val & 0xf;
      break;
    case INTERMODE:
      reg[idx] = val & 0x3;
      break;
    }
}

unsigned int
jvideo::j46505::get_ma ()
{
  return iy * reg[HDISP] + ix + (reg[SADDRLOW] | reg[SADDRHIGH] << 8);
}

unsigned int
jvideo::j46505::get_ra ()
{
  return ira;
}

bool
jvideo::j46505::get_cursor (bool force_blink)
{
  if (!get_disp ())
    return false;
  if (get_ma () != (reg[CURLOCLOW] | reg[CURLOCHIGH] << 8))
    return false;
  if (ira >= (reg[CURSTART] & 0x1f) && ira <= reg[CUREND])
    {
      switch (reg[CURSTART] & 0x60)
	{
	case 0x20:			// Non-display
	  return false;
	case 0x00:			// Non-blink
	  if (!force_blink)
	    return true;
	  // Fall through
	case 0x40:			// Blink, 1/16 field rate
	  if ((framecount & 0xf) <= 0x7)
	    return true;
	  else
	    return false;
	case 0x80:			// Blink, 1/32 field rate
	  if ((framecount & 0x1f) <= 0xf)
	    return true;
	  else
	    return false;
	}
    }
  return false;
}

bool
jvideo::j46505::get_disp ()
{
  if (ix < reg[HDISP] && iy < reg[VDISP])
    return true;
  else
    return false;
}

bool
jvideo::j46505::get_hsync ()
{
  if (ix >= reg[HSYNCPOS] && ix - reg[HSYNCPOS] < reg[HSYNCWIDTH])
    return true;
  else
    return false;
}

bool
jvideo::j46505::get_vsync ()
{
  if (iy >= reg[VSYNCPOS] &&
      (iy - reg[VSYNCPOS]) * (reg[MAXSCANLINE] + 1) + ira < 16)
    return true;
  else
    return false;
}

void
jvideo::j46505::tick ()
{
  ix++;
  if (ix > reg[HTOTAL])
    {
      ix = 0;
      ira++;
      if (iy <= reg[VTOTAL] && ira > reg[MAXSCANLINE])
	{
	  ira = 0;
	  iy++;
	}
      if (iy > reg[VTOTAL] && ira >= reg[VTOTALADJ])
	{
	  ira = 0;
	  iy = 0;
	  framecount++;
	}
    }
}

void
jvideo::out3d4 (unsigned char data)
{
  crtc.outb (0, data);
}

unsigned char
jvideo::in3d5 ()
{
  return crtc.inb (1);
}

void
jvideo::out3d5 (unsigned char data)
{
  crtc.outb (1, data);
}
