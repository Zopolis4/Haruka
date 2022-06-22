// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "jtype.h"

class jkey
{
private:
  int nmiflag;
  int clkcount;
  int keyrepeatclkcount;
  int keydata;
  int data0;
  int skipclkcount;
  int repeatkey;
  int intrclkcount;
  int keybuf[4];	// Internal key buffer for avoiding key losts
  unsigned char keybuf_getnext, keybuf_putnext;
  static const int KEYCLK = 3150; // 14318180*220/1000/1000
  static const int KEYREPEAT1CLK = 14318180 * 0.6;
  static const int KEYREPEAT2CLK = 14318180 / 11;
  static const int KEYINTRCLK = 14318180 / 10;
  bool input_ok (int clk);
public:
  jkey ();
  void keydown (int code);
  void keyup (int code);
  inline int getnmiflag () { return nmiflag; }
  inline int getkeydata () { return keydata & 1; }
  t16 in ();
  void out (t16 d);
  bool clkin (int clk);
};
