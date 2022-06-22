// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SDL.h"

#include "jvideo.h"

class sdlvideo : public jvideo::hw
{
  class surface
  {
  public:
    SDL_Surface* mysurface;
    int width;
    int height;
    int left;
    int top;
    unsigned int clkcount;
    surface (SDL_Palette* mypalette);
    ~surface();
  };
  static const unsigned int NSURFACES = 4;
  surface* s[NSURFACES];
  SDL_Window* const window;
  SDL_Palette* mypalette;
  int prev_x, prev_y, prev_w, prev_h;
  void clear_surface_if_necessary (SDL_Surface* surface, int x, int y, int w, int h);
  void set_rect (int& srcstart, int& srcsize, int& dststart, int& dstsize, int start, int minsize,
                 int border);
  SDL_sem* producer;
  SDL_sem* consumer;
  unsigned int p;
  SDL_Thread* thread;
  SDL_atomic_t thread_exit_flag;
  void draw_real (SDL_Surface* mysurface, int width, int height, int left, int top);
  static int sdlvideothread_c (void* p);
  void sdlvideothread();
  SDL_atomic_t sync_updating;
  unsigned int sync_audio_clkcount;
  unsigned int sync_audio_time;
  bool sync_clock (unsigned int clkcount);

public:
  sdlvideo (SDL_Window*);
  ~sdlvideo();
  unsigned char* get_pointer (int x, int y);
  void draw (int width, int height, int left, int top, unsigned int clkcount);
  void sync_audio (unsigned int clkcount);
};
