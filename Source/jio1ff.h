// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "jbus.h"

class jio1ffstatus : public jbus::io
{
  bool base1_rom, base2_rom;
  bool ex_video;

public:
  jio1ffstatus (jbus& bus);
  void ioport_read (unsigned int addr, unsigned int& val, int& cycles);
  void ioport_write (unsigned int addr, unsigned int val, int& cycles);
  inline void set_base1_rom (bool d) { base1_rom = d; }
  inline void set_base2_rom (bool d) { base2_rom = d; }
  void set_ex_video (bool d);
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
    unsigned int cmpand;
    unsigned int cmp;

  public:
    conf (unsigned int index, unsigned int andreg1, unsigned int andreg2, unsigned int orreg1,
          unsigned int orreg2);
    conf (unsigned int index, unsigned int andreg1, unsigned int andreg2, unsigned int orreg1,
          unsigned int orreg2, unsigned int cmpand, unsigned int cmp);
    unsigned int get_iobmp();
    bool is_io_block();
    bool is_my_mem_addr (unsigned int addr, unsigned int bit);
    bool is_my_memread_addr (unsigned int addr);
    bool is_my_memwrite_addr (unsigned int addr);
    bool is_my_io_addr (unsigned int addr);
    void read1ff();
    bool write1ff (unsigned int val);
  };
  jio1ffdev (jbus& bus, conf c);
  virtual void memory_read (unsigned int addr, unsigned int& val, int& cycles);
  virtual void memory_write (unsigned int addr, unsigned int val, int& cycles);
  virtual void ioport_read (unsigned int addr, unsigned int& val, int& cycles);
  virtual void ioport_write (unsigned int addr, unsigned int val, int& cycles);

private:
  class dev : public jbus::io
  {
    conf c;
    jio1ffdev& d;

  public:
    dev (jbus& bus, conf c, jio1ffdev& d);
    void memory_read (unsigned int addr, unsigned int& val, int& cycles);
    void memory_write (unsigned int addr, unsigned int val, int& cycles);
    void ioport_read (unsigned int addr, unsigned int& val, int& cycles);
    void ioport_write (unsigned int addr, unsigned int val, int& cycles);
  };
  dev d;
};
