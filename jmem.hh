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

class jmem
{
 private:
  unsigned char *mem;
  int size;
 public:
  jmem (int memsize);
  ~jmem ();
  inline unsigned char read (int offset) { return mem[offset]; }
  inline void write (int offset, unsigned char data) { mem[offset] = data; }
  void loadrom (int offset, const char *filename, int loadsize);
  int loadrom2 (int offset, const char *filename, int maxloadsize);
  void clearrom ();
};

