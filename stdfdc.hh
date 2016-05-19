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

class stdfdc : public jfdc
{
private:
  struct fddinfo
  {
    bool insert;
    bool writable;
    char *filename;
  };
  fddinfo fdd[4];
protected:
  virtual void read ();
  virtual void postread ();
  virtual void preformat ();
  virtual void format ();
  virtual void prewrite ();
  virtual void write ();
  virtual void recalibrate ();
  virtual void seek ();
public:
  stdfdc (jvideo *d);
  void insert (t16 drive, char *filename);
  void eject (t16 drive);
};

