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
  inline int getdata0 () { return data0; }
};
