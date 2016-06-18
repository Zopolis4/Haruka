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

#include <iostream>

#include "SDL.h"
#include "jtype.hh"
#include "jkey.hh"
#include "jevent.hh"

using std::cerr;
using std::endl;

extern "C"
{
  extern void printip8088 (void);
}

jevent::jevent (jkey *key)
{
  keybd = key;
  quit_flag = false;
}

// handle_event gets new SDL event and handle it.
// This method must be called in main thread.
void
jevent::handle_event ()
{
  int r;
  int tmp;
  SDL_Event event;

  r = SDL_WaitEvent (&event);
  if (r != 1)		// If error
    {
      cerr << "SDL_WaitEvent failed: " << SDL_GetError () << endl;	  
      return;
    }
  switch (event.type)
    {
    case SDL_KEYDOWN:
      // SDL1: one key down one SDL_KEYDOWN
      // SDL2: one key down many SDL_KEYDOWN during auto-repeat
      if (!event.key.repeat)
	{
	  tmp = keyconv (event.key.keysym.scancode);
	  if (tmp)
	    keybd->keydown (tmp);
	}
      break;
    case SDL_KEYUP:
      tmp = keyconv (event.key.keysym.scancode);
      if (tmp)
	keybd->keyup (tmp);
      break;
    case SDL_MOUSEBUTTONDOWN:
      cerr << "Mouse down" << endl;
      printip8088 ();
      break;
    case SDL_QUIT:
      cerr << "Quit" << endl;
      quit_flag = true;
      break;
    case SDL_USEREVENT:
      switch (event.user.type)
	{
	case 0:
	  break;
	}
      break;
    }
}

void
jevent::push_event (int code)
{
  SDL_Event e;

  e.type = SDL_USEREVENT;
  e.user.code = code;
  e.user.data1 = NULL;
  e.user.data2 = NULL;
  if (SDL_PushEvent (&e) != 1)
    cerr << "SDL_PushEvent failed: " << SDL_GetError () << endl;	  
}

void
jevent::push_quit_event ()
{
  SDL_Event e;

  e.type = SDL_QUIT;
  if (SDL_PushEvent (&e) != 1)
    cerr << "SDL_PushEvent failed: " << SDL_GetError () << endl;
}

int
jevent::keyconv (SDL_Scancode key)
{
  switch (key)
    {
    case SDL_SCANCODE_BACKSPACE: return 0xe;
    case SDL_SCANCODE_TAB: return 0xf;
    case SDL_SCANCODE_CLEAR: break;
    case SDL_SCANCODE_RETURN: return 0x1c;
    case SDL_SCANCODE_PAUSE: break;
    case SDL_SCANCODE_ESCAPE: return 0x1;
    case SDL_SCANCODE_SPACE: return 0x39;
    case SDL_SCANCODE_APOSTROPHE: return 0x28;
    case SDL_SCANCODE_COMMA: return 0x33;
    case SDL_SCANCODE_MINUS: return 0xc;
    case SDL_SCANCODE_PERIOD: return 0x34;
    case SDL_SCANCODE_SLASH: return 0x35;
    case SDL_SCANCODE_0: return 0xb;
    case SDL_SCANCODE_1: return 0x2;
    case SDL_SCANCODE_2: return 0x3;
    case SDL_SCANCODE_3: return 0x4;
    case SDL_SCANCODE_4: return 0x5;
    case SDL_SCANCODE_5: return 0x6;
    case SDL_SCANCODE_6: return 0x7;
    case SDL_SCANCODE_7: return 0x8;
    case SDL_SCANCODE_8: return 0x9;
    case SDL_SCANCODE_9: return 0xa;
    case SDL_SCANCODE_SEMICOLON: return 0x27;
    case SDL_SCANCODE_EQUALS: return 0xd;
    case SDL_SCANCODE_LEFTBRACKET: return 0x1a;
    case SDL_SCANCODE_BACKSLASH: return 0x2b;
    case SDL_SCANCODE_RIGHTBRACKET: return 0x1b;
    case SDL_SCANCODE_GRAVE: return 0x29;
    case SDL_SCANCODE_A: return 0x1e;
    case SDL_SCANCODE_B: return 0x30;
    case SDL_SCANCODE_C: return 0x2e;
    case SDL_SCANCODE_D: return 0x20;
    case SDL_SCANCODE_E: return 0x12;
    case SDL_SCANCODE_F: return 0x21;
    case SDL_SCANCODE_G: return 0x22;
    case SDL_SCANCODE_H: return 0x23;
    case SDL_SCANCODE_I: return 0x17;
    case SDL_SCANCODE_J: return 0x24;
    case SDL_SCANCODE_K: return 0x25;
    case SDL_SCANCODE_L: return 0x26;
    case SDL_SCANCODE_M: return 0x32;
    case SDL_SCANCODE_N: return 0x31;
    case SDL_SCANCODE_O: return 0x18;
    case SDL_SCANCODE_P: return 0x19;
    case SDL_SCANCODE_Q: return 0x10;
    case SDL_SCANCODE_R: return 0x13;
    case SDL_SCANCODE_S: return 0x1f;
    case SDL_SCANCODE_T: return 0x14;
    case SDL_SCANCODE_U: return 0x16;
    case SDL_SCANCODE_V: return 0x2f;
    case SDL_SCANCODE_W: return 0x11;
    case SDL_SCANCODE_X: return 0x2d;
    case SDL_SCANCODE_Y: return 0x15;
    case SDL_SCANCODE_Z: return 0x2c;
    case SDL_SCANCODE_DELETE: return 0x53;
    case SDL_SCANCODE_KP_PERIOD: break;
    case SDL_SCANCODE_KP_DIVIDE: break;
    case SDL_SCANCODE_KP_MULTIPLY: break;
    case SDL_SCANCODE_KP_MINUS: return 0x4a;
    case SDL_SCANCODE_KP_PLUS: return 0x4e;
    case SDL_SCANCODE_KP_ENTER: break;
    case SDL_SCANCODE_KP_EQUALS: break;
    case SDL_SCANCODE_UP: return 0x48;
    case SDL_SCANCODE_DOWN: return 0x50;
    case SDL_SCANCODE_RIGHT: return 0x4d;
    case SDL_SCANCODE_LEFT: return 0x4b;
    case SDL_SCANCODE_INSERT: return 0x52;
    case SDL_SCANCODE_HOME: return 0x47;
    case SDL_SCANCODE_END: break;
    case SDL_SCANCODE_PAGEUP: break;
    case SDL_SCANCODE_PAGEDOWN: break;
    case SDL_SCANCODE_F1: return 0x3b;
    case SDL_SCANCODE_F2: return 0x3c;
    case SDL_SCANCODE_F3: return 0x3d;
    case SDL_SCANCODE_F4: return 0x3e;
    case SDL_SCANCODE_F5: return 0x3f;
    case SDL_SCANCODE_F6: return 0x40;
    case SDL_SCANCODE_F7: return 0x41;
    case SDL_SCANCODE_F8: return 0x42;
    case SDL_SCANCODE_F9: return 0x43;
    case SDL_SCANCODE_F10: return 0x44;
    case SDL_SCANCODE_F11: return 0x54;	// Fn
    case SDL_SCANCODE_F12: return 0x6a;	// yen
    case SDL_SCANCODE_F13: break;
    case SDL_SCANCODE_F14: break;
    case SDL_SCANCODE_F15: break;
    case SDL_SCANCODE_CAPSLOCK: return 0x3a;
    case SDL_SCANCODE_SCROLLLOCK: return 0x46;
    case SDL_SCANCODE_RSHIFT: return 0x36;
    case SDL_SCANCODE_LSHIFT: return 0x2a;
    case SDL_SCANCODE_RCTRL: return 0x6d; // henkan
    case SDL_SCANCODE_LCTRL: return 0x1d;
    case SDL_SCANCODE_RALT: return 0x6b; // kanji
    case SDL_SCANCODE_LALT: return 0x38;
    case SDL_SCANCODE_RGUI: return 0x6c; // mu-henkan
    case SDL_SCANCODE_MODE: break;
    case SDL_SCANCODE_HELP: break;
    case SDL_SCANCODE_PRINTSCREEN: return 0x37;
    case SDL_SCANCODE_SYSREQ: break;
    case SDL_SCANCODE_MENU: break;
    case SDL_SCANCODE_POWER: break;
    default: break;
    }
  return 0;
}
