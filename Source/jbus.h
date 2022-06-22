// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class jbus
{
public:
  class io;

private:
  struct list
  {
    io* next[32];
    io** pnext[32];
    unsigned int iobmp;
  };
  list iolist;
  void iolist_init();
  bool is_iolist_empty();
  void iolist_add (io* p, int n);
  void iolist_del (io* p, int n);
  void iolist_init (io* p);
  void iolist_deinit (io* p);
  void iolist_update (io* p, unsigned int iobmp, unsigned int iobmp_and);

public:
  class io
  {
    friend class jbus;

  private:
    list iolist;
    jbus& bus;

  protected:
    void set_memory_iobmp (unsigned int iobmp);
    void set_ioport_iobmp (unsigned int iobmp);

  public:
    io (jbus& bus);
    ~io();
    virtual void memory_read (unsigned int addr, unsigned int& val, int& cycles);
    virtual void memory_write (unsigned int addr, unsigned int val, int& cycles);
    virtual void ioport_read (unsigned int addr, unsigned int& val, int& cycles);
    virtual void ioport_write (unsigned int addr, unsigned int val, int& cycles);
  };
  jbus();
  ~jbus();
  unsigned int memory_read (unsigned int addr, int& cycles);
  void memory_write (unsigned int addr, unsigned int val, int& cycles);
  unsigned int ioport_read (unsigned int addr, int& cycles);
  void ioport_write (unsigned int addr, unsigned int val, int& cycles);
  unsigned int memory_read (unsigned int addr);
  void memory_write (unsigned int addr, unsigned int val);
  unsigned int ioport_read (unsigned int addr);
  void ioport_write (unsigned int addr, unsigned int val);
};
