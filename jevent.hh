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

class jevent
{
private:
  jkey *keybd;
  int keyconv (SDL_Scancode key);
  bool quit_flag;
public:
  jevent (jkey *key);
  void handle_event ();
  void push_event (int code);
  bool get_quit_flag ()
  {
    return quit_flag;
  }
};
