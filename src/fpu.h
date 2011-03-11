/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       fpu.h
 * Description:     FPU Access
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

asm (".macro lfpdx  frt, idx, reg; .long ((31<<26)|((\\frt)<<21)|(\\idx<<16)|(\\reg<<11)|(462<<1)); .endm");
asm (".macro stfpdx frt, idx, reg; .long ((31<<26)|((\\frt)<<21)|(\\idx<<16)|(\\reg<<11)|(974<<1)); .endm");

class fpu_t
{
    public:
        static void read    (unsigned, fpu_word_t *);
        static void write   (unsigned, fpu_word_t *);
};
