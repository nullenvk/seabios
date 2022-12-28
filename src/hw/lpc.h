// Rudimentary support for LPC Interface Bridges
//
// Copyright (C) 2022 nullenvk 
//
// This file may be distributed under the terms of the GNU LGPLv3 license.

#ifndef _LPC_H
#define _LPC_H

#include "types.h"

void lpc_setup(void);

int lpc_can_wprotect(void);
void lpc_wprotect(void);

#endif
