// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

class jevent
{
private:
  jkey &keybd;
  int keyconv (SDL_Scancode key);
  bool quit_flag;
public:
  jevent (jkey &key);
  void handle_event ();
  void push_event (int code);
  void push_quit_event ();
  bool get_quit_flag ()
  {
    return quit_flag;
  }
};
