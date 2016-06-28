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

#include <cstdio>
#include "SDL.h"
class jvideo
{
private:
  class j46505
  {
  private:
    static const int NREG = 18;
    unsigned int reg[NREG];
    unsigned int ix, iy, ira;
    unsigned int cur_addr;
    unsigned int framecount;
    void set_reg (unsigned int idx, unsigned int val);
    unsigned int get_reg (unsigned int idx);
  public:
    j46505 ();
    void tick ();
    bool get_disp ();
    bool get_hsync ();
    bool get_vsync ();
    bool get_cursor (bool force_blink);
    unsigned int get_ma ();
    unsigned int get_ra ();
    unsigned int inb (unsigned int addr);
    void outb (unsigned int addr, unsigned int val);
  };
private:
  int flag3da[2];
  jmem &program;		// Programmable RAM
  jmem vram;			// 32KB VRAM + 32KB VRAM
  jmem &kanjirom;
  int vsynccount, cursorcount;
  int blinkcount;
  int vsyncintflag;
  t16 dat3da;
  unsigned char v3da;
  int pagereg[2];
  void conv ();
  int convsub (unsigned char *p, int vp);
  int index3d4;
  int hsynccount;
  int vtop (int vp2)
  {
    int a;

    a = (crtcdata[12] * 256 + crtcdata[13]) * 2;
    if (vp2)
      return a + (pagereg[1] & 3) * 16384;
    return a + (pagereg[0] & 7) * 16384;
  }
  jmem &vmem (int vp2)
  {
    if (vp2)
      return vram;
    return program;
  }
  int vmask (int vp2)
  {
    if (vp2)
      return 65535;
    return 131071;
  }
protected:
  int mode1[2], palettemask[2], mode2[2];
  unsigned char *drawdata;
  int crtcdata[32];
  int bordercolor;
  int reset, transpalette, superimpose;
  unsigned char palette[16];
public:
  unsigned char read (bool vp2, int offset)
  {
    if (vp2)
      return vram.read ((offset + ((pagereg[1] >> 3) & 3) * 16384) & 65535);
    return program.read ((offset + ((pagereg[0] >> 3) & 7) * 16384) & 131071);
  }
  void write (bool vp2, int offset, unsigned char data)
  {
    if (vp2)
      return vram.write ((offset + ((pagereg[1] >> 3) & 3) * 16384) & 65535,
			  data);
    return program.write ((offset + ((pagereg[0] >> 3) & 7) * 16384) & 131071,
			   data);
  }
  t16 in3da (bool vp2) { flag3da[vp2 ? 1 : 0] = 0; return dat3da ^= 16; }
  void out3da (bool vp2, unsigned char data);
  void out3df (unsigned char data) { pagereg[0] = data; }
  void out3d9 (unsigned char data) { pagereg[1] = data; }
  void out3d4 (unsigned char data) { index3d4 = data; }
  unsigned char in3d5 () { return crtcdata[index3d4 % 32]; }
  void out3d5 (unsigned char data) { crtcdata[index3d4 % 32] = data;
  /* for (int i=0;i<12;i++)printf("%02x ",crtcdata[i]);printf("\n"); */ }
  void clk (int clockcount, bool stopflag);
  //virtual void draw () = 0;
  bool pcjrmem () { return (mode2[1] & 16) ? true : false; }
  //virtual void floppyaccess (int n);
private:
  SDL_Window *window;
  SDL_Surface *surface;
  SDL_Color pal[256];
  SDL_Surface *mysurface;
  SDL_Surface *mysurface2;
  SDL_Palette *mypalette;
  unsigned long *bits;
  unsigned long *bits2;
public:
  jvideo (SDL_Window *, SDL_Surface *, jmem &program, jmem &kanjirom)
    throw (char *);
  ~jvideo ();
  void draw ();
  void floppyaccess (int n);
};

