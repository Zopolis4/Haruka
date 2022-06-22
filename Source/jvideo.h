// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "jmem.h"

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
    j46505();
    void tick();
    bool get_disp();
    bool get_hsync();
    bool get_vsync();
    bool get_cursor (bool force_blink);
    unsigned int get_ma();
    unsigned int get_ra();
    unsigned int inb (unsigned int addr);
    void outb (unsigned int addr, unsigned int val);
  };
  j46505 crtc;
  int flag3da[2];
  jmem& program;  // Programmable RAM
  jmem vram;      // 32KB VRAM + 32KB VRAM
  jmem& kanjirom;
  int blinkcount;
  unsigned char v3da;
  int pagereg[2];
  unsigned int convcount;
  int draw_x, draw_y;
  int disp_start_x, disp_start_y;
  int new_disp_start_x, new_disp_start_y, new_disp_start_count;
  bool gma_reset;
  unsigned int gma10;
  unsigned int gma20;
  unsigned int gma30;
  unsigned int gma1;
  unsigned int gma2;
  unsigned int gma3;
  unsigned int gra1;
  unsigned int gra2;
  unsigned int gra3;
  unsigned char last_color;
  unsigned int vsynccount;
  void correct_disp_pos();
  void convsub (int readtop1, int readtop2, int enable1, int enable2, int si, int len,
                unsigned char* p, bool disp);
  void ex_convsub (int enable, int len, unsigned char* p, bool disp);
  void conv (int clockcount);
  void ex_conv (int clockcount);
  void convtick (bool disp);
  void ex_draw();
  void draw();
  int index3d4;
  bool flag3dd;
  unsigned char v3dd;
  unsigned char ex_reg2;
  unsigned char ex_palette[4];
  unsigned char ex_reg5;
  unsigned char ex_reg6;
  unsigned char ex_reg7;
  unsigned int ex_convcount;
  unsigned int ex_framecount;
  bool ex_prev_cursor;
  unsigned int clkcount;

protected:
  int mode1[2], palettemask[2], mode2[2];
  unsigned char* drawdata;
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
      return vram.write ((offset + ((pagereg[1] >> 3) & 3) * 16384) & 65535, data);
    return program.write ((offset + ((pagereg[0] >> 3) & 7) * 16384) & 131071, data);
  }
  unsigned int in3da (bool vp2);
  void out3da (bool vp2, unsigned char data);
  void out3df (unsigned char data) { pagereg[0] = data; }
  void out3d9 (unsigned char data) { pagereg[1] = data; }
  void out3d4 (unsigned char data);
  unsigned char in3d5();
  void out3d5 (unsigned char data);
  unsigned int in3dd();
  void out3dd (unsigned char data);
  void clk (int clockcount);
  // virtual void draw () = 0;
  bool pcjrmem() { return (mode2[1] & 16) ? true : false; }
  // virtual void floppyaccess (int n);
private:
  static const int SURFACE_WIDTH = 800, SURFACE_HEIGHT = 240;
  static const int HSTART = -0x30, VSTART = -1;
  static const int HSYNCSTART = 600, VSYNCSTART = 200;
  static const int HSYNCEND = 1000, VSYNCEND = 300;
  static const int EX_SURFACE_WIDTH = 800, EX_SURFACE_HEIGHT = 600;
  static const int EX_HSTART = 0x0, EX_VSTART = -1;
  static const int EX_HSYNCSTART = 600, EX_VSYNCSTART = 500;
  static const int EX_HSYNCEND = 1000, EX_VSYNCEND = 700;

public:
  class hw
  {
  protected:
    static const int WIDTH = EX_SURFACE_WIDTH;
    static const int HEIGHT = EX_SURFACE_HEIGHT;

  public:
    virtual unsigned char* get_pointer (int x, int y) = 0;
    virtual void draw (int width, int height, int left, int top, unsigned int clkcount) = 0;
    virtual void sync_audio (unsigned int clkcount) = 0;
  };

private:
  hw& videohw;
  unsigned long* bits;
  unsigned long* bits2;

public:
  jvideo (hw& videohw, jmem& program, jmem& kanjirom);
  void floppyaccess (int n);
};
