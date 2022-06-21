// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "jtype.hh"
#include "jkey.hh"

jkey::jkey ()
{
  nmiflag = 0;
  clkcount = 0;
  keyrepeatclkcount = 0;
  keydata = 0;
  data0 = 0;
  skipclkcount = 0;
  repeatkey = 0;
  intrclkcount = 0;
  keybuf_getnext = 0;
  keybuf_putnext = 0;
}

void
jkey::keydown (int code)
{
  if (keybuf_getnext + 4 == keybuf_putnext)
    return;
  keybuf[keybuf_putnext++ & 3] = code;
}

void
jkey::keyup (int code)
{
  if (keybuf_getnext + 4 == keybuf_putnext)
    return;
  keybuf[keybuf_putnext++ & 3] = code | 128;
}

t16
jkey::in ()
{
  nmiflag = 0;
  return data0;
}

void
jkey::out (t16 d)
{
  data0 = d;
}

bool
jkey::input_ok (int clk)
{
  if (!(data0 & 128))
    {
      // NMI is disabled
      intrclkcount += clk;
      // Wait for NMI enable for convenience
      if (intrclkcount < KEYINTRCLK)
	return false;
    }
  intrclkcount = 0;
  return true;
}

bool
jkey::clkin (int clk)
{
  bool f = false;
  if (skipclkcount)
    {
      skipclkcount -= clk;
      if (skipclkcount < 0)
	skipclkcount = 0;
    }
  clkcount += clk;
  if (clkcount >= 0)
    {
      clkcount -= KEYCLK;
      if (!(keydata >>= 1))
	nmiflag = 0;
    }
  if (!skipclkcount)
    {
      int nextkey = 0;
      if (keybuf_getnext != keybuf_putnext && input_ok (clk))
	{
	  nextkey = keybuf[keybuf_getnext++ & 3];
	  if (nextkey & 128)
	    {
	      if ((nextkey & 127) == repeatkey)
		repeatkey = 0;
	    }
	  else
	    {
	      repeatkey = nextkey;
	      keyrepeatclkcount = KEYREPEAT1CLK;
	    }
	}
      else if (repeatkey && keyrepeatclkcount < 0)
	{
	  keyrepeatclkcount += KEYREPEAT2CLK;
	  nextkey = repeatkey;
	}
      if (nextkey)
	{
	  // Return value of getkeydata() for each bit:
	  // - Bit '0': 220ns 0 220ns 1
	  // - Bit '1': 220ns 1 220ns 0
	  // - Stop bit: 440ns 0
	  // Serialize key code
	  // S D0 D1 D2 D3 D4 D5 D6 D7 P O O O O O O O O O O O
	  // - S: Start bit '1'
	  // - D0-D7: Data bit
	  // - P: Parity bit (odd parity)
	  // - O: Stop bit
	  // Scan code for error (invalid combination): 0x55 (not used)
	  int i, k = 1 << 18;
	  for (i = 0; i < 8; i++)
	    if (nextkey & (1 << i)) // Bit '1'
	      k ^= (1 << (i * 2 + 2)) | (3 << 18);
	    else		// Bit '0'
	      k ^= 1 << (i * 2 + 3);
	  keydata = k | 1;	// Start bit '1'
	  clkcount = -KEYCLK;
	  skipclkcount = 14318180 / 32;
	  nmiflag = 1;
	}
    }
  if (nmiflag == 1 && (data0 & 128))
    {
      nmiflag = 2;
      f = true;
    }
  keyrepeatclkcount -= clk;
  return f;
}
