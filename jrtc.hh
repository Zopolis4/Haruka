// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

class jrtc
{
  struct rtc_data;
  rtc_data *d;
public:
  jrtc ();
  ~jrtc ();
  void clk (unsigned int clockcount);
  unsigned int inb (unsigned int addr);
  void outb (unsigned int addr, unsigned int val);
};
