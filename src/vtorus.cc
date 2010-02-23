/*********************************************************************
 *
 * Copyright (C) 2008, Jan Stoess, Udo Steinberg, IBM Corporation
 *
 * File path:       vtorus.cc
 * Description:     Virtual BlueGene DMA
 *
 * All rights reserved
 *
 ********************************************************************/
#include "config.h"
#include "vm.h"
#include "vtorus.h"
#include "ptorus.h"

#define Tdprintf(x...)     

word_t vtorus_dma_t::gpr_load (Register reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (torus->lock);
    
    switch(reg)
    {
    case INJ_FIFO0 ... INJ_FIFO0+INJ_FIFO_SIZE-1:
        switch (reg & 0xf)
        {
        case FIFO_HEAD:
            // Inj FIFO head is volatile
            inj_fifo_reg[INJ_FIFO_REG(reg)] = 
                ptorus_t::torus.dma[group].gpr_read(reg)
                - ((word_t) &fifo_area.fifo[INJ_FIFO_IDX(reg)] >> 4)
                + ((word_t) inj_fifo[INJ_FIFO_IDX(reg)].start);
            break;
        default:
            break;
        }
        *gpr = inj_fifo_reg[INJ_FIFO_REG(reg)];
        break;
    case RCV_FIFO0 ... RCV_FIFO0+RCV_FIFO_SIZE-1:
        switch (reg & 0xf)
        {
        case FIFO_TAIL:
            // Rcv FIFO tail is volatile
            rcv_fifo_reg[RCV_FIFO_REG(reg)] = torus->hpreg_to_gpreg(ptorus_t::torus.dma[group].gpr_read(reg));
            break;
        default:
            break;
        }
        *gpr = rcv_fifo_reg[RCV_FIFO_REG(reg)];
        break;
    case INJ_COUNTER0 ... INJ_COUNTER0+INJ_COUNTER_SIZE-1:
        switch (reg & 0xf)
        {
        case CTR_BASE:
            *gpr = inj_counter[INJ_CTR_IDX(reg)].base;
            break;
        default: 
            *gpr = ptorus_t::torus.dma[group].gpr_read(reg);
            break;
        }
        break;
    case RCV_COUNTER0 ... RCV_COUNTER0+RCV_COUNTER_SIZE-1:
        switch (reg & 0xf)
        {
        case CTR_BASE:
            *gpr = rcv_counter[RCV_CTR_IDX(reg)].base;
            break;
        case CTR_LIMIT:
            *gpr = rcv_counter[RCV_CTR_IDX(reg)].limit;
            break;
        default:
            *gpr = ptorus_t::torus.dma[group].gpr_read(reg);
            break;
        }
        break;
    default:
        *gpr = ptorus_t::torus.dma[group].gpr_read(reg);
        break;
    }

    Tdprintf ("%s: group %ld gpr=%#04x <- val=%#lx\n", __PRETTY_FUNCTION__, group,  reg, *gpr);
    return 0;
}

word_t vtorus_dma_t::gpr_store (Register reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (torus->lock);
    word_t val;

    switch(reg)
    {
    case INJ_FIFO0 ... INJ_FIFO0+INJ_FIFO_SIZE-1:
    {
        // We shadow the complete INJ FIFO 
    
        word_t old = inj_fifo_reg[INJ_FIFO_REG(reg)];
        inj_fifo_reg[INJ_FIFO_REG(reg)] = *gpr;

        // Descriptor area assumptions: 
        //   disjunct areas (jsXXX should check that)
        //   no more than nr_inj_entries
        word_t iidx = INJ_FIFO_IDX(reg);
        word_t iarea = (word_t) &fifo_area.fifo[iidx];
        fifo_t *ififo = &inj_fifo[iidx];

        Tdprintf("%s TORUS DMA group %ld inj FIFO %ld desc area reg %d %lx real %lx\n", __PRETTY_FUNCTION__, 
                 group, iidx, (reg & 0xf), *gpr, val);

        switch (reg & 0xf)
        { 
        case FIFO_START:
            val = iarea >> 4;
            break;
        case FIFO_END:
            assert((ififo->end - ififo->start) <= (sizeof(inj_fifo_area_t) / 16));
            val = (iarea >> 4) + (ififo->end - ififo->start);
            break;
        case FIFO_HEAD: 
            assert((ififo->head - ififo->start) <= (sizeof(inj_fifo_area_t) / 16));
            val = (iarea >> 4) + (ififo->head - ififo->start);
            break;
        case FIFO_TAIL:
        {
            assert((ififo->tail - ififo->start) <= (sizeof(inj_fifo_area_t) / 16));
            val = (iarea >> 4) + (ififo->tail - ififo->start);
            
            // Copy descriptors from old tail to new tail
            word_t src = torus->gpreg_to_hpreg(ififo->start, ~0ul);
            assert(src != ~0ul);
            
            old = old ? old : ififo->tail;
            
            for (word_t o=old-ififo->start; o != ififo->tail-ififo->start; o = (o+(sizeof(inj_desc_t)>>4))%((sizeof(inj_desc_t)*nr_inj_entries)>>4))
            {
                inj_desc_t *src_desc = (inj_desc_t *) ((src + o) << 4);
                inj_desc_t *dst_desc = (inj_desc_t *) (iarea + (o << 4));
               
                                    
                Tdprintf("\tcopy descs from %lx to %lx (ofs %lx old %lx)\n", (word_t) src_desc, (word_t) dst_desc, o, old);
                
                *dst_desc = *src_desc;

               if (!vm->comm_control.torus_desc_pass(src_desc))
                {
                    printf("%s: torus access to [%02d,%02d,%0d2] denied\n", __PRETTY_FUNCTION__,
                           src_desc->fifo.get_dest()[0], src_desc->fifo.get_dest()[1], src_desc->fifo.get_dest()[2]);
                    
                    // Revert value, fire torus error
                    inj_fifo_reg[INJ_FIFO_REG(reg)] = old;
                    return vm->bic.set_irq_status (torus_t::irq_fatal_err);
                }
                
                Tdprintf("\tinj [%08x, %08x, %08x, %08x] [%08x, %08x] [%08x, %08x]\n",
                         dst_desc->raw32[0], dst_desc->raw32[1], dst_desc->raw32[2], dst_desc->raw32[3],
                         dst_desc->raw32[4], dst_desc->raw32[5], dst_desc->raw32[6], dst_desc->raw32[7]);
                
                Tdprintf("\tdmahw [ctr %ld (g %02ld.%02ld) base %#lx ofs %#x -> payload %#lx len %#x]\n",
                         dst_desc->dma_hw.counter, dst_desc->dma_hw.counter >> 6, dst_desc->dma_hw.counter & 64, 
                         torus->dma[dst_desc->dma_hw.counter>>6].inj_counter[dst_desc->dma_hw.counter&63].base, 
                         dst_desc->dma_hw.base, 
                         torus->dma[dst_desc->dma_hw.counter>>6].inj_counter[dst_desc->dma_hw.counter&63].base+dst_desc->dma_hw.base, dst_desc->dma_hw.length);
                
                Tdprintf("\tphdr [dma %d pid %d,%d dest <%02d,%02d,%02d>]\n",
                         dst_desc->fifo.dma, dst_desc->fifo.pid0, dst_desc->fifo.pid1, 
                         dst_desc->fifo.dest_x, dst_desc->fifo.dest_y, dst_desc->fifo.dest_z);
     
            }
            break;
        }
        default: 
            val = 0;
            break;
            
        }
    }
    break;
    case RCV_FIFO0 ... RCV_FIFO0+RCV_FIFO_SIZE-1:
        rcv_fifo_reg[RCV_FIFO_REG(reg)] = *gpr;
        val = torus->gpreg_to_hpreg(*gpr, vm->ram.hpa_base);
        break;
    case INJ_COUNTER0 ... INJ_COUNTER0+INJ_COUNTER_SIZE-1:
        switch (reg & 0xf)
        {
        case CTR_BASE:
            inj_counter[INJ_CTR_IDX(reg)].base = *gpr;
            val = torus->gpreg_to_hpreg(*gpr, vm->ram.hpa_base);
            break;
        default: 
            val = *gpr;
            break;
        } 
        break;
    case RCV_COUNTER0 ... RCV_COUNTER0+RCV_COUNTER_SIZE-1:
        switch (reg & 0xf)
        {
        case CTR_BASE:
            rcv_counter[RCV_CTR_IDX(reg)].base = *gpr;
            val = torus->gpreg_to_hpreg(*gpr, vm->ram.hpa_base);
            break;
        case CTR_LIMIT:
            rcv_counter[RCV_CTR_IDX(reg)].limit = *gpr; 
            val = torus->gpreg_to_hpreg(*gpr, vm->ram.hpa_base + vm->ram.size -1);
            break;
        default:
            val = *gpr;
            break;
        }
        break;
    default:
        val = *gpr;
        break;
    }
 
    Tdprintf ("%s: group %ld gpr=%#04x <- val=%#lx (real %#lx)\n", __PRETTY_FUNCTION__, group, reg, *gpr, val);
    
    ptorus_t::torus.dma[group].gpr_write(reg, val);
    return 0;
}


bool vtorus_t::handle_hwirq(word_t irq, word_t &core_mask)
{
    core_mask |= vm->bic.set_irq_status (irq);
    return false;
}


/*
 * Translate DMA registers set to GPA to HPA  
 * 
 * @param gpa_reg   Input device control register containing GPA
 * @param hpa       Default HPA should translation fail
 * 
 * @return          Output device control register containing HPA
 */

INLINE word_t vtorus_t::gpreg_to_hpreg(word_t gpa_reg, paddr_t hpa) const
{
    word_t gpa = (paddr_t) gpa_reg << 4;
    if (!vm->ram.gpa_to_hpa(gpa, hpa))
    {
        // printf("TORUS: guest writes bogus GPA into reg, insert default\n");
    }
    return (hpa >> 4);
}


/*
 * Translate DMA registers set to HPA to GPA  
 * 
 * @param gpa_reg   Input device control register containing GPA
 * 
 * @return          Output device control register containing HPA
 */

INLINE word_t vtorus_t::hpreg_to_gpreg(word_t hpa_reg) const
{
    paddr_t hpa = (paddr_t) hpa_reg << 4;
    paddr_t gpa;
    bool mbt = vm->ram.hpa_to_gpa(hpa, gpa);
    assert(mbt);
    return (gpa >> 4);
}

void vtorus_t::dcr_read (Dcr reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (lock);
    *gpr = ptorus_t::torus.dcr_read(reg);
    Tdprintf ("%s: dcr=%#02x-> val=%#lx\n", __PRETTY_FUNCTION__, reg, *gpr);

}

void vtorus_t::dcr_write (Dcr reg, word_t val)
{
    lock_guard_t<spinlock_t> guard (lock);
    Tdprintf ("%s: dcr=%#02x <- val=%#lx\n", __PRETTY_FUNCTION__, reg, val);
    ptorus_t::torus.dcr_write(reg, val);
}



void vtorus_t::dmadcr_read (DmaDcr reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (lock);
    
    switch(reg)
    {
    case iDMA_MIN_VALID_ADDR0 ... iDMA_MAX_VALID_ADDR0 + 14:
    case rDMA_MIN_VALID_ADDR0 ... rDMA_MAX_VALID_ADDR0 + 14:
        // Shadow value
        break;
        
    default:  
        // Pass through value
        dmadcr.regs[reg] = ptorus_t::torus.dmadcr_read(reg);
        break;
    }

    *gpr = dmadcr.regs[reg];
    Tdprintf ("%s: dcr=%#02x -> val=%#lx\n", __PRETTY_FUNCTION__, reg, dmadcr.regs[reg]);

}

void vtorus_t::dmadcr_write (DmaDcr reg, word_t val)
{
    lock_guard_t<spinlock_t> guard (lock);
    dmadcr.regs[reg] = val;

    switch(reg)
    {
    case iDMA_MIN_VALID_ADDR0 ... iDMA_MAX_VALID_ADDR0 + 12:
    case rDMA_MIN_VALID_ADDR0 ... rDMA_MAX_VALID_ADDR0 + 15:
        val = gpreg_to_hpreg(dmadcr.regs[reg], 
                             ((reg & 0x1) ? vm->ram.hpa_base+vm->ram.size-1 : vm->ram.hpa_base));
        ptorus_t::torus.dmadcr_write(reg, val);
        
        if (!fifo_area_enabled &&(reg & 0x1 && dmadcr.regs[reg] > dmadcr.regs[reg-1]))
        {
            fifo_area_enabled = true;
            // Once the guest inserts a real value, make shadow injection descriptor area accessible
            ptorus_t::torus.set_kvmm_dma_inj_region((paddr_t) fifo_area, (paddr_t) fifo_area + sizeof(fifo_area));
        }
        break;
    case iDMA_MIN_VALID_ADDR0 + 14 ... iDMA_MAX_VALID_ADDR0 + 15:
        Tdprintf ("%s: dcr=%#02x <- val=%#lx ignored (reserved for KHVMM)", __PRETTY_FUNCTION__, reg, dmadcr.regs[reg]);
        break;
    default:  
        // Pass through value
        ptorus_t::torus.dmadcr_write(reg, val);
        break;
       
    }
    
    Tdprintf ("%s: dcr=%#02x <- val=%#lx (real %#lx)\n", __PRETTY_FUNCTION__, reg, dmadcr.regs[reg], val);
    
}

/*
 * Update torus  exception status at interrupt controller
 * @param torus_locked if lock is already held
 *
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtorus_t::ack_irq(word_t irq)
{
    ptorus_t::torus.ack_hwirq(irq);
    return 0;
}


