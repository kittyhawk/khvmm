/*********************************************************************
 *
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       ibm440.h
 * Description:     ibm 440 specific registers
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

extern inline word_t mfdcrx(word_t dcrn)
{
    word_t value;
    asm volatile ("mfdcrx %0,%1": "=r" (value) : "r" (dcrn) : "memory");
    return value;
}

extern inline void mtdcrx(word_t dcrn, word_t value)
{
    asm volatile("mtdcrx %0,%1": :"r" (dcrn), "r" (value) : "memory");
}
