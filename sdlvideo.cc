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

#include "jmem.hh"
#include "jvideo.hh"
#include "sdlvideo.hh"

sdlvideo::surface::surface (SDL_Palette *mypalette)
{
  mysurface = SDL_CreateRGBSurface (0, WIDTH, HEIGHT, 8, 0, 0, 0, 0);
  SDL_SetSurfacePalette (mysurface, mypalette);
  if (SDL_MUSTLOCK (mysurface))
    SDL_LockSurface (mysurface);
}

sdlvideo::surface::~surface ()
{
  if (SDL_MUSTLOCK (mysurface))
    SDL_UnlockSurface (mysurface);
  SDL_FreeSurface (mysurface);
}

void
sdlvideo::clear_surface_if_necessary (SDL_Surface *surface, int x, int y,
				      int w, int h)
{
  if (x != prev_x || y != prev_y || w != prev_w || h != prev_h)
    {
      SDL_FillRect (surface, NULL, SDL_MapRGB (surface->format, 0, 0, 0));
      prev_x = x;
      prev_y = y;
      prev_w = w;
      prev_h = h;
    }
}

void
sdlvideo::set_rect (int &srcstart, int &srcsize, int &dststart, int &dstsize,
		    int start, int minsize, int border)
{
  srcstart = 0;
  dststart = 0;
  if (srcsize <= dstsize)	// Window is larger than buffer
    {
      // Draw buffer at center of the window
      dststart = (dstsize - srcsize) / 2;
      dstsize = srcsize;
    }
  else if (srcsize > dstsize)	// Window is smaller than buffer
    {
      // Auto adjust the display position
      if (dstsize < start + minsize + border)
	srcstart = start + minsize + border - dstsize;
      if (srcstart > start - border)
	srcstart = start + (minsize - dstsize) / 2;
      if (srcstart < 0)
	srcstart = 0;
      if (srcstart + dstsize > srcsize)
	srcstart = srcsize - dstsize;
      srcsize = dstsize;
    }
}

void
sdlvideo::draw_real (SDL_Surface *mysurface, int width, int height, int left,
		     int top)
{
  if (SDL_MUSTLOCK (mysurface))
    SDL_UnlockSurface (mysurface);
  SDL_Surface *surface = SDL_GetWindowSurface (window);
  SDL_Rect dstrect;
  dstrect.w = surface->w;
  dstrect.h = surface->h;
  if (dstrect.w == WIDTH && dstrect.h == HEIGHT)
    {
      clear_surface_if_necessary (surface, 0, 0, dstrect.w, dstrect.h);
      SDL_BlitSurface (mysurface, NULL, surface, NULL);
    }
  else
    {
      SDL_Rect srcrect;
      srcrect.w = WIDTH;
      srcrect.h = HEIGHT;
      set_rect (srcrect.x, srcrect.w, dstrect.x, dstrect.w, left, width, 16);
      set_rect (srcrect.y, srcrect.h, dstrect.y, dstrect.h, top, height, 16);
      clear_surface_if_necessary (surface, dstrect.x, dstrect.y, dstrect.w,
				  dstrect.h);
      SDL_BlitSurface (mysurface, &srcrect, surface, &dstrect);
    }
  SDL_UpdateWindowSurface (window);
  if (SDL_MUSTLOCK (mysurface))
    SDL_LockSurface (mysurface);
}

int
sdlvideo::sdlvideothread_c (void *p)
{
  static_cast<sdlvideo *> (p)->sdlvideothread ();
  return 0;
}

void
sdlvideo::sdlvideothread ()
{
  unsigned int c = 0;
  for (;;)
    {
      SDL_SemWait (producer);
      if (SDL_AtomicGet (&thread_exit_flag))
	{
	  SDL_SemPost (consumer);
	  break;
	}
      draw_real (s[c]->mysurface, s[c]->width, s[c]->height, s[c]->left,
		 s[c]->top);
      SDL_SemPost (consumer);
      c = (c + 1) % NSURFACES;
    }
  SDL_AtomicSet (&thread_exit_flag, 2);
}

sdlvideo::sdlvideo (SDL_Window *window) : window (window)
{
  static const SDL_Color cl[16] =
    {
      {r: 0x00, g: 0x00, b: 0x00},
      {r: 0x00, g: 0x00, b: 0xdd},
      {r: 0x00, g: 0xdd, b: 0x00},
      {r: 0x00, g: 0xdd, b: 0xdd},
      {r: 0xdd, g: 0x00, b: 0x00},
      {r: 0xdd, g: 0x00, b: 0xdd},
      {r: 0xdd, g: 0xdd, b: 0x00},
      {r: 0xdd, g: 0xdd, b: 0xdd},
      {r: 0x88, g: 0x88, b: 0x88},
      {r: 0x88, g: 0x88, b: 0xff},
      {r: 0x88, g: 0xff, b: 0x88},
      {r: 0x88, g: 0xff, b: 0xff},
      {r: 0xff, g: 0x88, b: 0x88},
      {r: 0xff, g: 0x88, b: 0xff},
      {r: 0xff, g: 0xff, b: 0x88},
      {r: 0xff, g: 0xff, b: 0xff},
    };
  producer = SDL_CreateSemaphore (0);
  consumer = SDL_CreateSemaphore (0);
  p = 0;
  SDL_AtomicSet (&thread_exit_flag, 0);
  thread = SDL_CreateThread (sdlvideothread_c, "video", this);
  mypalette = SDL_AllocPalette (256);
  SDL_SetPaletteColors (mypalette, cl, 0, 16);
  for (unsigned int i = 0; i < NSURFACES; i++)
    {
      s[i] = new surface (mypalette);
      SDL_SemPost (consumer);
    }
  SDL_SemWait (consumer);
}

sdlvideo::~sdlvideo ()
{
  SDL_AtomicSet (&thread_exit_flag, 1);
  while (SDL_AtomicGet (&thread_exit_flag) == 1)
    {
      SDL_SemPost (producer);
      SDL_SemWait (consumer);
    }
  int status;
  SDL_WaitThread (thread, &status);
  for (unsigned int i = 0; i < NSURFACES; i++)
    delete s[i];
  SDL_FreePalette (mypalette);
  SDL_DestroySemaphore (consumer);
  SDL_DestroySemaphore (producer);
}

unsigned char *
sdlvideo::get_pointer (int x, int y)
{
  SDL_Surface *mysurface = s[p]->mysurface;
  unsigned char *pixels = static_cast<unsigned char *> (mysurface->pixels);
  return pixels + y * mysurface->pitch + x;
}

void
sdlvideo::draw (int width, int height, int left, int top)
{
  s[p]->width = width;
  s[p]->height = height;
  s[p]->left = left;
  s[p]->top = top;
  SDL_SemPost (producer);
  p = (p + 1) % NSURFACES;
  SDL_SemWait (consumer);
}
