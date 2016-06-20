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

class jbus
{
public:
  class io
  {
  private:
    io *next;
    io **pnext;
    io ***iolist_tailnext;
  public:
    io (jbus &bus);
    ~io ();
    io *get_next ();
    virtual void memory_read (unsigned int addr, unsigned int &val,
			      int &cycles);
    virtual void memory_write (unsigned int addr, unsigned int val,
			       int &cycles);
    virtual void ioport_read (unsigned int addr, unsigned int &val,
			      int &cycles);
    virtual void ioport_write (unsigned int addr, unsigned int val,
			       int &cycles);
  };
private:
  io *iolist;
  io **iolist_tailnext;
public:
  jbus ();
  ~jbus ();
  unsigned int memory_read (unsigned int addr, int &cycles);
  void memory_write (unsigned int addr, unsigned int val, int &cycles);
  unsigned int ioport_read (unsigned int addr, int &cycles);
  void ioport_write (unsigned int addr, unsigned int val, int &cycles);
  unsigned int memory_read (unsigned int addr);
  void memory_write (unsigned int addr, unsigned int val);
  unsigned int ioport_read (unsigned int addr);
  void ioport_write (unsigned int addr, unsigned int val);
};
