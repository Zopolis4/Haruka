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

using namespace std;

#include "SDL.h"
#include "jtype.hh"
#include "jkey.hh"
#include "jevent.hh"

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
      tmp = keyconv (event.key.keysym.sym);
      if (tmp)
	keybd->keydown (tmp);
      break;
    case SDL_KEYUP:
      tmp = keyconv (event.key.keysym.sym);
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
  if (SDL_PushEvent (&e) != 0)
    cerr << "SDL_PushEvent failed: " << SDL_GetError () << endl;	  
}

int
jevent::keyconv (SDLKey key)
{
  switch (key)
    {
    case SDLK_BACKSPACE: return 0xe;
    case SDLK_TAB: return 0xf;
    case SDLK_CLEAR: break;
    case SDLK_RETURN: return 0x1c;
    case SDLK_PAUSE: break;
    case SDLK_ESCAPE: return 0x1;
    case SDLK_SPACE: return 0x39;
    case SDLK_EXCLAIM: break;
    case SDLK_QUOTEDBL: break;
    case SDLK_HASH: break;
    case SDLK_DOLLAR: break;
    case SDLK_AMPERSAND: break;
    case SDLK_QUOTE: return 0x28;
    case SDLK_LEFTPAREN: break;
    case SDLK_RIGHTPAREN: break;
    case SDLK_ASTERISK: break;
    case SDLK_PLUS: break;
    case SDLK_COMMA: return 0x33;
    case SDLK_MINUS: return 0xc;
    case SDLK_PERIOD: return 0x34;
    case SDLK_SLASH: return 0x35;
    case SDLK_0: return 0xb;
    case SDLK_1: return 0x2;
    case SDLK_2: return 0x3;
    case SDLK_3: return 0x4;
    case SDLK_4: return 0x5;
    case SDLK_5: return 0x6;
    case SDLK_6: return 0x7;
    case SDLK_7: return 0x8;
    case SDLK_8: return 0x9;
    case SDLK_9: return 0xa;
    case SDLK_COLON: break;
    case SDLK_SEMICOLON: return 0x27;
    case SDLK_LESS: break;
    case SDLK_EQUALS: return 0xd;
    case SDLK_GREATER: break;
    case SDLK_QUESTION: break;
    case SDLK_AT: break;
    case SDLK_LEFTBRACKET: return 0x1a;
    case SDLK_BACKSLASH: return 0x2b;
    case SDLK_RIGHTBRACKET: return 0x1b;
    case SDLK_CARET: break;
    case SDLK_UNDERSCORE: break;
    case SDLK_BACKQUOTE: return 0x29;
    case SDLK_a: return 0x1e;
    case SDLK_b: return 0x30;
    case SDLK_c: return 0x2e;
    case SDLK_d: return 0x20;
    case SDLK_e: return 0x12;
    case SDLK_f: return 0x21;
    case SDLK_g: return 0x22;
    case SDLK_h: return 0x23;
    case SDLK_i: return 0x17;
    case SDLK_j: return 0x24;
    case SDLK_k: return 0x25;
    case SDLK_l: return 0x26;
    case SDLK_m: return 0x32;
    case SDLK_n: return 0x31;
    case SDLK_o: return 0x18;
    case SDLK_p: return 0x19;
    case SDLK_q: return 0x10;
    case SDLK_r: return 0x13;
    case SDLK_s: return 0x1f;
    case SDLK_t: return 0x14;
    case SDLK_u: return 0x16;
    case SDLK_v: return 0x2f;
    case SDLK_w: return 0x11;
    case SDLK_x: return 0x2d;
    case SDLK_y: return 0x15;
    case SDLK_z: return 0x2c;
    case SDLK_DELETE: return 0x53;
    case SDLK_KP0: break;
    case SDLK_KP1: break;
    case SDLK_KP2: break;
    case SDLK_KP3: break;
    case SDLK_KP4: break;
    case SDLK_KP5: break;
    case SDLK_KP6: break;
    case SDLK_KP7: break;
    case SDLK_KP8: break;
    case SDLK_KP9: break;
    case SDLK_KP_PERIOD: break;
    case SDLK_KP_DIVIDE: break;
    case SDLK_KP_MULTIPLY: break;
    case SDLK_KP_MINUS: return 0x4a;
    case SDLK_KP_PLUS: return 0x4e;
    case SDLK_KP_ENTER: break;
    case SDLK_KP_EQUALS: break;
    case SDLK_UP: return 0x48;
    case SDLK_DOWN: return 0x50;
    case SDLK_RIGHT: return 0x4d;
    case SDLK_LEFT: return 0x4b;
    case SDLK_INSERT: return 0x52;
    case SDLK_HOME: return 0x47;
    case SDLK_END: break;
    case SDLK_PAGEUP: break;
    case SDLK_PAGEDOWN: break;
    case SDLK_F1: return 0x3b;
    case SDLK_F2: return 0x3c;
    case SDLK_F3: return 0x3d;
    case SDLK_F4: return 0x3e;
    case SDLK_F5: return 0x3f;
    case SDLK_F6: return 0x40;
    case SDLK_F7: return 0x41;
    case SDLK_F8: return 0x42;
    case SDLK_F9: return 0x43;
    case SDLK_F10: return 0x44;
    case SDLK_F11: return 0x54;	// Fn
    case SDLK_F12: return 0x6a;	// yen
    case SDLK_F13: break;
    case SDLK_F14: break;
    case SDLK_F15: break;
    case SDLK_NUMLOCK: break;
    case SDLK_CAPSLOCK: return 0x3a;
    case SDLK_SCROLLOCK: return 0x46;
    case SDLK_RSHIFT: return 0x36;
    case SDLK_LSHIFT: return 0x2a;
    case SDLK_RCTRL: return 0x6d; // henkan
    case SDLK_LCTRL: return 0x1d;
    case SDLK_RALT: return 0x6b; // kanji
    case SDLK_LALT: return 0x38;
    case SDLK_RMETA: break;
    case SDLK_LMETA: break;
    case SDLK_LSUPER: break;
    case SDLK_RSUPER: return 0x6c; // mu-henkan
    case SDLK_MODE: break;
    case SDLK_HELP: break;
    case SDLK_PRINT: return 0x37;
    case SDLK_SYSREQ: break;
    case SDLK_BREAK: break;
    case SDLK_MENU: break;
    case SDLK_POWER: break;
    case SDLK_EURO: break;
    default: break;
    }
  return 0;
}
