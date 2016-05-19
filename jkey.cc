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

#include "jtype.hh"
#include "jkey.hh"

jkey::jkey ()
{
  nmiflag = 0;
  nmikeycode = 128;
  nmikeycode2 = 0;
  nmiclks = 0;
  keyrepeatcount = 0;
  keydata = 0;
  skipclks = 0;
  nmikeycodes[0] = 
    nmikeycodes[1] = 
    nmikeycodes[2] = 
    nmikeycodes[3] = 0;
}

void
jkey::keydown (int code)
{
  int i;

  /*
  if (code == 0x48) code = 0x79;
  if (code == 0x4b) code = 0x75;
  if (code == 0x4d) code = 0x77;
  if (code == 0x50) code = 0x73;
  */
  for (i = 0 ; i < 4 ; i++)
    if (!nmikeycodes[i])
      break;
  if (i == 4)
    return;
  nmikeycodes[i] = code;
}

void
jkey::keyup (int code)
{
  int i;

  /*
  if (code == 0x48) code = 0x79;
  if (code == 0x4b) code = 0x75;
  if (code == 0x4d) code = 0x77;
  if (code == 0x50) code = 0x73;
  */
  for (i = 0 ; i < 4 ; i++)
    if (!nmikeycodes[i])
      break;
  if (i == 4)
    return;
  nmikeycodes[i] = code | 128;
}

t16
jkey::in ()
{
  if (nmiflag != 1)
    nmiflag = 0;
  keydata = 0;
  return data0;
}

void
jkey::out (t16 d)
{
  data0 = d;
}

bool
jkey::clkin (int clk)
{
  bool f;
  int i;

  f = false;
  if (skipclks)
    {
      skipclks -= clk;
      if (skipclks < 0)
	skipclks = 0;
    }
  if (nmiflag >= 1)
    {
      nmiclks += clk;
      if (nmiclks >= 3150/*+340*/)
	{
	  nmiclks -= 3150/*+340*/;
	  if (nmiflag == 1)
	    {
	      if (skipclks == 0)
		{
		  if (data0 & 128)
		    {
		      nmiflag = 2;
		      f = true;
		      //callnmi ();
		      //nmiclks = 0;
		      keydata = 1;
		      skipclks = 14318180 / 32;
		    }
		}
	      //else
	      //  nmiflag = 0;
	    }
	  else//if (nmiflag >= 2)
	    {
	      nmiflag++;
	      switch (nmiflag)
		{
		case 3:
		  keydata = 0;
		  break;
		case 4:
		  keydata = nmikeycode & 1;
		  break;
		case 5:
		  keydata = !(nmikeycode & 1);
		  break;
		case 6:
		  keydata = nmikeycode & 2;
		  break;
		case 7:
		  keydata = !(nmikeycode & 2);
		  break;
		case 8:
		  keydata = nmikeycode & 4;
		  break;
		case 9:
		  keydata = !(nmikeycode & 4);
		  break;
		case 10:
		  keydata = nmikeycode & 8;
		  break;
		case 11:
		  keydata = !(nmikeycode & 8);
		  break;
		case 12:
		  keydata = nmikeycode & 16;
		  break;
		case 13:
		  keydata = !(nmikeycode & 16);
		  break;
		case 14:
		  keydata = nmikeycode & 32;
		  break;
		case 15:
		  keydata = !(nmikeycode & 32);
		  break;
		case 16:
		  keydata = nmikeycode & 64;
		  break;
		case 17:
		  keydata = !(nmikeycode & 64);
		  break;
		case 18:
		  keydata = nmikeycode & 128;
		  break;
		case 19:
		  keydata = !(nmikeycode & 128);
		  break;
		case 20:
		  {
		    int i, j = nmikeycode;
		    keydata = 1;
		    for (i=0;i<8;i++,j>>=1)
		      keydata ^= j & 1;
		  }
		  break;
		case 21:
		  {
		    int i, j = nmikeycode;
		    keydata = 0;
		    for (i=0;i<8;i++,j>>=1)
		      keydata ^= j & 1;
		  }
		  break;
		default:
		  nmiflag=22;
		}
	    }
	}
    }
  else
    {
      if (nmikeycode2)
	{
	  nmikeycode = nmikeycode2;
	  nmikeycode2 = 0;
	  nmiflag = 1;
	  keyrepeatcount = (int)(14318180 * 0.6);
	}
    }
  if (!(nmikeycode & 128))
    {
      keyrepeatcount -= clk;
      if (keyrepeatcount < 0)
	{
	  keyrepeatcount += 14318180 / 11;
	  if (!nmiflag)
	    nmiflag = 1;
	}
    }
  for (i = 4 - 2 ; i >= 0 ; i--)
    if (nmikeycodes[i] == 0 && nmikeycodes[i + 1] != 0)
      nmikeycodes[i] = nmikeycodes[i + 1], nmikeycodes[i + 1] = 0;
  if (nmikeycode2 == 0 && nmikeycodes[0] != 0)
    nmikeycode2 = nmikeycodes[0], nmikeycodes[0] = 0;
  return f;
}

