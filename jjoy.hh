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

class jjoy
{
protected:
  class joystick
  {
    int x, y, b;
    bool connected;
  public:
    joystick ();
    void connect (bool yes);
    void set_state (int xr, int yr, bool b1, bool b2);
    unsigned char get_value ();
    void clk (int d);
  };
  joystick joy1, joy2;
  virtual void update_state ();
public:
  jjoy ();
  unsigned char in201 ();
  void out201 (unsigned char data);
  void clk (int d);
};
