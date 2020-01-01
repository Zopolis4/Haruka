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

typedef unsigned int t16;
typedef unsigned int t20;

void printip8088 (void);
void reset8088 (void);
int run8088 (void);
void nmi8088 (int);
void intr8088 (int);
t16 memory_read (t20, int *);
void memory_write (t20, t16, int *);
t16 ioport_read (t20, int *);
void ioport_write (t20, t16, int *);
void interrupt_nmi (void);