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

class jio1ffstatus : public jbus::io
{
  bool base1_rom, base2_rom;
public:
  jio1ffstatus (jbus &bus);
  void ioport_read (unsigned int addr, unsigned int &val, int &cycles);
  void ioport_write (unsigned int addr, unsigned int val, int &cycles);
  inline void set_base1_rom (bool d) { base1_rom = d; }
  inline void set_base2_rom (bool d) { base2_rom = d; }
};

class jio1ffdev
{
public:
  class conf
  {
    unsigned int index;
    unsigned int andreg1;
    unsigned int andreg2;
    unsigned int orreg1;
    unsigned int orreg2;
    unsigned int reg1;
    unsigned int reg2;
    unsigned int curr_index;
    unsigned int curr_state;
  public:
    conf (unsigned int index, unsigned int andreg1, unsigned int andreg2,
	  unsigned int orreg1, unsigned int orreg2);
    bool is_io_block ();
    bool is_my_mem_addr (unsigned int addr, unsigned int bit);
    bool is_my_memread_addr (unsigned int addr);
    bool is_my_memwrite_addr (unsigned int addr);
    bool is_my_io_addr (unsigned int addr);
    void read1ff ();
    void write1ff (unsigned int val);
  };
  jio1ffdev (jbus &bus, conf c);
  virtual void memory_read (unsigned int addr, unsigned int &val, int &cycles);
  virtual void memory_write (unsigned int addr, unsigned int val, int &cycles);
  virtual void ioport_read (unsigned int addr, unsigned int &val, int &cycles);
  virtual void ioport_write (unsigned int addr, unsigned int val, int &cycles);
private:
  class dev : public jbus::io
  {
    conf c;
    jio1ffdev &d;
  public:
    dev (jbus &bus, conf c, jio1ffdev &d);
    void memory_read (unsigned int addr, unsigned int &val, int &cycles);
    void memory_write (unsigned int addr, unsigned int val, int &cycles);
    void ioport_read (unsigned int addr, unsigned int &val, int &cycles);
    void ioport_write (unsigned int addr, unsigned int val, int &cycles);
  };
  dev d;
};
