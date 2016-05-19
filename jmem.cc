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

using namespace std;

#include "jmem.hh"
#include <fstream>

jmem::jmem (int memsize)
{
  size = memsize;
  mem = new unsigned char[memsize];
}

void
jmem::loadrom (int offset, const char *filename, int loadsize)
{
  ifstream file (filename, ios::in | ios::binary);

  if (!file)
    throw ((char*)"can't open");
  file.read ((char *)&mem[offset], loadsize);
  if (!file.good ())
    throw ((char*)"can't read");
}

jmem::~jmem ()
{
  delete[] mem;
}

void
jmem::clearrom ()
{
  int i;

  for (i = 0 ; i < size ; i++)
    write (i, 0xee);
}

