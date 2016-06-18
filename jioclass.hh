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

class ioclass
{
 public:
  virtual t16 memr (unsigned long addr) = 0;
  virtual void memw (unsigned long addr, t16 v) = 0;
  virtual t16 in (t16 n) = 0;
  virtual void out (t16 n, t16 v) = 0;
};


class jioclass : public ioclass
{
private:
  struct struct_regs1ff
  {
    t16 reg1, reg2;
    t16 andreg1, andreg2;
    t16 orreg1, orreg2;
  };
  struct_regs1ff regs1ff[31];
  t16 memr_ (unsigned long adr);
  jvideo *videoclass;
  sdlsound *soundclass;
  jmem *systemrom;
  jmem *program;
  jmem *mainram;
  jmem *kanjirom;
  jkey *kbd;
  jmem *cartrom;
  jfdc *fdc;
  bool base1_rom, base2_rom;

  int joyx, joyy, joyb;
  int status1ff;
  t16 block1ff;
  t16 dat8255[3];
  t16 timerval[3], timerlatch[3];
  int timermod[3];
  int timermod2[3];
  t16 timercounter[3];
  int flag20;
  int ignore21;
  int timer2flag;
  int fdc_cmdcode, fdc_mode, fdc_drive, fdc_flag, fdc_c, fdc_h, fdc_r, fdc_n;
  int fdc_secsize;
  unsigned char fdc_data[512*20];
  int fdc_datap;
  int timerintflag;
public:
  jioclass (jvideo *, sdlsound *, jmem *sys, jmem *prg,
	    jmem *main, jmem *knj, jkey *key, jmem *cart, jfdc *dsk);
  bool timerset (int clk, bool stopredraw);
  t16 memr (unsigned long addr);
  void memw (unsigned long addr, t16 v);
  t16 in (t16 n);
  void out (t16 n, t16 v);
  inline void set_base1_rom (bool d) { base1_rom = d; }
  inline void set_base2_rom (bool d) { base2_rom = d; }
};
