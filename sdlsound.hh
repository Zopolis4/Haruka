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

class sdlsound
{
private:
  // 8253-5 Programmable Interval Timer
  class j8253
  {
  private:
    class j8253counter
    {
    private:
      unsigned int cntreg;
      unsigned int cntelement;
      unsigned int cntreload;
      unsigned int outlatch;
      unsigned int modebcd;
      bool counting, latching, setting;
      bool readmsb, writemsb;
      bool togglemsb;
      bool out, gate;
      bool trigger, strobe;
      void initout ();
      void reload ();
      unsigned int decrement ();
    public:
      j8253counter ();
      void write_control (unsigned int val);
      unsigned int read_counter ();
      void write_counter (unsigned int val);
      void setgate (bool val);
      inline bool getout ();
      void tick ();
    };
    j8253counter counter[4];
  public:
    inline void out8253 (unsigned int addr, unsigned int val);
    inline unsigned int in8253 (unsigned int addr);
    inline void setgate (unsigned int addr, bool val);
    inline bool getout (unsigned int addr);
    inline void tick (unsigned int addr);
  };
  // SN76489A Digital Complex Sound Generator
  class jsn76489a
  {
  private:
    unsigned int div32;
    unsigned int saved_data;
    unsigned int noise_register;
    unsigned int noise_shift;
    unsigned int attenuator[4];
    unsigned int frequency[4];
    unsigned int counter[4];
    bool noise_update, noise_white;
    bool outbit[4];
    unsigned int outsum, outcnt;
  public:
    jsn76489a ();
    void tick ();
    unsigned int getdata ();
    void outb (unsigned int data);
  };
  unsigned int rate, buffersize;
  signed short *localbuf;
  unsigned int clksum;
  unsigned int copyoffset;
  unsigned int filloffset;
  int samples;
  SDL_atomic_t fillsize;
  bool playing;
  j8253 pit;
  unsigned int pb;
  bool timer1sel;
  unsigned int timerclk;
  unsigned int internalbeep0, internalbeep1;
  unsigned int outbeep0, outbeep1;
  jsn76489a sn76489a;
  unsigned int soundclk;
  SDL_sem *semaphore;
  bool hurry;
  void tick_pit ();
  void tick_sound ();
  void tick_genaudio ();
public:
  void audiocallback (unsigned char *stream, int len);
  void iowrite (unsigned char data);
  void clk (int clockcount);
  sdlsound (unsigned int rate, unsigned int buffersize);
  ~sdlsound ();
  void out8253 (unsigned int addr, unsigned int val);
  unsigned int in8253 (unsigned int addr);
  bool gettimer2out ();
  void set8255b (unsigned int val);
  void selecttimer1in (bool timer0out);
  bool get_hurry ()
  {
    return hurry;
  }
};
