
/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       vtree.h
 * Description:     Virtual BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"
#include "sync.h"
#include "tree.h"

class vtree_t;
class vm_t;

/*
 * Virtual Tree Channel
 */
class vtree_chan_t : public tree_chan_t
{
    friend class vtree_t;

    private:
        // FIFO Depth
        static unsigned const nr_fifos = 100;
        static unsigned const fifo_head_size = nr_fifos * 8;
        static unsigned const fifo_data_size = fifo_head_size * 16;

        // Parent Tree Device
        vtree_t * const  tree;

        // Injection and Reception FIFOs
        fpu_word_t  inj_fifo_data[fifo_data_size];
        fpu_word_t  rec_fifo_data[fifo_data_size];
        word_t      inj_fifo_head[fifo_head_size];
        word_t      rec_fifo_head[fifo_head_size];

        // FIFO Counters
        word_t      inj_data_add, inj_data_rem;
        word_t      inj_head_add, inj_head_rem;
        word_t      rec_data_add, rec_data_rem;
        word_t      rec_head_add, rec_head_rem;
        bool        interrupt;

        union {
            word_t  vcfg;   // Virtual Channel Config Register
            struct {
                word_t  receive_all     : 1,
                                        : 7,
                        inj_csum_excl   : 8,
                                        : 5,
                        rec_watermark   : 3,
                                        : 5,
                        inj_watermark   : 3;
            };
        };

        word_t      cspy;   // Injection Checksum Payload
        word_t      cshd;   // Injection Checksum Header

        unsigned inj_data_cnt() const { return inj_data_add - inj_data_rem; }
        unsigned inj_head_cnt() const { return inj_head_add - inj_head_rem; }
        unsigned rec_data_cnt() const { return rec_data_add - rec_data_rem; }
        unsigned rec_head_cnt() const { return rec_head_add - rec_head_rem; }

        word_t status() const
        {
            return min(inj_data_cnt(), 7u) << 24 |
                   min(inj_head_cnt(), 7u) << 16 |
                   min(rec_data_cnt(), 7u) << 8  |
                   min(rec_head_cnt(), 7u)       |
                   interrupt << 4;
        }

        void send_packet();

    public:
        vtree_chan_t (vtree_t *t, unsigned c) : tree_chan_t (c), tree (t),
                                                inj_data_add (0), inj_data_rem (0),
                                                inj_head_add (0), inj_head_rem (0),
                                                rec_data_add (0), rec_data_rem (0),
                                                rec_head_add (0), rec_head_rem (0),
                                                interrupt (0) {}

        word_t fpr_load     (Register, unsigned);
        word_t fpr_store    (Register, unsigned);

        word_t gpr_load     (Register, word_t *);
        word_t gpr_store    (Register, word_t *);

        word_t recv_packet  (word_t);
};

/*
 * Virtual Tree Device
 */
class vtree_t : public tree_t
{
    friend class vtree_chan_t;

private:
    spinlock_t      lock;   // Tree Lock
    vm_t *const     vm;     // Parent VM
    word_t          rdr[8]; // Route Descriptor Registers
    word_t          isr[2]; // Idle Sequence Register
    word_t          rcfg;   // Router Configuration Register
    word_t          rto;    // Router Timeout Register
    word_t          fptr;   // FIFO Pointer Register
    word_t          naddr;  // Node Address Register
    word_t          prxf;   // Processor Reception Exception Flags
    word_t          prxen;  // Processor Reception Exception Enables
    word_t          prda;   // Processor Reception Diagnostic Address
    word_t          prdd;   // Processor Reception Diagnostic Data
    word_t          pixf;   // Processor Injection Exception Flags
    word_t          pixen;  // Processor Injection Exception Enables
    word_t          pida;   // Processor Injection Diagnostic Address
    word_t          pidd;   // Processor Injection Diagnostic Data

public:
    vtree_chan_t    chan0;  // Channel 0
    vtree_chan_t    chan1;  // Channel 1

    vtree_t (vm_t *v) : vm      (v),
                        prxf    (0),
                        prxen   (0),
                        pixf    (PIX_ENABLE),
                        pixen   (0),
                        chan0   (this, 0),
                        chan1   (this, 1) {}

    word_t dcr_read   (Dcr, word_t *);
    word_t dcr_write  (Dcr, word_t);
    
    word_t raise_irqs();
    word_t ack_irq(word_t)
        {
            lock_guard_t<spinlock_t> guard (lock);
            return raise_irqs();
        }
    void set_nodeaddr(word_t val) { naddr = val; }

};
