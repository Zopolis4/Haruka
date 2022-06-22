// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "jmem.h"
#include "jtype.h"
#include "jvideo.h"

class jfdc
{
private:
  jvideo &video;
  t16 cmdcode;
  void run (char c);
  bool transint;
  int watchdog;
protected:
  t16 step_rate, param1, drive, cylinder, head, sector, bytes_per_sector, eot,
    gap_length, dtl, sectors_per_track, filler;
  const char *p;
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
  jfdc (jvideo &d);
  t16 inf2 ();
  void outf2 (t16 v);
  t16 inf4 ();
  void outf4 (t16 v);
  t16 inf5 ();
  void outf5 (t16 v);
  void clk (int count);
};
