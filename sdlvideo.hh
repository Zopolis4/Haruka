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
  class surface
  {
  public:
    SDL_Surface *mysurface;
    int width;
    int height;
    int left;
    int top;
    unsigned int clkcount;
    surface (SDL_Palette *mypalette);
    ~surface ();
  };
  static const unsigned int NSURFACES = 4;
  surface *s[NSURFACES];
  SDL_Window *const window;
  SDL_Palette *mypalette;
  int prev_x, prev_y, prev_w, prev_h;
  void clear_surface_if_necessary (SDL_Surface *surface, int x, int y, int w,
				   int h);
  void set_rect (int &srcstart, int &srcsize, int &dststart, int &dstsize,
		 int start, int minsize, int border);
  SDL_sem *producer;
  SDL_sem *consumer;
  unsigned int p;
  SDL_Thread *thread;
  SDL_atomic_t thread_exit_flag;
  void draw_real (SDL_Surface *mysurface, int width, int height, int left,
		  int top);
  static int sdlvideothread_c (void *p);
  void sdlvideothread ();
  SDL_atomic_t sync_updating;
  unsigned int sync_audio_clkcount;
  unsigned int sync_audio_time;
  bool sync_clock (unsigned int clkcount);
public:
  sdlvideo (SDL_Window *);
  ~sdlvideo ();
  unsigned char *get_pointer (int x, int y);
  void draw (int width, int height, int left, int top, unsigned int clkcount);
  void sync_audio (unsigned int clkcount);
};
