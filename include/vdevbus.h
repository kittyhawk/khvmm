/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vdevbus.h
 * Description:     Virtual BlueGene Ethernet Devbus
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class vdevbus_t
{
    private:
        word_t  ier;
        word_t  l3cer;
        word_t  tecr;
        word_t  xgcr0;
        word_t  xgcr1;
        word_t  xgcr2;
        word_t  xgsr;
        word_t  darbcsr;

    public:
        // See: bgp/arch/include/bpcore/bgp_devbus_memmap.h
        enum Register
        {
            IER     = 0x00,     // Interrrupt Enable Register
            L3CER   = 0x04,     // L3 Coherency Enable Register
            TECR    = 0x08,     // TOMAL and Emac Control Register
            XGCR0   = 0x0C,     // XGXS Control Register 0
            XGCR1   = 0x10,     // XGXS Control Register 1
            XGCR2   = 0x14,     // XGXS Control Register 2
            XGSR    = 0x18,     // XGXS Status Register
            DARBCSR = 0x1C      // DMA Arbiter Control/Status Register
        };

        word_t gpr_load   (Register, word_t *);
        word_t gpr_store  (Register, word_t *);
};
