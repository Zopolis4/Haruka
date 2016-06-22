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

#include "jbus.hh"
#include "jio1ff.hh"

jio1ffstatus::jio1ffstatus (jbus &bus)
  : io (bus), base1_rom (false), base2_rom (false)
{
  set_memory_iobmp (0);
  set_ioport_iobmp (0x8000);
}

void
jio1ffstatus::ioport_read (unsigned int addr, unsigned int &val, int &cycles)
{
  if (addr == 0x1ff)
    {
      val = 255 ^ (base1_rom ? 32 : 0) ^ (base2_rom ? 64 : 0);
      cycles = 6;
    }
}

void
jio1ffstatus::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
  if (addr == 0x1ff)
    cycles = 6;
}

jio1ffdev::conf::conf (unsigned int index, unsigned int andreg1,
		       unsigned int andreg2, unsigned int orreg1,
		       unsigned int orreg2)
  : index (index), andreg1 (andreg1), andreg2 (andreg2), orreg1 (orreg1),
    orreg2 (orreg2), reg1 (0), reg2 (0), curr_index (0), curr_state (0)
{
}

unsigned int
jio1ffdev::conf::get_iobmp ()
{
  if (!(reg1 & 0x80))		// Disabled
    return 0;
  switch (reg2 & 0x1e)
    {
    case 0x0:
      return 0x01 << ((reg1 & 0x1e) >> 1);
    case 0x2:
      return 0x03 << ((reg1 & 0x1c) >> 1);
    case 0x6:
      return 0x0f << ((reg1 & 0x18) >> 1);
    case 0xe:
      return 0xff << ((reg1 & 0x10) >> 1);
    default:
      return 0xffff;
    }
}

bool
jio1ffdev::conf::is_io_block ()
{
  return index >= 0x80;
}

bool
jio1ffdev::conf::is_my_mem_addr (unsigned int addr, unsigned int bit)
{
  if (is_io_block ())		// I am not a memory block
    return false;
  addr = (addr >> 15) & 037;
  if (reg1 & 0x80)		// Enabled
    {
      if ((reg1 & 0x20) && (reg2 & bit)) // Mem space enabled && rd/wr bit test
	{
	  unsigned int mask = (reg2 & 037) ^ 037;
	  if ((mask & reg1) == (mask & addr))
	    return true;
	}
    }
  return false;
}

bool
jio1ffdev::conf::is_my_memread_addr (unsigned int addr)
{
  return is_my_mem_addr (addr, 0x20);
}

bool
jio1ffdev::conf::is_my_memwrite_addr (unsigned int addr)
{
  return is_my_mem_addr (addr, 0x40);
}

bool
jio1ffdev::conf::is_my_io_addr (unsigned int addr)
{
  if (!is_io_block ())		// I am not an I/O block
    return false;
  addr = (addr >> 3) & 0177;
  if (reg1 & 0x80)		// Enabled
    {
      unsigned int mask = (reg2 & 0177) ^ 0177;
      if ((mask & reg1) == (mask & addr))
	  return true;
    }
  return false;
}

void
jio1ffdev::conf::read1ff ()
{
  curr_state = 0;
}

bool
jio1ffdev::conf::write1ff (unsigned int val)
{
  bool ret = false;
  switch (curr_state++)
    {
    case 0:
      curr_index = val;
      break;
    case 1:
      if (curr_index == index)
	{
	  reg1 = (val & andreg1) | orreg1;
	  ret = true;
	}
      break;
    case 2:
      if (curr_index == index)
	{
	  reg2 = (val & andreg2) | orreg2;
	  ret = true;
	}
      curr_state = 0;
      break;
    }
  return ret;
}

jio1ffdev::jio1ffdev (jbus &bus, conf c)
  : d (bus, c, *this)
{
}

void
jio1ffdev::memory_read (unsigned int addr, unsigned int &val, int &cycles)
{
}

void
jio1ffdev::memory_write (unsigned int addr, unsigned int val, int &cycles)
{
}

void
jio1ffdev::ioport_read (unsigned int addr, unsigned int &val, int &cycles)
{
}

void
jio1ffdev::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
}

jio1ffdev::dev::dev (jbus &bus, conf c, jio1ffdev &d)
  : io (bus), c (c), d (d)
{
  if (c.is_io_block ())
    set_memory_iobmp (0);
  else
    set_ioport_iobmp (0x8000);
}

void
jio1ffdev::dev::memory_read (unsigned int addr, unsigned int &val, int &cycles)
{
  if (c.is_my_memread_addr (addr))
    d.memory_read (addr, val, cycles);
}

void
jio1ffdev::dev::memory_write (unsigned int addr, unsigned int val, int &cycles)
{
  if (c.is_my_memwrite_addr (addr))
    d.memory_write (addr, val, cycles);
}

void
jio1ffdev::dev::ioport_read (unsigned int addr, unsigned int &val, int &cycles)
{
  if (addr == 0x1ff)
    c.read1ff ();
  if (c.is_my_io_addr (addr))
    d.ioport_read (addr, val, cycles);
}

void
jio1ffdev::dev::ioport_write (unsigned int addr, unsigned int val, int &cycles)
{
  if (addr == 0x1ff && c.write1ff (val))
    {
      if (c.is_io_block ())
	set_ioport_iobmp (c.get_iobmp () | 0x8000);
      else
	set_memory_iobmp (c.get_iobmp ());
    }
  if (c.is_my_io_addr (addr))
    d.ioport_write (addr, val, cycles);
}
