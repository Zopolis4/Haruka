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

class jfdc
{
private:
  jvideo *video;
  t16 cmdcode;
  void run (char c);
  bool transint;
protected:
  t16 step_rate, param1, drive, cylinder, head, sector, bytes_per_sector, eot,
    gap_length, dtl, sectors_per_track, filler;
  char *p;
  t16 st[7];
  unsigned char data[8192];
  int datai, datasize;
  t16 f2;
  virtual void read () = 0;
  virtual void postread () = 0;
  virtual void preformat () = 0;
  virtual void format () = 0;
  virtual void prewrite () = 0;
  virtual void write () = 0;
  virtual void recalibrate () = 0;
  virtual void seek () = 0;
  void timeout ();
  void transfertimeout ();
public:
  jfdc (jvideo *d);
  t16 inf2 ();
  void outf2 (t16 v);
  t16 inf4 ();
  void outf4 (t16 v);
  t16 inf5 ();
  void outf5 (t16 v);
};

