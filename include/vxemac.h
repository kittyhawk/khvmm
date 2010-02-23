/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vxemac.h
 * Description:     Virtual BlueGene Ethernet Media Access Controller
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class vxemac_t
{
    private:
        word_t      mr0;
        word_t      mr1;
        word_t      tmr0;
        word_t      tmr1;
        word_t      rmr;
        word_t      isr;
        word_t      iser;
        word_t      vtpid;
        word_t      ptr;
        word_t      ipgvr;
        word_t      stacr;
        word_t      rwmr;
        word_t      revid;

    public:
        enum Register         // See: bgp/arch/include/bpcore/bgp_xemac_memmap.h
        {
            MR0    = 0x00,    // Mode Register 0
            MR1    = 0x04,    // Mode Register 1 (changeable only after reset)
            TMR0   = 0x08,    // Transmit Mode Register 0
            TMR1   = 0x0C,    // Transmit Mode Register 1
            RMR    = 0x10,    // Receive Mode Register
            ISR    = 0x14,    // Interrupt Status Register
            ISER   = 0x18,    // Interrupt Status Enable Register
            IAHR   = 0x1C,    // Individual Address High (Hi-Order Short of MAC)
            IALR   = 0x20,    // Individual Address Low  (Low-Order Long of MAC)
            VTPID  = 0x24,    // VLAN TPID Register
            VTCI   = 0x28,    // VLAN TCI Tag Control Info
            PTR    = 0x2C,    // Pause Timer Register
            IAHT1  = 0x30,    // Individual Address Hash Table 1 (16-bits)
            IAHT2  = 0x34,    // Individual Address Hash Table 2 (16-bits)
            IAHT3  = 0x38,    // Individual Address Hash Table 3 (16-bits)
            IAHT4  = 0x3C,    // Individual Address Hash Table 4 (16-bits)
            GAHT1  = 0x40,    // Group Address Hash Table 1 (16-bits)
            GAHT2  = 0x44,    // Group Address Hash Table 2 (16-bits)
            GAHT3  = 0x48,    // Group Address Hash Table 3 (16-bits)
            GAHT4  = 0x4C,    // Group Address Hash Table 4 (16-bits)
            LSAH   = 0x50,    // Last Source Address High (high 16-bits)
            LSAL   = 0x54,    // Last Source Address Low (low 32-bits)
            IPGVR  = 0x58,    // Inter-Packet Gap Value in 8 bit-times
            STACR  = 0x5C,    // STA Control Register
            TRTR   = 0x60,    // Transmit Request Threshold Register
            RWMR   = 0x64,    // Receive Low/High Water Mark Register
            SOPCM  = 0x68,    // SOP Command Mode
            SIAHR  = 0x6C,    // Secondary Indiv Addr High (Hi-Order Short of MAC)
            SIALR  = 0x70,    // Secondary Indiv Addr Low  (Low-Order Long of MAC)
            TXOC1  = 0x74,    // Transmitted Octets (bytes) Register 1
            TXOC2  = 0x78,    // Transmitted Octets (bytes) Register 2
            RXOC1  = 0x7C,    // Received Octets (bytes) Register 1
            RXOC2  = 0x80,    // Received Octets (bytes) Register 2
            REVID  = 0x84,    // Revision ID Register
            HWDBG  = 0x88     // Hardware Debug Register
        };

        // Register reset values
        vxemac_t() : mr0   (0xc0000000),
                     mr1   (0x120020),
                     tmr0  (0x7),
                     tmr1  (0x2200420),
                     rmr   (0x7),
                     isr   (0x0),
                     iser  (0x0),
                     vtpid (0x8100),
                     ptr   (0xffff),
                     ipgvr (0xc0d0000),
                     stacr (0x1000),
                     rwmr  (0x6000700),
                     revid (0x10) {}

        word_t gpr_load     (Register, word_t *);
        word_t gpr_store    (Register, word_t *);
};
