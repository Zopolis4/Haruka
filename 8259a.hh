// Copyright (C) 2000-2016 Hideki EIRAKU
// SPDX-License-Identifier: GPL-2.0-or-later

typedef unsigned int t16;

void trigger_irq8259 (unsigned int);
void untrigger_irq8259 (unsigned int);
t16 read8259 (t16);
void write8259 (t16, t16);
t16 interrupt_iac (void);
