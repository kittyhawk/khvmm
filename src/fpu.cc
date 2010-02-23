/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       fpu.cc
 * Description:     FPU Access
 *
 * All rights reserved
 *
 ********************************************************************/

#include "fpu.h"

void fpu_t::read (unsigned n, fpu_word_t *fpr)
{
    assert(((word_t) fpr & 0xF) == 0);
        
    switch (n) {
        case  0: asm volatile ("stfpdx  0, 0, %0" : : "b" (fpr) : "memory"); break;
        case  1: asm volatile ("stfpdx  1, 0, %0" : : "b" (fpr) : "memory"); break;
        case  2: asm volatile ("stfpdx  2, 0, %0" : : "b" (fpr) : "memory"); break;
        case  3: asm volatile ("stfpdx  3, 0, %0" : : "b" (fpr) : "memory"); break;
        case  4: asm volatile ("stfpdx  4, 0, %0" : : "b" (fpr) : "memory"); break;
        case  5: asm volatile ("stfpdx  5, 0, %0" : : "b" (fpr) : "memory"); break;
        case  6: asm volatile ("stfpdx  6, 0, %0" : : "b" (fpr) : "memory"); break;
        case  7: asm volatile ("stfpdx  7, 0, %0" : : "b" (fpr) : "memory"); break;
        case  8: asm volatile ("stfpdx  8, 0, %0" : : "b" (fpr) : "memory"); break;
        case  9: asm volatile ("stfpdx  9, 0, %0" : : "b" (fpr) : "memory"); break;
        case 10: asm volatile ("stfpdx 10, 0, %0" : : "b" (fpr) : "memory"); break;
        case 11: asm volatile ("stfpdx 11, 0, %0" : : "b" (fpr) : "memory"); break;
        case 12: asm volatile ("stfpdx 12, 0, %0" : : "b" (fpr) : "memory"); break;
        case 13: asm volatile ("stfpdx 13, 0, %0" : : "b" (fpr) : "memory"); break;
        case 14: asm volatile ("stfpdx 14, 0, %0" : : "b" (fpr) : "memory"); break;
        case 15: asm volatile ("stfpdx 15, 0, %0" : : "b" (fpr) : "memory"); break;
        case 16: asm volatile ("stfpdx 16, 0, %0" : : "b" (fpr) : "memory"); break;
        case 17: asm volatile ("stfpdx 17, 0, %0" : : "b" (fpr) : "memory"); break;
        case 18: asm volatile ("stfpdx 18, 0, %0" : : "b" (fpr) : "memory"); break;
        case 19: asm volatile ("stfpdx 19, 0, %0" : : "b" (fpr) : "memory"); break;
        case 20: asm volatile ("stfpdx 20, 0, %0" : : "b" (fpr) : "memory"); break;
        case 21: asm volatile ("stfpdx 21, 0, %0" : : "b" (fpr) : "memory"); break;
        case 22: asm volatile ("stfpdx 22, 0, %0" : : "b" (fpr) : "memory"); break;
        case 23: asm volatile ("stfpdx 23, 0, %0" : : "b" (fpr) : "memory"); break;
        case 24: asm volatile ("stfpdx 24, 0, %0" : : "b" (fpr) : "memory"); break;
        case 25: asm volatile ("stfpdx 25, 0, %0" : : "b" (fpr) : "memory"); break;
        case 26: asm volatile ("stfpdx 26, 0, %0" : : "b" (fpr) : "memory"); break;
        case 27: asm volatile ("stfpdx 27, 0, %0" : : "b" (fpr) : "memory"); break;
        case 28: asm volatile ("stfpdx 28, 0, %0" : : "b" (fpr) : "memory"); break;
        case 29: asm volatile ("stfpdx 29, 0, %0" : : "b" (fpr) : "memory"); break;
        case 30: asm volatile ("stfpdx 30, 0, %0" : : "b" (fpr) : "memory"); break;
        case 31: asm volatile ("stfpdx 31, 0, %0" : : "b" (fpr) : "memory"); break;
    }
}

void fpu_t::write (unsigned n, fpu_word_t *fpr)
{
    assert(((word_t) fpr & 0xF) == 0);
    switch (n) {
        case  0: asm volatile ("lfpdx  0, 0, %0" : : "b" (fpr) : "memory"); break;
        case  1: asm volatile ("lfpdx  1, 0, %0" : : "b" (fpr) : "memory"); break;
        case  2: asm volatile ("lfpdx  2, 0, %0" : : "b" (fpr) : "memory"); break;
        case  3: asm volatile ("lfpdx  3, 0, %0" : : "b" (fpr) : "memory"); break;
        case  4: asm volatile ("lfpdx  4, 0, %0" : : "b" (fpr) : "memory"); break;
        case  5: asm volatile ("lfpdx  5, 0, %0" : : "b" (fpr) : "memory"); break;
        case  6: asm volatile ("lfpdx  6, 0, %0" : : "b" (fpr) : "memory"); break;
        case  7: asm volatile ("lfpdx  7, 0, %0" : : "b" (fpr) : "memory"); break;
        case  8: asm volatile ("lfpdx  8, 0, %0" : : "b" (fpr) : "memory"); break;
        case  9: asm volatile ("lfpdx  9, 0, %0" : : "b" (fpr) : "memory"); break;
        case 10: asm volatile ("lfpdx 10, 0, %0" : : "b" (fpr) : "memory"); break;
        case 11: asm volatile ("lfpdx 11, 0, %0" : : "b" (fpr) : "memory"); break;
        case 12: asm volatile ("lfpdx 12, 0, %0" : : "b" (fpr) : "memory"); break;
        case 13: asm volatile ("lfpdx 13, 0, %0" : : "b" (fpr) : "memory"); break;
        case 14: asm volatile ("lfpdx 14, 0, %0" : : "b" (fpr) : "memory"); break;
        case 15: asm volatile ("lfpdx 15, 0, %0" : : "b" (fpr) : "memory"); break;
        case 16: asm volatile ("lfpdx 16, 0, %0" : : "b" (fpr) : "memory"); break;
        case 17: asm volatile ("lfpdx 17, 0, %0" : : "b" (fpr) : "memory"); break;
        case 18: asm volatile ("lfpdx 18, 0, %0" : : "b" (fpr) : "memory"); break;
        case 19: asm volatile ("lfpdx 19, 0, %0" : : "b" (fpr) : "memory"); break;
        case 20: asm volatile ("lfpdx 20, 0, %0" : : "b" (fpr) : "memory"); break;
        case 21: asm volatile ("lfpdx 21, 0, %0" : : "b" (fpr) : "memory"); break;
        case 22: asm volatile ("lfpdx 22, 0, %0" : : "b" (fpr) : "memory"); break;
        case 23: asm volatile ("lfpdx 23, 0, %0" : : "b" (fpr) : "memory"); break;
        case 24: asm volatile ("lfpdx 24, 0, %0" : : "b" (fpr) : "memory"); break;
        case 25: asm volatile ("lfpdx 25, 0, %0" : : "b" (fpr) : "memory"); break;
        case 26: asm volatile ("lfpdx 26, 0, %0" : : "b" (fpr) : "memory"); break;
        case 27: asm volatile ("lfpdx 27, 0, %0" : : "b" (fpr) : "memory"); break;
        case 28: asm volatile ("lfpdx 28, 0, %0" : : "b" (fpr) : "memory"); break;
        case 29: asm volatile ("lfpdx 29, 0, %0" : : "b" (fpr) : "memory"); break;
        case 30: asm volatile ("lfpdx 30, 0, %0" : : "b" (fpr) : "memory"); break;
        case 31: asm volatile ("lfpdx 31, 0, %0" : : "b" (fpr) : "memory"); break;
    }
}
