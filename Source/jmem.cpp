// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "jmem.h"
#include <fstream>

using std::ifstream;
using std::ios;

jmem::jmem (int memsize)
{
  size = memsize;
  mem = new unsigned char[memsize];
}

void jmem::loadrom (int offset, const char* filename, int loadsize)
{
  ifstream file (filename, ios::in | ios::binary);

  if (!file)
    throw ((char*)"can't open");
  file.read ((char*)&mem[offset], loadsize);
  if (!file.good())
    throw ((char*)"can't read");
}

int jmem::loadrom2 (int offset, const char* filename, int maxloadsize)
{
  ifstream file (filename, ios::in | ios::binary);

  if (!file)
    throw ((char*)"can't open");
  file.seekg (0, file.end);
  int loadsize = file.tellg();
  file.seekg (0, file.beg);
  file.read ((char*)&mem[offset], loadsize);
  if (!file.good())
    throw ((char*)"can't read");
  return loadsize;
}

jmem::~jmem()
{
  delete[] mem;
}

void jmem::clearrom()
{
  int i;

  for (i = 0; i < size; i++)
    write (i, 0xee);
}
