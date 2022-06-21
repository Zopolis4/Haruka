// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

class stdfdc : public jfdc
{
private:
  struct fddinfo
  {
    bool insert;
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
  stdfdc (jvideo &d);
  void insert (t16 drive, char *filename);
  void eject (t16 drive);
};

