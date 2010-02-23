/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, Jan Stoess, IBM Corporation
 *
 * File path:       vtorus.h
 * Description:     Virtual BlueGene DMA
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "torus.h"
#include "std.h"

class vm_t;
class vtorus_t;

class vtorus_dma_t : public torus_dma_t
{
private:
    
    vtorus_t *const torus; // Parent Tree Device
    vm_t     *const vm;    // Parent VM

    // Shadow FIFOs
    union {
        fifo_t inj_fifo[nr_inj_fifos];
        u32_t  inj_fifo_reg[INJ_FIFO_SIZE/sizeof(u32_t)];
    };
    
    union {
        fifo_t rcv_fifo[nr_rcv_fifos];
        u32_t  rcv_fifo_reg[RCV_FIFO_SIZE/sizeof(u32_t)];
    };
    
    // Shadow Counter Members
    struct 
    {
        word_t base;
    } inj_counter[nr_counters];
    
    struct 
    {
        word_t base;
        word_t limit;
    } rcv_counter[nr_counters];

       
public:
    // Shadow Injection Descriptors (DMA only so far)
    static const word_t nr_inj_entries = 256;
    
    
    struct inj_fifo_area_t
    {
        union
        {
            inj_desc_t desc[nr_inj_entries];
            u8_t       raw[nr_inj_entries * sizeof(inj_desc_t)];
        } ALIGN(32) fifo[nr_inj_fifos];
    };
    
    inj_fifo_area_t &fifo_area;
public:

    word_t gpr_load     (Register, word_t *);
    word_t gpr_store    (Register, word_t *);

    vtorus_dma_t(vtorus_t *t, vm_t *v, word_t g, inj_fifo_area_t &fa) : torus_dma_t (g),
                                                                        torus(t),
                                                                        vm (v),
                                                                        fifo_area (fa)
        { 
            for (word_t i=0; i < INJ_FIFO_SIZE/sizeof(u32_t) / sizeof(word_t); i++)
                inj_fifo_reg[i] = 0;
            for (word_t i=0; i < RCV_FIFO_SIZE/sizeof(u32_t) / sizeof(word_t); i++)
                rcv_fifo_reg[i] = 0;
            for (word_t i=0; i < nr_counters; i++)
                inj_counter[i].base = rcv_counter[i].base = rcv_counter[i].limit = 0;
            for (word_t i=0; i < nr_inj_fifos; i++)
               for (word_t j=0; j < nr_inj_entries * sizeof(inj_desc_t); j++)
                   fifo_area.fifo[i].raw[j] = 0;
        }
};
   
    
    
class vtorus_t : public torus_t
{
    friend class vtorus_dma_t;
private:
    /* 
     * When protecting DMA physical addresses we assume:
     *   GPA begins at 0
     *   Segment-based GPA->HPA mapping, no paging
     */

    word_t base;
    spinlock_t      lock;                       // Torus lock
    vm_t *const     vm;                         // Parent VM
    vtorus_dma_t::inj_fifo_area_t fifo_area[4]; // Shadow injection descriptor area for all groups (contiguously allocated)
    dmadcr_t        dmadcr;                     // Shadow DMA DCR Map
    vtorus_dma_t    dma0;                       // DMA Group 0
    vtorus_dma_t    dma1;                       // DMA Group 1
    vtorus_dma_t    dma2;                       // DMA Group 0
    vtorus_dma_t    dma3;                       // DMA Group 1
    bool fifo_area_enabled;

    word_t gpreg_to_hpreg(word_t gpa_reg, paddr_t hpa) const;
    word_t hpreg_to_gpreg(word_t hpa_reg) const;

    
public:

    void dcr_read   (Dcr, word_t *);
    void dcr_write  (Dcr, word_t);

    void dmadcr_read   (DmaDcr, word_t *);
    void dmadcr_write  (DmaDcr, word_t);

    word_t ack_irq(word_t);
    bool handle_hwirq(word_t irq, word_t &core_mask);

    vtorus_dma_t * const dma;

    vtorus_t (vm_t *v) : vm (v),
                         dma0 (this, v, 0, fifo_area[0]),
                         dma1 (this, v, 1, fifo_area[1]),
                         dma2 (this, v, 2, fifo_area[2]),
                         dma3 (this, v, 3, fifo_area[3]),
                         fifo_area_enabled(false),
                         dma(&dma0) 
        
        { }
    
   
};
