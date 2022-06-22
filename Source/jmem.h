// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class jmem
{
private:
  unsigned char* mem;
  int size;

public:
  jmem (int memsize);
  ~jmem();
  inline unsigned char read (int offset) { return mem[offset]; }
  inline void write (int offset, unsigned char data) { mem[offset] = data; }
  void loadrom (int offset, const char* filename, int loadsize);
  int loadrom2 (int offset, const char* filename, int maxloadsize);
  void clearrom();
};
