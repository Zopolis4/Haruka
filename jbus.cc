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

void
jbus::iolist_init ()
{
  for (int i = 0; i < 32; i++)
    {
      iolist.next[i] = NULL;
      iolist.pnext[i] = &iolist.next[i];
    }
}

bool
jbus::is_iolist_empty ()
{
  for (int i = 0; i < 32; i++)
    if (iolist.next[i] || iolist.pnext[i] != &iolist.next[i])
      return false;
  return true;
}

void
jbus::iolist_add (io *p, int n)
{
  if (!(p->iolist.iobmp & (1 << n)))
    {
      p->iolist.next[n] = NULL;
      p->iolist.pnext[n] = iolist.pnext[n];
      *iolist.pnext[n] = p;
      iolist.pnext[n] = &p->iolist.next[n];
      p->iolist.iobmp |= (1 << n);
    }
}

void
jbus::iolist_init (io *p)
{
  p->iolist.iobmp = 0;
  for (int i = 0; i < 32; i++)
    iolist_add (p, i);
}

void
jbus::iolist_del (io *p, int n)
{
  if (p->iolist.iobmp & (1 << n))
    {
      *p->iolist.pnext[n] = p->iolist.next[n];
      if (p->iolist.next[n])
	p->iolist.next[n]->iolist.pnext[n] = p->iolist.pnext[n];
      else
	iolist.pnext[n] = p->iolist.pnext[n];
      p->iolist.iobmp &= ~(1 << n);
    }
}

void
jbus::iolist_deinit (io *p)
{
  for (int i = 0; i < 32; i++)
    iolist_del (p, i);
}

void
jbus::iolist_update (io *p, unsigned int iobmp, unsigned int iobmp_and)
{
  unsigned int iobmp_xor = (iobmp ^ p->iolist.iobmp) & 0xffffffff & iobmp_and;
  for (int i = 0; iobmp_xor; i++)
    {
      const unsigned int bit = (1 << i);
      if (iobmp_xor & bit)
	{
	  if (iobmp & bit)
	    iolist_add (p, i);
	  else
	    iolist_del (p, i);
	  iobmp_xor ^= bit;
	}
    }
}

void
jbus::io::set_memory_iobmp (unsigned int iobmp)
{
  bus.iolist_update (this, iobmp & 0xffff, 0xffff);
}

void
jbus::io::set_ioport_iobmp (unsigned int iobmp)
{
  bus.iolist_update (this, iobmp << 16, 0xffff0000);
}

jbus::io::io (jbus &bus) : bus (bus)
{
  bus.iolist_init (this);
}

jbus::io::~io ()
{
  bus.iolist_deinit (this);
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
  iolist_init ();
}

jbus::~jbus ()
{
  if (!is_iolist_empty ())
    std::cerr << "Deinitializing a bus before devices connected to the bus"
	      << std::endl;
}

unsigned int
jbus::memory_read (unsigned int addr, int &cycles)
{
  unsigned int val = 0xff;
  addr &= 0xfffff;
  const unsigned int n = (addr >> 16) & 0xf;
  for (io *p = iolist.next[n]; p; p = p->iolist.next[n])
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
  const unsigned int n = (addr >> 16) & 0xf;
  for (io *p = iolist.next[n]; p; p = p->iolist.next[n])
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
  const unsigned int n = ((addr >> 4) & 0xf) + 16;
  for (io *p = iolist.next[n]; p; p = p->iolist.next[n])
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
  const unsigned int n = ((addr >> 4) & 0xf) + 16;
  for (io *p = iolist.next[n]; p; p = p->iolist.next[n])
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
