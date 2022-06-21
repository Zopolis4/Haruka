// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

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
