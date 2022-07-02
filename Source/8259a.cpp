// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

#include "8259a.h"

#include "8088.h"

//  Copyright (C) 2001  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

/* 8259A - Programmable Interrupt Controller */

import <cstdio>;

#define WARN8259(a) fprintf (stderr, "8259A: WARNING: %s\n", a)

#define Bit8u unsigned int
#define Boolean int

typedef unsigned int t16;

/* from pic.h in Bochs-1.2.1 */
typedef struct
{
  Bit8u single_PIC;       /* 0=cascaded PIC, 1=master only */
  Bit8u interrupt_offset; /* programmable interrupt vector offset */
  union
  {
    Bit8u slave_connect_mask; /* for master, a bit for each interrupt line
                                 0=not connect to a slave, 1=connected */
    Bit8u slave_id;           /* for slave, id number of slave PIC */
  } u;
  Bit8u sfnm;            /* specially fully nested mode: 0=no, 1=yes*/
  Bit8u buffered_mode;   /* 0=no buffered mode, 1=buffered mode */
  Bit8u master_slave;    /* master/slave: 0=slave PIC, 1=master PIC */
  Bit8u auto_eoi;        /* 0=manual EOI, 1=automatic EOI */
  Bit8u imr;             /* interrupt mask register, 1=masked */
  Bit8u isr;             /* in service register */
  Bit8u irr;             /* interrupt request register */
  Bit8u read_reg_select; /* 0=IRR, 1=ISR */
  Bit8u irq;             /* current IRQ number */
  Boolean INT;           /* INT request pin of PIC */
  struct
  {
    Boolean in_init;
    Boolean requires_4;
    int byte_expected;
  } init;
  Boolean special_mask;
} bx_pic_t;

/* JX has only one pic. */
bx_pic_t master_pic;

void reset8259 (void)
{
  master_pic.single_PIC = 1;
  master_pic.interrupt_offset = 8;
  master_pic.u.slave_connect_mask = 0;
  master_pic.sfnm = 0;
  master_pic.buffered_mode = 1;
  master_pic.master_slave = 0;
  master_pic.auto_eoi = 0;
  master_pic.imr = 255;
  master_pic.isr = 0;
  master_pic.irr = 0;
  master_pic.read_reg_select = 0;
  master_pic.irq = 0;
  master_pic.INT = 0;
  master_pic.init.in_init = 0;
  master_pic.init.requires_4 = 0;
  master_pic.init.byte_expected = 0;
  master_pic.special_mask = 0;
}

void service_master_pic (void)
{
  t16 unmasked_requests;
  int irq, max_irq;
  t16 isr;

  if (master_pic.INT)
    return;
  if (master_pic.special_mask)
    max_irq = 7;
  else
  {
    isr = master_pic.isr;
    if (isr)
    {
      max_irq = 0;
      while (!(isr & 1))
      {
        isr >>= 1;
        max_irq++;
      }
      if (!max_irq)
        return;
      if (max_irq > 7)
        WARN8259 ("error in service_master_pic ()");
    }
    else
      max_irq = 7;
  }
  unmasked_requests = master_pic.irr & ~master_pic.imr;
  if (unmasked_requests)
  {
    for (irq = 0; irq <= max_irq; irq++)
    {
      if (master_pic.special_mask && ((master_pic.isr >> irq) & 1))
        continue;
      if (unmasked_requests & (1 << irq))
      {
        master_pic.irr &= ~(1 << irq);
        master_pic.INT = 1;
        intr8088 (1);
        master_pic.irq = irq;
        return;
      }
    }
  }
}

t16 interrupt_iac (void)
{
  t16 vector, irq;

  intr8088 (0);
  master_pic.INT = 0;
  master_pic.isr |= (1 << master_pic.irq);
  master_pic.irr &= ~(1 << master_pic.irq);
  irq = master_pic.irq;
  vector = irq + master_pic.interrupt_offset;
  service_master_pic();
  return vector;
}

t16 read8259 (t16 addr)
{
  switch (addr & 1)
  {
  case 0:
    if (master_pic.read_reg_select)
      return master_pic.isr;
    else
      return master_pic.irr;
  case 1:
    return master_pic.imr;
  }
  return 0;
}

void write8259 (t16 addr, t16 v)
{
  int irq;

  switch (addr & 1)
  {
  case 0:
    if (v & 0x10)
    {
      master_pic.init.in_init = 1;
      master_pic.init.requires_4 = v & 1;
      master_pic.init.byte_expected = 2;
      master_pic.imr = 255;
      master_pic.isr = 0;
      master_pic.irr = 0;
      master_pic.INT = 0;
      master_pic.single_PIC = (v & 2) ? 1 : 0;
      intr8088 (0);
      return;
    }
    if ((v & 0x18) == 8)
    {
      t16 special_mask, poll, read_op;

      special_mask = (v & 0x60) >> 5;
      poll = (v & 4) >> 2;
      read_op = v & 3;
      if (poll)
        WARN8259 ("OCW3: poll bit set");
      if (read_op == 2)
        master_pic.read_reg_select = 0;
      else if (read_op == 3)
        master_pic.read_reg_select = 1;
      if (special_mask == 2)
        master_pic.special_mask = 0;
      else if (special_mask == 3)
        master_pic.special_mask = 1, service_master_pic();
      return;
    }
    switch (v)
    {
    case 0:
      WARN8259 ("Rotate in Auto-EOI mode command received.");
      break;
    case 0xa:
      master_pic.read_reg_select = 0;
      break;
    case 0xb:
      master_pic.read_reg_select = 1;
      break;
    case 0x20:
      for (irq = 0; irq <= 7; irq++)
      {
        if (master_pic.isr & (1 << irq))
        {
          master_pic.isr &= ~(1 << irq);
          break;
        }
      }
      service_master_pic();
      break;
    case 0x60:
    case 0x61:
    case 0x62:
    case 0x63:
    case 0x64:
    case 0x65:
    case 0x66:
    case 0x67:
      master_pic.isr &= ~(1 << (v - 0x60));
      service_master_pic();
      break;
    case 0xC0: /* 0 7 6 5 4 3 2 1 */
    case 0xC1: /* 1 0 7 6 5 4 3 2 */
    case 0xC2: /* 2 1 0 7 6 5 4 3 */
    case 0xC3: /* 3 2 1 0 7 6 5 4 */
    case 0xC4: /* 4 3 2 1 0 7 6 5 */
    case 0xC5: /* 5 4 3 2 1 0 7 6 */
    case 0xC6: /* 6 5 4 3 2 1 0 7 */
    case 0xC7: /* 7 6 5 4 3 2 1 0 */
      WARN8259 ("IRQ lowest command");
      break;
    default:
      WARN8259 ("write to port 0x20");
    }
    break;
  case 1:
    if (master_pic.init.in_init)
    {
      switch (master_pic.init.byte_expected)
      {
      case 2:
        master_pic.interrupt_offset = v & 0xf8;
        master_pic.init.byte_expected = 3;
        return;
      case 3:
        /* fixme */
        if (master_pic.init.requires_4)
          master_pic.init.byte_expected = 4;
        else
        {
          master_pic.init.in_init = 0;
          return;
        }
      case 4:
        if (v & 1)
          WARN8259 ("8086 mode");
        else
          WARN8259 ("NOT 8086 mode");
        master_pic.init.in_init = 0;
        return;
      default:
        WARN8259 ("master expecting bad init command");
      }
    }
    master_pic.imr = v;
    service_master_pic();
    return;
  }
}

void trigger_irq8259 (unsigned int irq_no)
{
  int irq_no_bitmask;

  irq_no_bitmask = 1 << irq_no;
  master_pic.irr |= irq_no_bitmask;
  service_master_pic();
}

void untrigger_irq8259 (unsigned int irq_no)
{
  int irq_no_bitmask;

  irq_no_bitmask = 1 << irq_no;
  master_pic.irr &= ~irq_no_bitmask;
  service_master_pic();
}
