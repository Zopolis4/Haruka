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
#include "jbus.hh"

jbus::io::io (jbus &bus)
{
  iolist_tailnext = &bus.iolist_tailnext;
  next = NULL;
  pnext = *iolist_tailnext;
  *pnext = this;
  *iolist_tailnext = &next;
}

jbus::io::~io ()
{
  *pnext = next;
  if (next)
    next->pnext = pnext;
  else
    *iolist_tailnext = pnext;
}

jbus::io *
jbus::io::get_next ()
{
  return next;
}

void
jbus::io::memory_read (unsigned int addr, unsigned int &val, int &cycles)
{
}

void
jbus::io::memory_write (unsigned int addr, unsigned int val, int &cycles)
{
}

void
jbus::io::ioport_read (unsigned int addr, unsigned int &val, int &cycles)
{
}

void
jbus::io::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
}

jbus::jbus ()
{
  iolist = NULL;
  iolist_tailnext = &iolist;
}

jbus::~jbus ()
{
  if (iolist || iolist_tailnext != &iolist)
    std::cerr << "Deinitializing a bus before devices connected to the bus"
	      << std::endl;
}

unsigned int
jbus::memory_read (unsigned int addr, int &cycles)
{
  unsigned int val = 0xff;
  addr &= 0xfffff;
  for (io *p = iolist; p; p = p->get_next ())
    {
      int pcycles = 0;
      p->memory_read (addr, val, pcycles);
      if (cycles < pcycles)
	cycles = pcycles;
    }
  val &= 0xff;
  return val;
}

void
jbus::memory_write (unsigned int addr, unsigned int val, int &cycles)
{
  addr &= 0xfffff;
  val &= 0xff;
  for (io *p = iolist; p; p = p->get_next ())
    {
      int pcycles = 0;
      p->memory_write (addr, val, pcycles);
      if (cycles < pcycles)
	cycles = pcycles;
    }
}

unsigned int
jbus::ioport_read (unsigned int addr, int &cycles)
{
  unsigned int val = 0xff;
  addr &= 0xffff;
  for (io *p = iolist; p; p = p->get_next ())
    {
      int pcycles = 0;
      p->ioport_read (addr, val, pcycles);
      if (cycles < pcycles)
	cycles = pcycles;
    }
  val &= 0xff;
  return val;
}

void
jbus::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
  addr &= 0xffff;
  val &= 0xff;
  for (io *p = iolist; p; p = p->get_next ())
    {
      int pcycles = 0;
      p->ioport_write (addr, val, pcycles);
      if (cycles < pcycles)
	cycles = pcycles;
    }
}

unsigned int
jbus::memory_read (unsigned int addr)
{
  int cycles = 0;
  return memory_read (addr, cycles);
}

void
jbus::memory_write (unsigned int addr, unsigned int val)
{
  int cycles = 0;
  memory_write (addr, val, cycles);
}

unsigned int
jbus::ioport_read (unsigned int addr)
{
  int cycles = 0;
  return ioport_read (addr, cycles);
}

void
jbus::ioport_write (unsigned int addr, unsigned int val)
{
  int cycles = 0;
  return ioport_write (addr, val, cycles);
}
