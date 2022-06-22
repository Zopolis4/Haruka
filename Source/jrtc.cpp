// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "jrtc.h"
#include <ctime>

// Real Time Clock (TIMDEV.SYS) support
// Port 0x360 Second  1
// Port 0x361 Second 10
// Port 0x362 Minute  1
// Port 0x363 Minute 10
// Port 0x364 Hour    1
// Port 0x365 Hour   10
// Port 0x366 Day     1
// Port 0x367 Day    10
// Port 0x368 Month   1
// Port 0x369 Month  10
// Port 0x36a Year    1
// Port 0x36b Year   10
// Port 0x36c Week?   1
// Port 0x36d
// Port 0x36f

struct jrtc::rtc_data
{
  time_t curr_time;
  unsigned int latch[0xd];
  bool write_mode;
  unsigned int clk;
};

jrtc::jrtc()
{
  d = new rtc_data;
  d->curr_time = time (NULL);
}

jrtc::~jrtc()
{
  delete d;
}

void jrtc::clk (unsigned int clockcount)
{
  unsigned int sum = d->clk + clockcount;
  d->curr_time += sum / 14318180;
  d->clk = sum % 14318180;
}

unsigned int jrtc::inb (unsigned int addr)
{
  addr &= 0xf;
  if (addr < 0xd)
    return d->latch[addr];
  return 0;
}

void jrtc::outb (unsigned int addr, unsigned int val)
{
  addr &= 0xf;
  if (addr < 0xd)
  {
    d->latch[addr] = val & 0xff;
    return;
  }
  if (addr == 0xd)
  {
    if (val)
    {
      d->write_mode = false;
      // FIXME: localtime is not thread safe, but Windows API does
      // not have localtime_r
      struct tm t = *localtime (&d->curr_time);
      d->latch[0] = t.tm_sec % 10;
      d->latch[1] = t.tm_sec / 10;
      d->latch[2] = t.tm_min % 10;
      d->latch[3] = t.tm_min / 10;
      d->latch[4] = t.tm_hour % 10;
      d->latch[5] = t.tm_hour / 10;
      d->latch[6] = t.tm_mday % 10;
      d->latch[7] = t.tm_mday / 10;
      d->latch[8] = (t.tm_mon + 1) % 10;
      d->latch[9] = (t.tm_mon + 1) / 10;
      d->latch[10] = t.tm_year % 10;
      d->latch[11] = (t.tm_year / 10) % 10;
      d->latch[12] = t.tm_wday;
    }
    else if (d->write_mode)
    {
      d->write_mode = false;
      struct tm t;
      t.tm_sec = (d->latch[1] & 0x7) * 10 + (d->latch[0] & 0xf);
      t.tm_min = (d->latch[3] & 0x7) * 10 + (d->latch[2] & 0xf);
      t.tm_hour = (d->latch[5] & 0x3) * 10 + (d->latch[4] & 0xf);
      t.tm_mday = (d->latch[7] & 0x3) * 10 + (d->latch[6] & 0xf);
      t.tm_mon = (d->latch[9] & 0x1) * 10 + (d->latch[8] & 0xf) - 1;
      t.tm_year = (d->latch[11] & 0xf) * 10 + (d->latch[10] & 0xf);
      if (t.tm_year < 80)
        t.tm_year += 100;
      d->curr_time = mktime (&t);
    }
  }
  if (addr == 0xf)
    d->write_mode = true;
}
