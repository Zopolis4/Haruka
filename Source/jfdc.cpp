// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "jfdc.h"
#include "8259a.h"
#include <cctype>
#include <cstdio>

static const int WATCHDOG_TIME = 14318180 * 2; // 1-3sec.

jfdc::jfdc (jvideo &d) : video (d)
{
  p = "AZ";
  f2 = 0;
  cmdcode = 0;
}

t16
jfdc::inf2 ()
{
  return f2;
}

void
jfdc::outf2 (t16 v)
{
  if ((v & 0x80) != (f2 & 0x80))
    {
      st[0] = 0xc0;
      p = "AZ";
    }
  f2 = v;
  video.floppyaccess (v);
  if (!(v & 0x20) && watchdog == -1)
    {
      untrigger_irq8259 (6);
      watchdog = 0;
    }
  if (v & 0x40)
    watchdog = WATCHDOG_TIME;
}

t16
jfdc::inf4 ()
{
  switch (p[0])
    {
    case 'A':
      return 0x80;
    case 'B':
      return 0xd0;
    case 'C':
      return 0xa0;
    }
  return 0;
}

void
jfdc::outf4 (t16 v)
{
}

void
jfdc::run (char c)
{
  switch (c)
    {
    case 'Z':
      transint = false;
      switch (cmdcode)
	{
	case 8:			// SENSE
	  p = "**B0B1B2B3B4B5B6";
	  break;
	case 3:			// SPECIFY
	  p = "**AaAb";
	  break;
	case 0x46:		// READ
	  p = "**AcAdAeAfAgAhAiAJC B0B1B2B3B4B5B6";
	  break;
	case 0x4d:		// FORMAT
	  p = "**AcAgAkAiALC B0B1B2B3B4B5B6";
	  break;
	case 0x45:		// WRITE
	  p = "**AcAdAeAfAgAhAiAJC B0B1B2B3B4B5B6";
	  break;
	case 7:			// RECALIBRATE
	  p = "**AC";
	  break;
	case 0xf:		// SEEK
	  p = "**AcAD";
	  break;
	}
      break;
    default:
      switch (cmdcode)
	{
	case 8:			// SENSE
	  break;
	case 3:			// SPECIFY
	  break;
	case 0x46:		// READ
	  read ();
	  break;
	case 0x4d:		// FORMAT
	  preformat ();
	  break;
	case 0x45:		// WRITE
	  prewrite ();
	  break;
	case 7:			// RECALIBRATE
	  recalibrate ();
	  break;
	case 0xf:		// SEEK
	  seek ();
	  break;
	}
    }
}

t16
jfdc::inf5 ()
{
  t16 r;

  r = 0;
  switch (tolower (p[1]))
    {
    case '0': r = st[0]; break;
    case '1': r = st[1]; break;
    case '2': r = st[2]; break;
    case '3': r = st[3]; break;
    case '4': r = st[4]; break;
    case '5': r = st[5]; break;
    case '6': r = st[6]; break;
    case 'a': r = step_rate; break;
    case 'b': r = param1; break;
    case 'c': r = drive; break;
    case 'd': r = cylinder; break;
    case 'e': r = head; break;
    case 'f': r = sector; break;
    case 'g': r = bytes_per_sector; break;
    case 'h': r = eot; break;
    case 'i': r = gap_length; break;
    case 'j': r = dtl; break;
    case 'k': r = sectors_per_track; break;
    case 'l': r = filler; break;
    case ' ':
      if (transint)
	{
	  timeout ();
	  transint = false;
	  return 0;
	}
      r = data[datai++];
      break;
    case 'z': r = cmdcode; break;
    }
  if (isupper (p[1]))
    run (p[1]);
  if (p[1] == ' ' && datai >= datasize)
    {
      if (cmdcode == 0x46)	// READ
	postread ();
    }
  if (p[1] != ' ' || datai >= datasize)
    p += 2;
  if (p[0] == 0)
    p = "AZ";
  return r;
}

void
jfdc::outf5 (t16 v)
{
  switch (tolower (p[1]))
    {
    case '0': st[0] = v; break;
    case '1': st[1] = v; break;
    case '2': st[2] = v; break;
    case '3': st[3] = v; break;
    case '4': st[4] = v; break;
    case '5': st[5] = v; break;
    case '6': st[6] = v; break;
    case 'a': step_rate = v; break;
    case 'b': param1 = v; break;
    case 'c': drive = v; break;
    case 'd': cylinder = v; break;
    case 'e': head = v; break;
    case 'f': sector = v; break;
    case 'g': bytes_per_sector = v; break;
    case 'h': eot = v; break;
    case 'i': gap_length = v; break;
    case 'j': dtl = v; break;
    case 'k': sectors_per_track = v; break;
    case 'l': filler = v; break;
    case ' ':
      if (transint)
	{
	  timeout ();
	  transint = false;
	  return;
	}
      data[datai++] = v;
      break;
    case 'z': cmdcode = v; break;
    }
  if (isupper (p[1]))
    run (p[1]);
  if (p[1] == ' ' && datai >= datasize)
    {
      switch (cmdcode)
	{
	case 0x4d:		// FORMAT
	  format ();
	  break;
	case 0x45:		// WRITE
	  write ();
	  break;
	}
    }
  if (p[1] != ' ' || datai >= datasize)
    p += 2;
  if (p[0] == 0)
    p = "AZ";
}

void
jfdc::timeout ()
{
  if (f2 & 0x20)
    {
      trigger_irq8259 (6);
      watchdog = -1;
    }
  p = "B0B1B2B3B4B5B6";
}

void
jfdc::transfertimeout ()
{
  transint = true;
}

void
jfdc::clk (int count)
{
  if (watchdog > 0)
    {
      if (watchdog > count)
	watchdog -= count;
      else
	{
	  if (f2 & 0x20)
	    {
	      trigger_irq8259 (6);
	      watchdog = -1;
	    }
	  else
	    watchdog = 0;
	}
    }
}
