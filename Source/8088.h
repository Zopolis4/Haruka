// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

typedef unsigned int t16;
typedef unsigned int t20;

void printip8088 (void);
void reset8088 (void);
int run8088 (void);
void nmi8088 (int);
void intr8088 (int);
t16 memory_read (t20, int*);
void memory_write (t20, t16, int*);
t16 ioport_read (t20, int*);
void ioport_write (t20, t16, int*);
void interrupt_nmi (void);
