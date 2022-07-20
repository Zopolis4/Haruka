// Copyright (C) 2000-2016 Hideki EIRAKU
// Copyright 2022 Maximilian Downey Twiss
// SPDX-License-Identifier: GPL-2.0-or-later

export module RTC;

import <chrono>;

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

std::time_t curr_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
unsigned int latch[0xd];

export namespace RTC
{
  // Page 25 of the IBM JX Technical Reference Manual (2.2 Fundamental System Function):
  // "Devide" typo present in original manual scan

  // Signal    I/O    Description
  // CPU CLK    0     System Clock: It is a "devide-by-three" of the
  //                  14.31818 MHz oscillator and has a period of
  //                  210 ns (4.77MHz). The clock has 33 % of the
  //                  duty cycle
  //                       ______                      ______                      ___
  //                      |      |                    |      |                    |
  //                  ____|  70  |_______140 ns_______|  70  |_______140 ns_______|
  //                         ns
  void clk (unsigned int clockcount)
  {
    unsigned int clk;
    unsigned int sum = clk + clockcount;
    curr_time += sum / 14318180;
    clk = sum % 14318180;
  }

  unsigned int inb (unsigned int addr)
  {
    addr &= 0xf;
    if (addr < 0xd)
      return latch[addr];
    return 0;
  }

  void outb (unsigned int addr, unsigned int val)
  {
    bool write_mode;
    addr &= 0xf;
    if (addr < 0xd)
    {
      latch[addr] = val & 0xff;
      return;
    }
    if (addr == 0xd)
    {
      if (val)
      {
        write_mode = false;
        struct tm t = *localtime (&curr_time);
        latch[0] = t.tm_sec % 10;
        latch[1] = t.tm_sec / 10;
        latch[2] = t.tm_min % 10;
        latch[3] = t.tm_min / 10;
        latch[4] = t.tm_hour % 10;
        latch[5] = t.tm_hour / 10;
        latch[6] = t.tm_mday % 10;
        latch[7] = t.tm_mday / 10;
        latch[8] = (t.tm_mon + 1) % 10;
        latch[9] = (t.tm_mon + 1) / 10;
        latch[10] = t.tm_year % 10;
        latch[11] = (t.tm_year / 10) % 10;
        latch[12] = t.tm_wday;
      }
      else if (write_mode)
      {
        write_mode = false;
        struct tm t;
        t.tm_sec = (latch[1] & 0x7) * 10 + (latch[0] & 0xf);
        t.tm_min = (latch[3] & 0x7) * 10 + (latch[2] & 0xf);
        t.tm_hour = (latch[5] & 0x3) * 10 + (latch[4] & 0xf);
        t.tm_mday = (latch[7] & 0x3) * 10 + (latch[6] & 0xf);
        t.tm_mon = (latch[9] & 0x1) * 10 + (latch[8] & 0xf) - 1;
        t.tm_year = (latch[11] & 0xf) * 10 + (latch[10] & 0xf);
        if (t.tm_year < 80)
          t.tm_year += 100;
        curr_time = mktime (&t);
      }
    }
    if (addr == 0xf)
      write_mode = true;
  }

}
