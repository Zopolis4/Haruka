// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cmath>
#include <fstream>
#include <cstdio>
#include <iostream>

#include "SDL.h"
#include "jmem.hh"
#include "jvideo.hh"
#include "sdlsound.hh"
#include "8259a.hh"

using std::cout;
using std::cerr;
using std::endl;

void
sdlsound::out8253 (unsigned int addr, unsigned int val)
{
  pit.out8253 (addr, val);
}

unsigned int
sdlsound::in8253 (unsigned int addr)
{
  return pit.in8253 (addr);
}

bool
sdlsound::gettimer2out ()
{
  return pit.getout (2);
}

void
sdlsound::set8255b (unsigned int val)
{
  pit.setgate (2, ((val & 0x1) != 0) ? true : false);
  pb = val;
}

void
sdlsound::selecttimer1in (bool timer0out)
{
  timer1sel = timer0out;
}

void
sdlsound::audiocallback (Uint8 *stream, int len)
{
  int size = SDL_AtomicGet (&fillsize);
  if (!playing && size >= samples * 2)
    playing = true;
  if (playing)
    {
      const int bufsize = buffersize;
      short *buf = (short *)stream;
      len /= 2;
      while (len > 0 && size > 0)
	{
	  const unsigned int off = copyoffset;
	  if (size > len)
	    size = len;
	  for (int i = 0; i < size; i++)
	    buf[i] = localbuf[(off + i) % bufsize];
	  const unsigned int newoff = (off + size) % bufsize;
	  if (off + size != newoff)
	    videohw.sync_audio (clkcount_at_buf0);
	  copyoffset = newoff;
	  buf += size;
	  len -= size;
	  int oldsize = SDL_AtomicAdd (&fillsize, -size);
	  if (oldsize == bufsize)
	    SDL_SemPost (semaphore);
	  size = oldsize - size;
	}
      if (len > 0)
	{
	  cerr << "Audio buffer underflow" << "\tbuffersize " << buffersize
	       << " len " << len << endl;
	  for (int i = 0; i < len; i++)
	    buf[i] = 0;
	  playing = false;
	}
    }
}

void
sdlsound::sdlaudiocallback (void *data, Uint8 *stream, int len)
{
  sdlsound *p = static_cast<sdlsound *> (data);

  p->audiocallback (stream, len);
}

Uint32
sdlsound::sdltimercallback (Uint32 interval, void *param)
{
  sdlsound *p = static_cast<sdlsound *> (param);
  Uint8 buf[p->samples * 2];

  p->audiocallback (buf, sizeof buf);
  if (p->closing)
    {
      SDL_SemPost (p->closing);
      return 0;
    }
  return interval;
}

sdlsound::sdlsound (unsigned int rate, unsigned int buffersize,
		    unsigned int samples, jvideo::hw &videohw)
  : samples (samples), videohw (videohw)
{
  unsigned int i;
  SDL_AudioSpec fmt;

  // Gate 0 and 1 of PIT are always high
  pit.setgate (0, true);
  pit.setgate (1, true);

  // Initialize CLK counter
  timerclk = 0;
  soundclk = 0;
  clksum = 0;
  clkcount = 0;

  // Initialize audio buffer
  sdlsound::rate = rate;
  sdlsound::buffersize = buffersize;
  localbuf = new signed short [buffersize];
  for (i = 0; i < buffersize; i++)
    localbuf[i] = 0;

  // Initialize variables used by callback handler
  copyoffset = 0;
  filloffset = 0;
  SDL_AtomicSet (&fillsize, 0);
  playing = false;
  semaphore = SDL_CreateSemaphore (0);
  clkcount_at_buf0 = 0;

  // Open SDL audio
  timer_id = 0;
  fmt.freq = rate;
  fmt.format = AUDIO_S16SYS;
  fmt.channels = 1;
  fmt.samples = samples;
  fmt.callback = sdlaudiocallback;
  fmt.userdata = (void *)this;
  if (SDL_OpenAudio (&fmt, NULL) < 0)
    {
      cerr << "SDL_OpenAudio failed. Continue without audio." << endl;
      sdlsound::rate = 10000;
      samples = 100;
      closing = NULL;
      timer_id = SDL_AddTimer (10, sdltimercallback, this);
      if (!timer_id)
	cerr << "SDL_AddTimer failed." << endl;
      return;
    }
  SDL_PauseAudio (0);
}

sdlsound::~sdlsound ()
{
  if (timer_id)
    {
      closing = SDL_CreateSemaphore (0);
      SDL_SemWait (closing);
      SDL_DestroySemaphore (closing);
    }
  else
    SDL_CloseAudio ();
  SDL_DestroySemaphore (semaphore);
  delete [] localbuf;
}

void
sdlsound::iowrite (unsigned char data)
{
  sn76489a.outb (data);
}

void
sdlsound::tick_pit ()
{
  bool out00 = pit.getout (0);
  pit.tick (0);
  bool out01 = pit.getout (0);
  if (!out00 && out01)
    trigger_irq8259 (0);
  else if (out00 && !out01)
    untrigger_irq8259 (0);
  if (!timer1sel || (!out00 && out01))
    pit.tick (1);
  pit.tick (2);
  if ((pb & 0x2) == 0 || !pit.getout (2)) // !PB1 || !Timer2output
    {
      outbeep0++;
      internalbeep0++;
    }
  else
    {
      outbeep1++;
      if ((pb & 0x10) != 0) // PB4
	internalbeep0++;
      else
	internalbeep1++;
    }
}

void
sdlsound::tick_sound ()
{
  sn76489a.tick ();
}

void
sdlsound::tick_genaudio ()
{
  int soundtmp = sn76489a.getdata ();
  int speakerin = 8192 * internalbeep1 / (internalbeep0 + internalbeep1);
  int speakerout;
  switch (pb & 0x60)	// PB5 and PB6
    {
    default:		// Make compiler happy
    case 0x00:		// Timer 2 output
      speakerout = 8192 * outbeep1 / (outbeep0 + outbeep1);
      break;
    case 0x20:		// Cassette tape audio
    case 0x40:		// I/O channel audio
      speakerout = 0;
      break;
    case 0x60:		// Sound generator output
      speakerout = soundtmp - 8192;
      break;
    }
  if (!filloffset)
    clkcount_at_buf0 = clkcount;
  localbuf[filloffset] = speakerin + speakerout;
  outbeep0 = 0;
  outbeep1 = 0;
  internalbeep0 = 0;
  internalbeep1 = 0;
  if (filloffset + 1 >= buffersize)
    filloffset = 0;
  else
    filloffset++;
  if (SDL_AtomicAdd (&fillsize, 1) + 1u == buffersize)
    SDL_SemWait (semaphore);
}

void
sdlsound::clk (int clockcount)
{
  // Use a simple fast loop for each counter until generating audio to
  // improve performance
  int n = (14318180 - 1 - clksum) / rate;
  if (n > clockcount)
    n = clockcount;
  int t = (timerclk + n) / 12;
  timerclk = (timerclk + n) % 12;
  for (int i = 0; i < t; i++)
    tick_pit ();
  t = (soundclk + n) / 4;
  soundclk = (soundclk + n) % 4;
  for (int i = 0; i < t; i++)
    tick_sound ();
  clksum += rate * n;
  clkcount += n;
  // Slow loop...
  for (int i = n; i < clockcount; i++)
    {
      // PIT CLK input: 14.31818MHz / 12 = 1.193182 MHz
      timerclk++;
      if (timerclk >= 12)
	{
	  timerclk -= 12;
	  tick_pit ();
	}
      // SN76489 CLK input: 14.31818MHz / 4 = 3.579545MHz
      soundclk++;
      if (soundclk >= 4)
	{
	  soundclk -= 4;
	  tick_sound ();
	}
      // Generating audio data: rate = 14.31818MHz * (rate / 14.31818MHz)
      clksum += rate;
      if (clksum >= 14318180)
	{
	  clksum -= 14318180;
	  tick_genaudio ();
	}
      clkcount++;
    }
}

////////////////////////////////////////////////////////////
// SN76489A

sdlsound::jsn76489a::jsn76489a ()
{
  int i;

  // Just clear all variables
  div32 = 0;
  saved_data = 0;
  noise_register = 0;
  noise_update = false;
  noise_white = false;
  noise_shift = 0;
  for (i = 0; i < 4; i++)
    {
      attenuator[i] = 0xf;	// Off
      frequency[i] = 0;
      counter[i] = 0;
    }
  for (i = 0; i < 3; i++)
    outbit[i] = false;
  outsum = 0;
  outcnt = 0;
}

void
sdlsound::jsn76489a::tick ()
{
  int i;
  unsigned int noise_frequency;

  if (div32 > 1)
    {
      div32--;
      return;
    }
  div32 = 32;
  // Just like 8253 mode 3 counter
  for (i = 0; i < 3; i++)
    {
      if (counter[i] == 0 && frequency[i] == 0)
	{
	  outbit[i] = false;
	  continue;
	}
      if (counter[i]-- <= 1)
	{
	  outbit[i] = false;
	  counter[i] = frequency[i];
	  if (counter[i] <= 22)	// low pass filter 4.8kHz
	    counter[i] = 0;
	  if ((counter[i] & 1) == 0)
	    continue;
	}
      if (counter[i]-- <= 1)
	{
	  outbit[i] = !outbit[i];
	  counter[i] = frequency[i];
	  if (counter[i] <= 22)	// low pass filter 4.8kHz
	    counter[i] = 0;
	  if ((counter[i] & 1) != 0 && outbit[i] == false)
	    counter[i]--;
	}
    }
  // Noise
  if (counter[3]-- <= 1)
    {
      switch (noise_register & 0x3)
	{
	default:		// Avoid compiler warning
	case 0x0:		// N / 512
	  noise_frequency = 16;
	  break;
	case 0x1:		// N / 1024
	  noise_frequency = 32;
	  break;
	case 0x2:		// N / 2048
	  noise_frequency = 64;
	  break;
	case 0x3:		// Tone 3 frequency
	  noise_frequency = frequency[2] * 2;
	  if (noise_frequency <= 22) // low pass filter 4.8kHz
	    noise_frequency = 0;
	  break;
	}
      if (noise_update)
	{
	  if ((noise_register & 0x4) != 0)
	    {
	      // White noise
	      noise_shift = 0x4001;
	      noise_white = true;
	    }
	  else
	    {
	      // Periodic noise
	      noise_shift = 1;
	      noise_white = false;
	    }
	  noise_update = false;
	}
      counter[3] = noise_frequency;
      if (noise_frequency != 0)
	{
	  outbit[3] = ((noise_shift & 0x1) != 0) ? true : false;
	  if (noise_white)
	    {
	      if (((noise_shift ^ noise_shift >> 1) & 0x1) != 0)
		noise_shift = 0x4000 | noise_shift >> 1;
	      else
		noise_shift >>= 1;
	    }
	  else
	    {
	      if ((noise_shift & 0x1) != 0)
		noise_shift = 0x2000;
	      else
		noise_shift >>= 1;
	    }
	}
      else
	{
	  outbit[3] = false;
	}
    }
  // Calculate output
  for (i = 0; i < 4; i++)
    {
      if (outbit[i])
	outsum += 546 * (0xf - attenuator[i]);
    }
  outcnt++;
}

unsigned int
sdlsound::jsn76489a::getdata ()
{
  unsigned int r;

  r = outsum / outcnt;
  outsum = 0;
  outcnt = 0;
  if (r > 32767)
    return 32767;
  else
    return r;
}

void
sdlsound::jsn76489a::outb (unsigned int data)
{
  switch (data & 0xf0)
    {
    case 0x80:			// Tone 1 frequency
    case 0xa0:			// Tone 2 frequency
    case 0xc0:			// Tone 3 frequency
      frequency[(data >> 5) & 3] &= ~0xf;
      frequency[(data >> 5) & 3] |= data & 0xf;
      saved_data = data;	// These are two bytes command.
      break;
    case 0xe0:			// Noise
      noise_register = data;
      noise_update = true;
      break;
    case 0x90:			// Tone 1 attenuator
    case 0xb0:			// Tone 2 attenuator
    case 0xd0:			// Tone 3 attenuator
    case 0xf0:			// Noise attenuator
      attenuator[(data >> 5) & 3] = data & 0xf;
      break;
    default:			// Two bytes command
      frequency[(saved_data >> 5) & 3] = ((data & 0x3f) << 4)
	| (saved_data & 0xf);
      break;
    }
}

////////////////////////////////////////////////////////////
// 8253

sdlsound::j8253::j8253counter::j8253counter ()
{
  // Just clear all variables
  counting = false;
  latching = false;
  setting = false;
  readmsb = false;
  writemsb = false;
  togglemsb = false;
  modebcd = 0;
  outlatch = 0;
  cntelement = 0;
  cntreload = 0;
  cntreg = 0;
  out = false;
  gate = false;
  trigger = false;
  strobe = false;
}

void
sdlsound::j8253::j8253counter::initout ()
{
  switch (modebcd & 0xe)
    {
    case 0x0:	  // Mode 0: Interrupt on terminal count
      out = false;
      break;
    case 0x2:	  // Mode 1: Hardware retriggerable one-shot
    case 0x4:	  // Mode 2: Rate generator
    case 0xc:	  // Mode 2
    case 0x6:	  // Mode 3: Square wave mode
    case 0xe:	  // Mode 3
    case 0x8:	  // Mode 4: Software triggered strobe
    case 0xa:	  // Mode 5: Hardware triggered strobe (retriggerable)
      out = true;
      break;
    }
}

void
sdlsound::j8253::j8253counter::write_control (unsigned int val)
{
  switch (val & 0x30)
    {
    case 0x00:			// Counter latch command
      if (!latching)		// Ignore if this is already latched
	{
	  outlatch = cntelement;
	  latching = true;
	}
      return;
    case 0x10:			// LSB only
      readmsb = false;
      writemsb = false;
      togglemsb = false;
      break;
    case 0x20:			// MSB only
      readmsb = true;
      writemsb = true;
      togglemsb = false;
      break;
    case 0x30:			// LSB then MSB
      readmsb = false;
      writemsb = false;
      togglemsb = true;
      break;
    }
  counting = false;
  setting = false;
  strobe = false;
  modebcd = val & 0xf;
  initout ();
}

unsigned int
sdlsound::j8253::j8253counter::read_counter ()
{
  unsigned int val;

  if (latching)
    val = outlatch;
  else
    val = cntelement;
  if ((modebcd & 0x6) == 0x6)	// Mode 3, (modebcd & 0xe) is 0x6 or 0xe
    val &= 0xfffe;		// Mode 3 counter must be even
  if (readmsb)
    val >>= 8;
  val &= 0xff;
  if (!togglemsb || readmsb)
    latching = false;
  if (togglemsb)
    readmsb = !readmsb;
  return val;
}

void
sdlsound::j8253::j8253counter::write_counter (unsigned int val)
{
  setting = false;
  val &= 0xff;
  if (writemsb)
    val <<= 8;
  if (!togglemsb || !writemsb)
    cntreg = 0;
  if ((modebcd & 0xe) == 0x0)	// Mode 0
    {
      counting = false;
      initout ();
    }
  cntreg |= val;
  if (!togglemsb || writemsb)
    setting = true;
  if (togglemsb)
    writemsb = !writemsb;
}

void
sdlsound::j8253::j8253counter::setgate (bool val)
{
  // GATE has no effect on OUT except mode 2 or mode 3
  if ((modebcd & 0x4) == 0x4)	// Mode 2 or mode 3
    {
      if (gate && !val)
	out = true;
    }
  if (!gate && val)
    trigger = true;
  gate = val;
}

inline bool
sdlsound::j8253::j8253counter::getout ()
{
  return out;
}

void
sdlsound::j8253::j8253counter::reload ()
{
  if (setting)
    cntreload = cntreg;
  cntelement = cntreload;
  setting = false;
  counting = true;
}

unsigned int
sdlsound::j8253::j8253counter::decrement ()
{
  if ((modebcd & 0x1) == 0x1)	// BCD
    {
      if ((cntelement & 0xf) == 0x0)
	{
	  if ((cntelement & 0xf0) == 0x00)
	    {
	      if ((cntelement & 0xf00) == 0x000)
		{
		  if ((cntelement & 0xf000) == 0x0000)
		    {
		      // 0000 - 1 = 9999
		      cntelement = 0x9999;
		    }
		  else
		    {
		      // 1000 - 1 = 0999
		      cntelement -= 0x667;
		    }
		}
	      else
		{
		  // 0100 - 1 = 0099
		  cntelement -= 0x67;
		}
	    }
	  else
	    {
	      // 0010 - 1 = 0009
	      cntelement -= 0x7;
	    }
	}
      else
	{
	  // 0001 - 1 = 0000
	  cntelement--;
	}
    }
  else
    {
      if (cntelement == 0)
	cntelement = 0xffff;
      else
	cntelement--;
    }
  return cntelement;
}

void
sdlsound::j8253::j8253counter::tick ()
{
  switch (modebcd & 0xe)
    {
    case 0x0:			// Mode 0
      if (setting)
	{
	  reload ();
	  out = false;
	  break;		// this cycle does not decrement
	}
      if (!counting)
	break;
      if (!gate)
	break;
      if (decrement () == 0)
	out = true;
      break;
    case 0x2:			// Mode 1
      if (!counting && setting)
	{
	  reload ();
	  out = true;
	  break;
	}
      if (!counting)
	break;
      if (trigger)
	{
	  reload ();
	  out = false;
	}
      else
	{
	  if (decrement () == 0)
	    out = true;
	}
      break;
    case 0x4:			// Mode 2
    case 0xc:			// Mode 2
      if (!counting && setting)
	{
	  reload ();
	  out = true;
	  break;
	}
      if (!counting)
	break;
      if (!gate)
	break;
      if (trigger)
	{
	  reload ();
	  out = true;
	  break;
	}
      switch (decrement ())
	{
	case 1:
	  out = false;
	  break;
	case 0:
	  reload ();
	  out = true;
	  break;
	}
      break;
    case 0x6:			// Mode 3
    case 0xe:			// Mode 3
      if (!counting && setting)
	{
	  reload ();
	  out = true;
	  break;
	}
      if (!counting)
	break;
      if (!gate)
	break;
      if (trigger)
	{
	  reload ();
	  out = true;
	  break;
	}
      if (decrement () == 0)
	{
	  // Odd counts only: OUT is high here and will be low
	  // Decrementing after reload will make counter even counts
	  out = false;
	  reload ();
	  // if count is not odd here, we need to skip decrementing
	  // because counter is changed to even counts now.
	  if ((cntelement & 1) == 0)
	    break;
	}
      if (decrement () == 0)
	{
	  // Even counts: OUT is unknown
	  // Odd counts: OUT is low here and will be high
	  out = !out;
	  reload ();
	  // if count is odd and out is low, the counter is changed from
	  // even counts to odd counts.
	  if ((cntelement & 1) != 0 && out == false)
	    decrement ();
	}
      break;
    case 0x8:			// Mode 4
      if (setting)
	{
	  reload ();
	  out = true;
	  strobe = true;
	  break;		// this cycle does not decrement
	}
      if (!counting)
	break;
      if (!gate)
	break;
      if (decrement () == 0 && strobe)
	{
	  out = false;
	  strobe = false;
	}
      else
	{
	  out = true;
	}
      break;
    case 0xa:			// Mode 5
      if (trigger)
	{
	  reload ();
	  out = true;
	  strobe = true;
	  break;		// this cycle does not decrement
	}
      if (!counting)
	break;
      if (decrement () == 0 && strobe)
	{
	  out = false;
	  strobe = false;
	}
      else
	{
	  out = true;
	}
      break;
    }
  trigger = false;
};

inline void
sdlsound::j8253::out8253 (unsigned int addr, unsigned int val)
{
  addr &= 3;
  if (addr <= 2)
    counter[addr].write_counter (val);
  else
    counter[(val & 0xc0) >> 6].write_control (val);
}

inline unsigned int
sdlsound::j8253::in8253 (unsigned int addr)
{
  addr &= 3;
  if (addr <= 2)
    return counter[addr].read_counter ();
  else
    return 0xff;
}

inline void
sdlsound::j8253::setgate (unsigned int addr, bool val)
{
  counter[addr].setgate (val);
}

inline bool
sdlsound::j8253::getout (unsigned int addr)
{
  return counter[addr].getout ();
}

inline void
sdlsound::j8253::tick (unsigned int addr)
{
  counter[addr].tick ();
}
