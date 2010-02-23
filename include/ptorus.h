/*********************************************************************
 *
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       torus.cc
 * Description:     BlueGene DMA
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "config.h"
#include "torus.h"
#include "ibm440.h"
#include "resource.h"
#include "vdp83820.h"

class ptorus_t;

class ptorus_dma_t : public torus_dma_t
{
    friend class vtorus_dma_t;
    
private:
    ptorus_t *   torus;
    word_t       base;

    word_t gpr_read (Register reg)
        {
            return *reinterpret_cast<word_t volatile *>(base + reg);
        }
    
    void gpr_write (Register reg, word_t val)
        {
            *reinterpret_cast<word_t volatile *>(base + reg) = val;
        }

public:
    
    void reset()
        {
        
            /* disable all counters, init all fifos so that nothing bad can happen... */
            for (word_t c = 0; c < nr_counters; c++) 
            {
                gpr_write(INJ_CTR_REG(c, CTR_CTR), ~0ul);
                gpr_write(INJ_CTR_REG(c, CTR_BASE), 0ul);

                gpr_write(RCV_CTR_REG(c, CTR_CTR), ~0ul);
                gpr_write(RCV_CTR_REG(c, CTR_BASE), 0ul);
                gpr_write(RCV_CTR_REG(c, CTR_LIMIT), ~0ul);
                
            }
            gpr_write(INJ_CNTR_CNF_REG(0,  CNF_DISABLE), ~0ul);
            gpr_write(INJ_CNTR_CNF_REG(32, CNF_DISABLE), ~0ul);
            gpr_write(RCV_CNTR_CNF_REG(0,  CNF_DISABLE), ~0ul);
            gpr_write(RCV_CNTR_CNF_REG(32, CNF_DISABLE), ~0ul);

            
            gpr_write(INJ_FIFO_DEACTIVATE, ~0U);

            for (word_t f = 0; f < nr_inj_fifos; f++) 
            {
                gpr_write(INJ_FIFO_REG(f, FIFO_START), 0);
                gpr_write(INJ_FIFO_REG(f, FIFO_END), 0);
                gpr_write(INJ_FIFO_REG(f, FIFO_HEAD), 0);
                gpr_write(INJ_FIFO_REG(f, FIFO_TAIL), 0);
            }

            for (word_t f = 0; f < nr_rcv_fifos; f++) 
            {
                gpr_write(RCV_FIFO_REG(f, FIFO_START), 0);
                gpr_write(RCV_FIFO_REG(f, FIFO_END),   0);
                gpr_write(RCV_FIFO_REG(f, FIFO_HEAD), 0);
                gpr_write(RCV_FIFO_REG(f, FIFO_TAIL), 0);
            }
            
            /* clear thresholds */
            gpr_write(INJ_FIFO_THRESHOLD_CLR, ~0ul);
            gpr_write(Register(RCV_FIFO_THRESHOLD_CLR0+0), ~0ul);
            gpr_write(Register(RCV_FIFO_THRESHOLD_CLR0+4), ~0ul);


        }
    
    void init_inj_fifo(word_t fifo, paddr_t paddr, word_t size)
        {
            assert(fifo < nr_inj_fifos);
            assert(!(paddr & 0xf));
            
            gpr_write(INJ_FIFO_REG(fifo, FIFO_START), paddr >> 4);
            gpr_write(INJ_FIFO_REG(fifo, FIFO_END  ), (paddr + size) >> 4);
            gpr_write(INJ_FIFO_REG(fifo, FIFO_HEAD ), paddr >> 4);
            gpr_write(INJ_FIFO_REG(fifo, FIFO_TAIL ), paddr >> 4);
           
        }

    paddr_t get_inj_fifo_head(word_t fifo)
        {
            assert(fifo < nr_inj_fifos);
            return (gpr_read(INJ_FIFO_REG(fifo, FIFO_HEAD)) << 4);
        }
    void set_inj_fifo_tail(word_t fifo, paddr_t paddr)
        {
            assert(fifo < nr_inj_fifos);
            assert(!(paddr & 0xf));
            gpr_write(INJ_FIFO_REG(fifo, FIFO_TAIL), paddr >> 4);
        }
 
    void enable_inj_counter(word_t counter, paddr_t paddr)
        {
            assert(counter < nr_counters);
            

            gpr_write(INJ_CTR_REG(counter, CTR_CTR), 0);
            gpr_write(INJ_CTR_REG(counter, CTR_BASE), paddr >> 4);
            
            gpr_write(INJ_CNTR_CNF_REG(counter, CNF_ENABLE), INJ_CTR_MASK(counter));
            gpr_write(INJ_CNTR_CNF_REG(counter, CNF_HITZERO_CLR), INJ_CTR_MASK(counter));
            
        }
    
    void enable_inj_counter_hitzero_irq(word_t counter, word_t vector=0);
    word_t get_inj_counter_group_status()
        { 
            return gpr_read(INJ_CNTR_GROUP_STS);
        }

    word_t get_inj_counter_hitzero_mask(word_t counter)
        { 
            assert(counter < nr_counters);
            return gpr_read(INJ_CNTR_CNF_REG(counter, CNF_HITZERO));
        }
    void clear_inj_counter_hitzero_mask(word_t counter)
        { 
            assert(counter < nr_counters);
            gpr_write(INJ_CNTR_CNF_REG(counter, CNF_HITZERO_CLR), INJ_CTR_MASK(counter)); 
        }
    void inc_inj_counter(word_t counter, word_t inc)
        {
            assert(counter < nr_counters);
            gpr_write(INJ_CTR_REG(counter, CTR_INC), inc);
        }
    void activate_inj_dma(word_t fifo)
        { gpr_write(INJ_FIFO_DMA_ACTIVATE, 0x80000000U >> fifo); }
    
    
    void init_rcv_fifo(word_t fifo, paddr_t paddr, word_t size)
        {
            assert(fifo < nr_rcv_fifos);
            assert(!(paddr & 0xf));
            
            gpr_write(RCV_FIFO_REG(fifo, FIFO_START), paddr >> 4);
            gpr_write(RCV_FIFO_REG(fifo, FIFO_END  ), (paddr + size) >> 4);
            gpr_write(RCV_FIFO_REG(fifo, FIFO_HEAD ), paddr >> 4);
            gpr_write(RCV_FIFO_REG(fifo, FIFO_TAIL ), paddr >> 4);
           
        }
    
    void enable_rcv_fifo_threshold_irq(word_t fifo, word_t type=0);
    word_t get_rcv_fifo_not_not_empty_mask(word_t fifo)
        { 
            return gpr_read(Register(RCV_FIFO_NOT_EMPTY0 + (rcv_fifo_id(fifo) / 32)));
        }
    void clear_rcv_threshold_mask(word_t fifo)
        { 
            assert(fifo < nr_rcv_fifos);
            gpr_write(Register(RCV_FIFO_THRESHOLD_CLR0 + (rcv_fifo_id(fifo) / 32)), (0x80000000 >> (rcv_fifo_id(fifo) % 32)));
        }
    
    paddr_t get_rcv_fifo_tail(word_t fifo)
        {
            assert(fifo < nr_inj_fifos);
            return (gpr_read(RCV_FIFO_REG(fifo, FIFO_TAIL)) << 4);
        } 
    
    void set_rcv_fifo_head(word_t fifo, paddr_t paddr)
        {
            assert(fifo < nr_inj_fifos);
            assert(!(paddr & 0xf));
            gpr_write(RCV_FIFO_REG(fifo, FIFO_HEAD), paddr >> 4);
        }



#ifndef NODEBUG
    void dump_rcv_channel(word_t ch=nr_rcv_fifos, word_t ctr=nr_counters);
    void dump_inj_channel(word_t ch=nr_inj_fifos, word_t ctr=nr_counters);
#endif
    
    ptorus_dma_t (ptorus_t *t, unsigned g, word_t phys, word_t virt) : torus_dma_t (g), torus (t), base (virt)
        {
            L4_Sigma0_GetPage (L4_nilthread, L4_Fpage (phys, BGP_SIZE_TORUS_DMA_GRP), L4_Fpage (virt, BGP_SIZE_TORUS_DMA_GRP));
        }

};

class ptorus_t : public torus_t
{
    coordinates_t   coordinates;
    coordinates_t   dimension;
    
    ptorus_dma_t    dma0;  // DMA Group 0
    ptorus_dma_t    dma1;  // DMA Group 1
    ptorus_dma_t    dma2;  // DMA Group 0
    ptorus_dma_t    dma3;  // DMA Group 1

    word_t pending_irqs[3];
    
    class vmchan_t : public torus_dmareg_t
    {
        friend class ptorus_t;
    private:
        // The channel's nic client 
        vdp83820_t   *vnic;
        
        // The channel's dma 
        ptorus_t     * const     torus;
        ptorus_dma_t * const     dma;
        
        static const word_t conn_id         = 0;
        static const word_t inj_counter_no  = torus_dma_t::nr_counters-1;

        // Used torus fifos
        static const word_t pid            = nr_pids-1;
        static const word_t fifomap       = 0x01;

        // INJ/RCV Memory Fifos
        static const word_t inj_fifo_no   = nr_inj_fifos-1;
        static const word_t rcv_fifo_no   = nr_rcv_fifos-2;
        static const word_t inj_fifo_size = 256;
        static const word_t rcv_fifo_size = 512;
        
        inj_desc_t inj_fifo[inj_fifo_size] ALIGN(32);
        rcv_desc_t rcv_fifo[rcv_fifo_size] ALIGN(32);
        word_t     inj_fifo_add, inj_fifo_rem;
        word_t     rcv_fifo_add, rcv_fifo_rem;

        unsigned inj_fifo_cnt() const { return inj_fifo_add - inj_fifo_rem; }
     
        // Callback Descriptor table
        vdp83820_t::desc_t     *inj_descs[inj_fifo_size];
        bool     enabled;

    public:
        static const word_t group           = nr_dma_groups-1;
        
        bool is_enabled() { return enabled; }
        void enable(resource_t &);
        void register_vdp83820(vdp83820_t *v) { assert(vnic == NULL || vnic == v); vnic = v; }
        
        bool insert_injdesc(paddr_t, size_t, coordinates_t, dma_sw_src_key_t, vdp83820_t::desc_t*);

        bool handle_inj_ctr_zero(word_t &);
        bool handle_rcv_thr_zero(word_t &);
        
        vmchan_t(ptorus_t *t, ptorus_dma_t *d) : vnic(0),
                                                 torus(t), 
                                                 dma(d), 
                                                 inj_fifo_add(0), inj_fifo_rem(0),
                                                 rcv_fifo_add(0), rcv_fifo_rem(0),
                                                 enabled (false) 
            { for (word_t i=0; i < inj_fifo_size; i++) inj_descs[i] = NULL; }
    };
    
    void set_pending(const word_t irq)
        {
            assert(is_irq(irq));
            pending_irqs[(irq - irq_min) >> 5] |= (31 - (irq - irq_min)) & 31;
        }
    void clear_pending(const word_t irq)
        {
            assert(is_irq(irq));
            pending_irqs[(irq - irq_min) >> 5] &= ~(31 - (irq - irq_min)) & 31;
        }
    bool is_pending(const word_t irq)
        {
            assert(is_irq(irq));
            return (pending_irqs[(irq - irq_min) >> 5] & ((31 - (irq - irq_min)) & 31));
        }

  
    void set_dma_rcv_region(word_t r, paddr_t min, paddr_t max)
        { 
            dmadcr_write(DmaDcr(rDMA_MIN_VALID_ADDR0+(2*r)), min >> 4); 
            dmadcr_write(DmaDcr(rDMA_MAX_VALID_ADDR0+(2*r)), max >> 4); 
        }
    
    void set_dma_inj_region(word_t r, paddr_t min, paddr_t max)
        { 
            dmadcr_write(DmaDcr(iDMA_MIN_VALID_ADDR0+(2*r)), min >> 4); 
            dmadcr_write(DmaDcr(iDMA_MAX_VALID_ADDR0+(2*r)), max >> 4); 
        }

    void set_inj_fifo_map(word_t group, word_t fifo, u8_t fifomap)
        {
            
            u32_t val = dmadcr_read(DmaDcr(iDMA_TS_INJ_FIFO_MAP0 + group * 8 + (fifo / 4)));
            val &= ~(0xff000000 >> (8 * (fifo % 4)));
            val |= (u32_t)fifomap << (8 * (3 - (fifo % 4)));
            dmadcr_write(DmaDcr(iDMA_TS_INJ_FIFO_MAP0 + group * 8 + (fifo / 4)), val);
        }

    void set_inj_fifo_prio(word_t group, word_t fifo,
                           word_t prio)
        {
            DmaDcr dcr = DmaDcr(iDMA_FIFO_PRIORITY0 + group);
            u32_t val = dmadcr_read(dcr);
            val &= ~(0x80000000 >> (fifo % 32));
            val |= ((prio << 31) >> (fifo % 32));
            dmadcr_write(dcr, val);
        }

    void set_inj_fifo_local_dma(word_t group, word_t fifo,
                                word_t local_dma)
        {
            u32_t val = dmadcr_read(DmaDcr(iDMA_LOCAL_COPY0 + group));
            val &= ~(0x80000000 >> (fifo % 32));
            val |= ((local_dma << 31) >> (fifo % 32));
            dmadcr_write(DmaDcr(iDMA_LOCAL_COPY0 + group), val);
        }

    void config_inj_fifo(word_t group, word_t fifo,
                         u8_t fifomap, word_t prio, word_t local_dma)
        {
            set_inj_fifo_map(group, fifo, fifomap);
            set_inj_fifo_prio(group, fifo, prio);
            set_inj_fifo_local_dma( group, fifo, local_dma);
        }

    void reset_inj_fifo(word_t group, word_t fifo)
        {
            config_inj_fifo(group, fifo, default_fifomap, 0, 0);
        }

    void set_rcv_fifo_threshold(word_t threshold, word_t type=0)
        {
            dmadcr_write(DmaDcr(rDMA_FIFO_THRESH0 + type), threshold);
        }
    
    /* mapping from pid -> (group/rcv fifo) */
    void set_rcv_fifo_map(word_t pid, word_t group, word_t fifoid)
        {
            u8_t fifomap = (group << 3) + (fifoid & 0x7);

            dmadcr_write(DmaDcr(rDMA_TS_REC_FIFO_MAP_G0_PID00_XY + (pid * 2)),
                         ((fifomap & 0xff) << 24) |
                         ((fifomap & 0xff) << 16) |
                         ((fifomap & 0xff) <<  8) |
                         ((fifomap & 0xff) <<  0));
            
            dmadcr_write(DmaDcr(rDMA_TS_REC_FIFO_MAP_G0_PID00_ZHL + (pid * 2)),
                         ((fifomap & 0xff) << 24) |
                         ((fifomap & 0xff) << 16) |
                         ((fifomap & 0xff) <<  8) |
                         ((fifomap & 0xff) <<  0));
        }

    
    void enable_inj_fifo(word_t group, word_t fifo)
        {
            assert(fifo < torus_dma_t::nr_inj_fifos);
            DmaDcr dcr = DmaDcr(iDMA_FIFO_ENABLE0 + group);
            dmadcr_write(dcr, dmadcr_read(dcr) | (0x80000000 >> fifo));
        }
    
    void enable_rcv_fifo(word_t group, word_t fifo)
        {
            // Can only enable non-header RCV FIFOs
            assert(fifo < (torus_dma_t::nr_rcv_fifos-1));
            DmaDcr dcr = DmaDcr(rDMA_FIFO_ENABLE);
            dmadcr_write(dcr, dmadcr_read(dcr) | (0x80000000 >> (group * (torus_dma_t::nr_rcv_fifos-1) + fifo)));
        }
    
    void dma_reset();
    
public:
    ptorus_dma_t * const dma;
    vmchan_t vmchan;
    static ptorus_t torus;
    
    u8_t get_coordinate(const Axes Ax) { return coordinates[Ax]; }

    void set_kvmm_dma_inj_region(paddr_t min, paddr_t max)
        { set_dma_inj_region(7, min, max); }
    void set_kvmm_dma_rcv_region(paddr_t min, paddr_t max)
        { set_dma_rcv_region(7, min, max); }

    word_t dcr_read (Dcr reg)
        {
            return mfdcrx( DCRBASE + reg);
        }
    
    void dcr_write (Dcr reg, word_t val)
        {
            mtdcrx( DCRBASE + reg, val);
        }

    word_t dmadcr_read (DmaDcr reg)
        {
            return mfdcrx( DCRBASE_DMA + reg );
        }
    
    void dmadcr_write (DmaDcr reg, word_t val)
        {
            mtdcrx( DCRBASE_DMA + reg, val);
        }

   
    bool handle_hwirq(const word_t, word_t &);
    void ack_hwirq(word_t);
    
    ptorus_t (word_t addr) : dma0 (this, 0, BGP_SIGMA0_TORUS_DMA + 0 * BGP_SIZE_TORUS_DMA_GRP, addr + 0 * BGP_SIZE_TORUS_DMA_GRP),
                             dma1 (this, 1, BGP_SIGMA0_TORUS_DMA + 1 * BGP_SIZE_TORUS_DMA_GRP, addr + 1 * BGP_SIZE_TORUS_DMA_GRP),
                             dma2 (this, 2, BGP_SIGMA0_TORUS_DMA + 2 * BGP_SIZE_TORUS_DMA_GRP, addr + 2 * BGP_SIZE_TORUS_DMA_GRP),
                             dma3 (this, 3, BGP_SIGMA0_TORUS_DMA + 3 * BGP_SIZE_TORUS_DMA_GRP, addr + 3 * BGP_SIZE_TORUS_DMA_GRP),
                             dma(&dma0), 
                             vmchan(this, &dma[vmchan.group]) {} 
#ifndef NODEBUG
    void dump_dcrs();
#endif
    

    void init(L4_ThreadId_t);

};
