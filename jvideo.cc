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

jvideo::jvideo (jmem *program, jmem *kanjirom)
{
  int i;

  jvideo::program = program;
  jvideo::kanjirom = kanjirom;
  drawdata = new unsigned char[640 * 200];
  drawdata1 = new unsigned char[640 * 200];
  drawdata2 = new unsigned char[640 * 200];
  vram = new jmem (65536);
  vsynccount = 0;
  cursorcount = 0;
  vsyncintflag = 0;
  dat3da = 0;
  flag3da[0] = flag3da[1] = 0;
  pagereg[0] = pagereg[1] = 0;
  for (i = 0 ; i < 16 ; i++)
    palette[i] = 0;
}

jvideo::~jvideo ()
{
  SDL_FreeSurface (mysurface2);
  SDL_FreeSurface (mysurface);
  SDL_FreePalette (mypalette);
  delete vram;
  delete[] drawdata;
  delete[] drawdata1;
  delete[] drawdata2;
}

unsigned char
jvideo::read2 (int offset)
{
  return vram->read ((offset + ((pagereg[1] >> 3) & 3) * 16384) & 65535);
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
  vsynccount += clockcount;
  cursorcount += clockcount;
  blinkcount += clockcount;
  while (cursorcount >= (14318180 / 4))
    cursorcount -= (14318180 / 4);
  while (blinkcount >= (14318180 / 2))
    blinkcount -= (14318180 / 2);
  if (vsynccount > 912 * 262)
    {
      vsynccount -= 912 * 262, dat3da |= 8, vsyncintflag = 1,
	trigger_irq8259 (5);
      if (drawflag)
	{
	  conv ();
	  draw ();
	}
    }
  if (vsyncintflag && vsynccount > 500)
    vsyncintflag = 0, untrigger_irq8259 (5);
  if (vsynccount > 912 * 10)
    {
      dat3da &= ~8;
      hsynccount = (vsynccount + 456) / 912;
      if (31 <= hsynccount && hsynccount <= 230)
	{
	  if (912 * hsynccount - 136 < vsynccount &&
	      vsynccount < 912 * hsynccount + 136)
	    dat3da |= 1;
	  else
	    dat3da &= ~1;
	}
    }
#if 0
  if (vsynccount >= (14318180 / 60))
    {
      vsynccount -= 14318180 / 60;
      dat3da &= 1^255;
      dat3da |= 8;
      vsyncintflag = 1, trigger_irq8259 (5);
      if (drawflag)
	{
	  conv ();
	  draw ();
	}
    }
  if (vsyncintflag)
    {
#if 0
      if (vsynccount > 500)
	{
	  vsyncintflag = 0;
	  hsynccount = 0;
	  dat3da &= 9^255;
	  /* intclass->intcallor (32); */
	  trigger_irq8259 (5);
	}
#endif
      if (vsynccount > 500 && vsyncintflag == 1)
	{
	  vsyncintflag = 2;
	  /* intclass->intcallor (32); */
	  untrigger_irq8259 (5);
	}
      if (vsynccount > 1000 && vsyncintflag == 2)
	{
	  vsyncintflag = 0;
	  hsynccount = 0;
	  dat3da &= 9^255;
	}
    }
  if (!(dat3da & 1))
    if (vsynccount > (long long)((hsynccount + 1) * 14318180LL / 60 / 201))
      dat3da |= 1;
  if (dat3da & 1)
    if (vsynccount >
	(long long)((hsynccount + 1) * 14318180LL / 60 / 201 + 600))
      {
	dat3da &= 1^255;
	hsynccount++;
      }
#endif
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
      bordercolor = data;
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

int
jvideo::convsub (unsigned char *p, int vp)
{
  jmem *readmem;
  int maskdat;
  int readtop;
  bool fillbottom;
  unsigned char ctmp;

  fillbottom = false;
  readmem = vmem (vp);
  maskdat = vmask (vp);
  readtop = vtop (vp);
  if (!(mode1[vp] & 8))
    return 2;
  if (!(mode1[vp] & 2))		// Text mode
    {
      bool x80, y25;

      x80 = y25 = false;
      if (!vp || !(mode1[1] & 64))
	y25 = true;
      if (mode1[vp] & 1)
	x80 = true;
      if (y25 == false)
	{
	  int i, j, k, l, bg, fg, d1, d2, addr, m, n, drawaddr, d, e, d3;

	  fillbottom = true;
	  k = readtop;
	  l = x80 ? 80 : 40;
	  for (i = 0 ; i < 11 ; i++)
	    {
	      drawaddr = i * 640 * 18;
	      for (j = 0 ; j < l ; j++)
		{
		  d1 = readmem->read (k & maskdat);
		  d2 = readmem->read ((k + 1) & maskdat);
		  bg = (d2 / 16) % 8;
		  fg = d2 % 8;
		  if (!(d2 & 128))
		    addr = d1 << 5;
		  else
		    {
		      if (!(d2 & 8))
			{
			  d3 = readmem->read ((k + 2) & maskdat);
			  addr = ((d1 * 256 + d3) & 0x1fff) << 5;
			}
		      else
			{
			  d3 = readmem->read ((k-2) & maskdat);
			  addr = (((d3 * 256 + d1) & 0x1fff) << 5) + 1;
			}
		    }
		  for (m = 0 ; m < 16 ; m++)
		    {
		      e = drawaddr + 640 * m;
		      d = kanjirom->read (addr);
		      if (x80 == true)
			for (n = 0 ; n < 8 ; n++)
			  p[e++] = (d & 128) ? fg : bg, d *= 2;
		      else
			for (n = 0 ; n < 8 ; n++)
			  p[e] = p[e + 1] = (d & 128) ? fg : bg, d *= 2,
			    e += 2;
		      addr += 2;
		    }
		  for (; m < 18 ; m++)
		    {
		      e = drawaddr + 640 * m;
		      if (x80 == true)
			for (n = 0 ; n < 8 ; n++)
			  p[e++] = bg;
		      else
			for (n = 0 ; n < 8 ; n++)
			  p[e] = p[e + 1] = bg, e += 2;
		    }
		  drawaddr += x80 ? 8 : 16;
		  k += 2;
		}
	    }
	  if (cursorcount < (14318180 / 8))
	    {
	      i = crtcdata[14] * 256 + crtcdata[15]
		- (crtcdata[12] * 256 + crtcdata[13]);
	      j = (i / l) * 640 * 18 - 1;
	      j += (i % l) * (x80 ? 8 : 16);
	      i = crtcdata[10];
	      if ((i & 96) != 32)
		{
		  if (i < 0)
		    i = 0;
		  if (i > 17)
		    i = 17;
		  j += i * 640;
		  n = crtcdata[11];
		  if (n > 17)
		    n = 17;
		  for (; i <= n ; i++)
		    {
		      for (k = x80 ? 8 : 16 ; k > 0 ; k--)
			p[(j + k) % (640 * 200)] = 15;
		      j += 640;
		    }
		}
	    }
	}
      else			// 25 lines
	{
	  int i, j, k, l, bg, fg, d1, d2, addr, m, n, drawaddr, d, e;

	  k = readtop;
	  l = x80 ? 80 : 40;
	  for (i = 0 ; i < 25 ; i++)
	    {
	      drawaddr = i * 640 * 8;
	      for (j = 0 ; j < l ; j++)
		{
		  d1 = readmem->read (k & maskdat);
		  d2 = readmem->read ((k + 1) & maskdat);
		  bg = (d2 / 16) % 16;
		  fg = d2 % 16;
		  if (mode2[vp] & 2) // Blinking enabled
		    {
		      bg %= 8;
		      // FIXME: blink rate and timing
		      if ((d2 & 0x80) && blinkcount < (14318180 / 4))
			fg = bg;
		    }
		  addr = (d1 << 5) + 1 + (vp ? 0 : 16);
		  if (vp)
		    {
		      for (m = 0 ; m < 8 ; m++)
			{
			  e = drawaddr + 640 * m;
			  d = kanjirom->read (addr);
			  if (x80 == true)
			    for (n = 0 ; n < 8 ; n++)
			      p[e++] = (d & 128) ? fg : bg, d *= 2;
			  else
			    for (n = 0 ; n < 8 ; n++)
			      p[e] = p[e + 1] = (d & 128) ? fg : bg, d *= 2,
				e += 2;
			  addr += 2;
			}
		      for (; m < 8 ; m++)
			{
			  e = drawaddr + 640 * m;
			  if (x80 == true)
			    for (n = 0 ; n < 8 ; n++)
			      p[e++] = bg;
			  else
			    for (n = 0 ; n < 8 ; n++)
			      p[e] = p[e + 1] = bg, e += 2;
			}
		    }
		  else
		    {
		      for (m = 0 ; m < 8 ; m++)
			{
			  e = drawaddr + 640 * m;
			  d = kanjirom->read (addr);
			  if (x80 == true)
			    for (n = 0 ; n < 8 ; n++)
			      p[e] &= 15, p[e++] |= ((d & 128) ? fg : bg) << 4,
				d *= 2;
			  else
			    for (n = 0 ; n < 8 ; n++)
			      ctmp = ((d & 128) ? fg : bg) << 4, p[e] = (p[e] & 15) | ctmp, p[e + 1] = (p[e + 1] & 15) | ctmp, d *= 2,
				e += 2;
			  addr += 2;
			}
		      for (; m < 8 ; m++)
			{
			  e = drawaddr + 640 * m;
			  if (x80 == true)
			    for (n = 0 ; n < 8 ; n++)
			      p[e] &= 15, p[e++] |= bg << 4;
			  else
			    for (n = 0 ; n < 8 ; n++)
			      ctmp = bg << 4, p[e] = (p[e] & 15) | ctmp, p[e + 1] = (p[e + 1] & 15) | ctmp, e += 2;
			}
		    }
		  drawaddr += x80 ? 8 : 16;
		  k += 2;
		}
	    }
	  if (cursorcount < (14318180 / 8))
	    {
	      i = crtcdata[14] * 256 + crtcdata[15]
		- (crtcdata[12] * 256 + crtcdata[13]);
	      j = (i / l) * 640 * 8 - 1;
	      j += (i % l) * (x80 ? 8 : 16);
	      i = crtcdata[10];
	      if ((i & 96) != 32)
		{
		  if (i < 0)
		    i = 0;
		  if (i > 8)
		    i = 8;
		  j += i * 640;
		  n = crtcdata[11];
		  if (n > 8)
		    n = 8;
		  if (vp)
		    {
		      for (; i <= n ; i++)
			{
			  for (k = x80 ? 8 : 16 ; k > 0 ; k--)
			    p[(j + k) % (640 * 200)] = 15;
			  j += 640;
			}
		    }
		  else
		    {
		      for (; i <= n ; i++)
			{
			  for (k = x80 ? 8 : 16 ; k > 0 ; k--)
			    p[(j + k) % (640 * 200)] |= 15 * 16;
			  j += 640;
			}
		    }
		}
	    }
	}
    }
  else				// Graphic mode
    {
      int x, c, y, i, j, k, d, d1, d2, l, m;

      if (mode1[vp] & 16)
	{
	  c = 4;
	  if (mode1[vp] & 1)
	    x = 2;
	  else
	    x = 4;
	}
      else
	{
	  if (mode2[vp] & 8)
	    c = 1, x = 1;
	  else
	    {
	      c = 2;
	      if (mode1[vp] & 1)
		x = 1;
	      else
		x = 2;
	    }
	}
      if (mode1[vp] & 1)
	y = 4;
      else
	y = 2;
      for (i = 0 ; i < 200 ; i += y)
	{
	  for (m = 40 * i ; m < 0x2000 * y ; m += 0x2000)
	    {
	      j = m + readtop;
	      switch (c)
		{
		case 1:
		  if (vp)
		    {
		      for (k = 0 ; k < 80 ; k++)
			{
			  d = readmem->read ((j++) & maskdat);
			  p[7] = d & 1, d >>= 1;
			  p[6] = d & 1, d >>= 1;
			  p[5] = d & 1, d >>= 1;
			  p[4] = d & 1, d >>= 1;
			  p[3] = d & 1, d >>= 1;
			  p[2] = d & 1, d >>= 1;
			  p[1] = d & 1, d >>= 1;
			  p[0] = d & 1;
			  p += 8;
			}
		    }
		  else
		    {
		      for (k = 0 ; k < 80 ; k++)
			{
			  d = readmem->read ((j++) & maskdat);
			  p[7] &= 15;
			  p[6] &= 15;
			  p[5] &= 15;
			  p[4] &= 15;
			  p[3] &= 15;
			  p[2] &= 15;
			  p[1] &= 15;
			  p[0] &= 15;
			  p[7] |= (d & 1) << 4, d >>= 1;
			  p[6] |= (d & 1) << 4, d >>= 1;
			  p[5] |= (d & 1) << 4, d >>= 1;
			  p[4] |= (d & 1) << 4, d >>= 1;
			  p[3] |= (d & 1) << 4, d >>= 1;
			  p[2] |= (d & 1) << 4, d >>= 1;
			  p[1] |= (d & 1) << 4, d >>= 1;
			  p[0] |= (d & 1) << 4;
			  p += 8;
			}
		    }
		  break;
		case 2:
		  if (x == 2)
		    {
		      if (vp)
			{
			  for (k = 0 ; k < 80 ; k++)
			    {
			      d = readmem->read ((j++) & maskdat);
			      p[7] = p[6] = d & 3; d >>= 2;
			      p[5] = p[4] = d & 3; d >>= 2;
			      p[3] = p[2] = d & 3; d >>= 2;
			      p[1] = p[0] = d & 3;
			      p += 8;
			    }
			}
		      else
			{
			  for (k = 0 ; k < 80 ; k++)
			    {
			      d = readmem->read ((j++) & maskdat);
			      p[7] &= 15;
			      p[6] &= 15;
			      p[5] &= 15;
			      p[4] &= 15;
			      p[3] &= 15;
			      p[2] &= 15;
			      p[1] &= 15;
			      p[0] &= 15;
			      ctmp = (d & 3) << 4; d >>= 2; p[7] |= ctmp; p[6] |= ctmp;
			      ctmp = (d & 3) << 4; d >>= 2; p[5] |= ctmp; p[4] |= ctmp;
			      ctmp = (d & 3) << 4; d >>= 2; p[3] |= ctmp; p[2] |= ctmp;
			      ctmp = (d & 3) << 4; p[1] |= ctmp; p[0] |= ctmp;
			      p += 8;
			    }
			}
		    }
		  else
		    {
		      if (vp)
			{
			  for (k = 0 ; k < 80 ; k++)
			    {
			      d = readmem->read ((j++) & maskdat);
			      p[7] = d & 1, d >>= 1;
			      p[6] = d & 1, d >>= 1;
			      p[5] = d & 1, d >>= 1;
			      p[4] = d & 1, d >>= 1;
			      p[3] = d & 1, d >>= 1;
			      p[2] = d & 1, d >>= 1;
			      p[1] = d & 1, d >>= 1;
			      p[0] = d & 1;
			      d = readmem->read ((j++) & maskdat);
			      p[7] |= 2 * (d & 1), d >>= 1;
			      p[6] |= 2 * (d & 1), d >>= 1;
			      p[5] |= 2 * (d & 1), d >>= 1;
			      p[4] |= 2 * (d & 1), d >>= 1;
			      p[3] |= 2 * (d & 1), d >>= 1;
			      p[2] |= 2 * (d & 1), d >>= 1;
			      p[1] |= 2 * (d & 1), d >>= 1;
			      p[0] |= 2 * (d & 1);
			      p += 8;
			    }
			}
		      else
			{
			  for (k = 0 ; k < 80 ; k++)
			    {
			      d = readmem->read ((j++) & maskdat);
			      p[7] &= 15;
			      p[6] &= 15;
			      p[5] &= 15;
			      p[4] &= 15;
			      p[3] &= 15;
			      p[2] &= 15;
			      p[1] &= 15;
			      p[0] &= 15;
			      p[7] |= (d & 1) << 4, d >>= 1;
			      p[6] |= (d & 1) << 4, d >>= 1;
			      p[5] |= (d & 1) << 4, d >>= 1;
			      p[4] |= (d & 1) << 4, d >>= 1;
			      p[3] |= (d & 1) << 4, d >>= 1;
			      p[2] |= (d & 1) << 4, d >>= 1;
			      p[1] |= (d & 1) << 4, d >>= 1;
			      p[0] |= (d & 1) << 4;
			      d = readmem->read ((j++) & maskdat);
			      p[7] |= 32 * (d & 1), d >>= 1;
			      p[6] |= 32 * (d & 1), d >>= 1;
			      p[5] |= 32 * (d & 1), d >>= 1;
			      p[4] |= 32 * (d & 1), d >>= 1;
			      p[3] |= 32 * (d & 1), d >>= 1;
			      p[2] |= 32 * (d & 1), d >>= 1;
			      p[1] |= 32 * (d & 1), d >>= 1;
			      p[0] |= 32 * (d & 1);
			      p += 8;
			    }
			}
		    }
		  break;
		case 4:
		  if (vp)
		    {
		      for (k = 40 * y ; k > 0 ; k--)
			{
			  d = readmem->read ((j++) & maskdat);
			  d1 = d / 16;
			  d2 = d % 16;
			  for (l = 0 ; l < x ; l++)
			    *p++ = d1;
			  for (l = 0 ; l < x ; l++)
			    *p++ = d2;
			}
		    }
		  else
		    {
		      for (k = 40 * y ; k > 0 ; k--)
			{
			  d = readmem->read ((j++) & maskdat);
			  d1 = d / 16;
			  d2 = d % 16;
			  ctmp = d1 << 4;
			  for (l = 0 ; l < x ; l++)
			    *p &= 15, *p++ |= ctmp;
			  ctmp = d2 << 4;
			  for (l = 0 ; l < x ; l++)
			    *p &= 15, *p++ |= ctmp;
			}
		    }
		  break;
		}
	    }
	}
    }
  return fillbottom ? 1 : 0;
}

void
jvideo::conv ()
{
  int i;
  int r1, r2;

  //mode1[0] = mode1[1] = 0x13;
  //mode2[0] = mode2[1] = 0;
  //mode1[1] = 0x13;
  //mode2[1] = 0;
  //superimpose = 1;
  //pagereg[0] = pagereg[1] = 0;
  //pagereg[0] = 0x36;
#if 0
  for (i = 0 ; i < 16 ; i++)
    palette[i] = i;
#endif

  r1 = r2 = 0;
  if ((superimpose & 15) != 0)
    r2 = convsub (drawdata, 1);
  if ((superimpose & 15) != 1)
    r1 = convsub (drawdata, 0);
  if (r1 == 2 || r2 == 2)
    {
      for (i = 0 ; i < 640 * 200 ; i++)
	drawdata[i] = bordercolor;
      return;
    }
#if 0
  if ((superimpose & 12) == 0)
    {
      switch (superimpose & 3)
	{
	case 0:
	  for (i = 0 ; i < 640 * 200 ; i++)
	    drawdata[i] = palette[drawdata1[i]];
	  break;
	case 1:
	  for (i = 0 ; i < 640 * 200 ; i++)
	    drawdata[i] = palette[drawdata2[i]];
	  break;
	case 2:
	  j = transpalette;
	  for (i = 0 ; i < 640 * 200 ; i++)
	    {
	      d = drawdata1[i];
	      if (d == j)
		d = drawdata2[i];
	      drawdata[i] = palette[d];
	    }
	  break;
	case 3:			// ???
	  j = transpalette;
	  for (i = 0 ; i < 640 * 200 ; i++)
	    {
	      d = drawdata1[i];
	      if (d != j)
		d = drawdata2[i];
	      drawdata[i] = palette[d];
	    }
	  break;
	}
    }
  else
    {
      switch (superimpose & 12)
	{
	case 12:
	  for (i = 0 ; i < 640 * 200 ; i++)
	    drawdata[i] = palette[drawdata1[i] | drawdata2[i]];
	  break;
	case 8:
	  for (i = 0 ; i < 640 * 200 ; i++)
	    drawdata[i] = palette[drawdata1[i] & drawdata2[i]];
	  break;
	case 4:
	  if ((superimpose & 15) == 6 && (mode1[1] & 128)) // SUPER 16 COLOR
	    for (i = 0 ; i < 640 * 200 ; i++)
	      drawdata[i] = palette[(drawdata1[i] | (drawdata2[i] * 4)) ^ 10];
	  else
	    for (i = 0 ; i < 640 * 200 ; i++)
	      drawdata[i] = palette[drawdata1[i] ^ drawdata2[i]];
	  break;
	}
    }
  if (r2 == 1)
    {
      for (i = 0 ; i < 640 * 2 ; i++)
	drawdata[640 * 198 + i] = bordercolor;
    }
#endif
  fillbottom = (r2 == 1);
}

void
jvideo::floppyaccess (int n)
{
}

jvideo::jvideo (SDL_Window *window, SDL_Surface *surface, jmem *program,
		jmem *kanjirom) throw (char *)
{
  int i;

  jvideo::program = program;
  jvideo::kanjirom = kanjirom;
  drawdata = new unsigned char[640 * 200];
  drawdata1 = new unsigned char[640 * 200];
  drawdata2 = new unsigned char[640 * 200];
  vram = new jmem (65536);
  vsynccount = 0;
  cursorcount = 0;
  blinkcount = 0;
  vsyncintflag = 0;
  dat3da = 0;
  flag3da[0] = flag3da[1] = 0;
  pagereg[0] = pagereg[1] = 0;
  for (i = 0 ; i < 16 ; i++)
    palette[i] = 0;

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
  // SDL_BlitScaled seems not working with 8bit depth surface so
  // create another 32bit depth surface and copy twice to blit :-)
  mysurface2 = SDL_CreateRGBSurface (0, 640, 200, 32, 0, 0, 0, 0);
}

void
jvideo::draw ()
{
  unsigned char *q;
  int i;
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

  q = drawdata;
  if ((superimpose & 12) == 0)
    {
      switch (superimpose & 3)
	{
	case 0:
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  drawdata[i] = palette[drawdata1[i]];
	  for (i = 0 ; i < 256 ; i++)
	    pal[i] = cl[palette[i / 16]];
	  break;
	case 1:
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  drawdata[i] = palette[drawdata2[i]];
	  for (i = 0 ; i < 256 ; i++)
	    pal[i] = cl[palette[i % 16]];
	  break;
	case 2:
	  //j = transpalette;
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  {
	  //    d = drawdata1[i];
	  //    if (d == j)
	  //	  d = drawdata2[i];
	  //    drawdata[i] = palette[d];
	  //  }
	  for (i = 0 ; i < 256 ; i++)
	    {
	      if (i / 16 == transpalette)
		pal[i] = cl[palette[i % 16]];
	      else
		pal[i] = cl[palette[i / 16]];
	    }
	  break;
	case 3:			// ???
	  //j = transpalette;
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  {
	  //    d = drawdata1[i];
	  //    if (d != j)
	  //	  d = drawdata2[i];
	  //    drawdata[i] = palette[d];
	  //  }
	  for (i = 0 ; i < 256 ; i++)
	    {
	      if (i / 16 != transpalette)
		pal[i] = cl[palette[i % 16]];
	      else
		pal[i] = cl[palette[i / 16]];
	    }
	  break;
	}
    }
  else
    {
      switch (superimpose & 12)
	{
	case 12:
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  drawdata[i] = palette[drawdata1[i] | drawdata2[i]];
	  for (i = 0 ; i < 256 ; i++)
	    pal[i] = cl[palette[(i / 16) | (i % 16)]];
	  break;
	case 8:
	  //for (i = 0 ; i < 640 * 200 ; i++)
	  //  drawdata[i] = palette[drawdata1[i] & drawdata2[i]];
	  for (i = 0 ; i < 256 ; i++)
	    pal[i] = cl[palette[(i / 16) & (i % 16)]];
	  break;
	case 4:
	  //if ((superimpose & 15) == 6 && (mode1[1] & 128)) // SUPER 16 COLOR
	  //  for (i = 0 ; i < 640 * 200 ; i++)
	  //    drawdata[i] = palette[(drawdata1[i] | (drawdata2[i] * 4)) ^ 10];
	  //else
	  //  for (i = 0 ; i < 640 * 200 ; i++)
	  //    drawdata[i] = palette[drawdata1[i] ^ drawdata2[i]];
	  if ((superimpose & 15) == 6 && (mode1[1] & 128))
	    for (i = 0 ; i < 256 ; i++)
	      pal[i] = cl[palette[((i / 16) | ((i % 16) * 4)) ^ 10]];
	  else
	    for (i = 0 ; i < 256 ; i++)
	      pal[i] = cl[palette[(i / 16) ^ (i % 16)]];
	  break;
	}
    }
  //for (i = 0 ; i < 256 ; i++)
  //  pal[i] = cl[palette[15]];
  SDL_SetPaletteColors (mypalette, pal, 0, 256);
  if (SDL_MUSTLOCK (mysurface))
    {
      if (SDL_LockSurface (mysurface) < 0)
	{
	  cerr << "lock failed" << endl;
	  return;
	}
    }
  for (i = 0 ; i < 200 ; i++)
    {
      memcpy ((Uint8 *)mysurface->pixels + i * mysurface->pitch, q, 640);
      q += 640;
    }
  if (SDL_MUSTLOCK (mysurface))
    SDL_UnlockSurface (mysurface);
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
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
      return reg[idx];
    }
  return 0xff;
}

void
jvideo::j46505::set_reg (unsigned int idx, unsigned int val)
{
  switch (idx)
    {
    case 0:
    case 1:
    case 2:
    case 13:
    case 15:
      reg[idx] = val & 0xff;
      break;
    case 4:
    case 6:
    case 7:
    case 10:
      reg[idx] = val & 0x7f;
      break;
    case 12:
    case 14:
      reg[idx] = val & 0x3f;
      break;
    case 5:
    case 9:
    case 11:
      reg[idx] = val & 0x1f;
      break;
    case 3:
      reg[idx] = val & 0xf;
      break;
    case 8:
      reg[idx] = val & 0x3;
      break;
    }
}

unsigned int
jvideo::j46505::get_ma ()
{
  return iy * reg[1] + ix + (reg[13] | reg[12] << 8);
}

bool
jvideo::j46505::get_cursor ()
{
  if (!get_disp ())
    return false;
  if (get_ma () != (reg[15] | reg[14] << 8))
    return false;
  if (ira >= reg[10] && ira <= reg[11])
    return true;
  else
    return false;
}

bool
jvideo::j46505::get_disp ()
{
  if (ix < reg[1] && iy < reg[6])
    return true;
  else
    return false;
}

bool
jvideo::j46505::get_hsync ()
{
  if (ix >= reg[2] && ix - reg[2] < reg[3])
    return true;
  else
    return false;
}

bool
jvideo::j46505::get_vsync ()
{
  if (iy >= reg[7] && iy - reg[7] < 1)
    return true;
  else
    return false;
}

void
jvideo::j46505::tick ()
{
  ix++;
  if (ix > reg[0])
    {
      ix = 0;
      ira++;
      if (ira > reg[9])
	{
	  ira = 0;
	  iy++;
	}
      if (iy > reg[4] && ira >= reg[5])
	{
	  ira = 0;
	  iy = 0;
	}
    }
}
