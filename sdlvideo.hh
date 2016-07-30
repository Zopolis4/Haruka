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

#include "SDL.h"

class sdlvideo : public jvideo::hw
{
  SDL_Window *const window;
  SDL_Surface *mysurface;
  SDL_Palette *mypalette;
  int prev_x, prev_y, prev_w, prev_h;
  void clear_surface_if_necessary (SDL_Surface *surface, int x, int y, int w,
				   int h);
  void set_rect (int &srcstart, int &srcsize, int &dststart, int &dstsize,
		 int start, int minsize, int border);
public:
  sdlvideo (SDL_Window *);
  ~sdlvideo ();
  unsigned char *get_pointer (int x, int y);
  void draw (int width, int height, int left, int top);
};
