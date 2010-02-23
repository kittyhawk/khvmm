/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       vtree.cc
 * Description:     Virtual BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/

#include "fpu.h"
#include "lock_guard.h"
#include "ptree.h"
#include "vtree.h"
#include "vm.h"

/*
 * Receive a packet into the virtual tree device
 *
 * @param head  Packet header; packet payload is passed via the FPU
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_chan_t::recv_packet (word_t head)
{
    lock_guard_t<spinlock_t> guard (tree->lock);
    
    // Need space for at least one header and one payload to receive a packet
    if (rec_head_cnt() != fifo_head_size && rec_data_cnt() / 16 != fifo_data_size) {

        rec_fifo_head[rec_head_add % fifo_head_size] = head;

        for (unsigned i = 0; i < 16; i++)
            fpu_t::read (i, &rec_fifo_data[(rec_data_add + i) % fifo_data_size]);

        rec_head_add += 1;
        rec_data_add += 16;

        // Flag watermark exception
        if (rec_data_cnt() / 16 > rec_watermark)
            tree->prxf |= vtree_t::PRX_WM0 >> channel;

        return tree->raise_irqs();
    }

    //printf ("virtual RX FIFO full, packet dropped\n");

    return 0;
}

/*
 * Potentially send a packet from the virtual to the physical tree device
 * Caller must hold tree lock!
 */
void vtree_chan_t::send_packet()
{
    assert (tree->lock.is_locked());

    // Check for injection interface disable
    if (!((tree->pixen | tree->pixf) & vtree_t::PIX_ENABLE))
        return;

    // Need at least one header and one payload to send a packet
    if (inj_head_cnt() && inj_data_cnt() / 16) {

        // Test if the tree network is accessible
        lnk_hdr_t *lhdr = (lnk_hdr_t *) &inj_fifo_data[inj_data_rem % fifo_data_size];
        
        if (!tree->vm->comm_control.tree_pass(lhdr->dst_key))
        {
            printf("%s: tree access to network [%06ld] denied\n", __PRETTY_FUNCTION__, lhdr->dst_key);
            // Ignore packet
        }
        else 
        {
            // Use the physical channel that corresponds to the virtual channel
            ptree_t::tree.chan[channel].send_packet (inj_fifo_head[inj_head_rem % fifo_head_size],
                                                     &inj_fifo_data[inj_data_rem % fifo_data_size]);
        }
        inj_head_rem += 1;
        inj_data_rem += 16;

        // Flag watermark exception
        if (inj_data_cnt() / 16 <= inj_watermark)
            tree->pixf |= vtree_t::PIX_WM0 >> channel;
    }
}

/*
 * Emulate FPU register load from tree channel register
 *
 * @param reg   Tree channel register
 * @param fpr   Floating point register number
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_chan_t::fpr_load (Register reg, unsigned fpr)
{
    word_t core_mask = 0;

    lock_guard_t<spinlock_t> guard (tree->lock);

    switch (reg) {

        case PRD:
            if (!rec_data_cnt())
                tree->prxf |= vtree_t::PRX_PFU0 >> channel;
            else {
                fpu_t::write (fpr, &rec_fifo_data[rec_data_rem++ % fifo_data_size]);

                // Clear watermark exception
                if (rec_data_cnt() / 16 <= rec_watermark)
                    tree->prxf &= ~(vtree_t::PRX_WM0 >> channel);
            }	

            core_mask = tree->raise_irqs();
            break;

        default:
            printf ("%s: reg=%#lx -> fpr=%u\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), fpr);
    }

    return core_mask;
}

/*
 * Emulate FPU register store to tree channel register
 *
 * @param reg   Tree channel register
 * @param fpr   Floating point register number
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_chan_t::fpr_store (Register reg, unsigned fpr)
{
    word_t core_mask = 0;

    lock_guard_t<spinlock_t> guard (tree->lock);

    switch (reg) {

        case PID:
            if (inj_data_cnt() == fifo_data_size)
                tree->pixf |= vtree_t::PIX_PFO0 >> channel;
            else {
                fpu_t::read (fpr, &inj_fifo_data[inj_data_add++ % fifo_data_size]);

                // Clear watermark exception
                if (inj_data_cnt() / 16 > inj_watermark)
                    tree->pixf &= ~(vtree_t::PIX_WM0 >> channel);

                send_packet();
            }

            core_mask = tree->raise_irqs();
            break;

        default:
            printf ("%s: reg=%#lx <- fpr=%u\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), fpr);
    }

    return core_mask;
}

/*
 * Emulate GP register load from tree channel register
 *
 * @param reg   Tree channel register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_chan_t::gpr_load (Register reg, word_t *gpr)
{
    word_t core_mask = 0;

    lock_guard_t<spinlock_t> guard (tree->lock);

    switch (reg) {

        case PRH:
            if (rec_head_cnt())
                *gpr = rec_fifo_head[rec_head_rem++ % fifo_head_size];
            else
                tree->prxf |= vtree_t::PRX_HFU0 >> channel;

            core_mask = tree->raise_irqs();
            break;

        case PSR:
            *gpr = status();
            break;

        default:
            printf ("%s: reg=%#lx -> val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    }

    return core_mask;
}

/*
 * Emulate GP register store to tree channel register
 *
 * @param reg   Tree channel register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_chan_t::gpr_store (Register reg, word_t *gpr)
{
    word_t core_mask = 0;

    lock_guard_t<spinlock_t> guard (tree->lock);

    switch (reg) {

        case PIH:
            if (inj_head_cnt() == fifo_head_size)
                tree->pixf |= vtree_t::PIX_HFO0 >> channel;
            else {
                inj_fifo_head[inj_head_add++ % fifo_head_size] = *gpr;
                send_packet();
            }

            core_mask = tree->raise_irqs();
            break;

        default:
            printf ("%s: reg=%#lx <- val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    }

    return core_mask;
}

/*
 * Emulate read from tree DCR
 *
 * @param dcr   Device control register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_t::dcr_read (Dcr dcr, word_t *gpr)
{
    word_t core_mask = 0;

    lock_guard_t<spinlock_t> guard (lock);

    // Seems like all the registers are readable
    switch (dcr) {
        case RDR_MIN ... RDR_MAX: *gpr = rdr[dcr - RDR_MIN];                break;
        case ISR_MIN ... ISR_MAX: *gpr = isr[dcr - ISR_MIN];                break;
        case RCFG:      *gpr = rcfg;                                        break;
        case RTO:       *gpr = rto;                                         break;
        case FPTR:      *gpr = fptr;                                        break;
        case NADDR:     *gpr = naddr;                                       break;
        case VCFG0:     *gpr = chan0.vcfg;                                  break;
        case VCFG1:     *gpr = chan1.vcfg;                                  break;
        case PRXEN:     *gpr = prxen;                                       break;
        case PRDA:      *gpr = prda;                                        break;
        case PRDD:      *gpr = prdd;                                        break;
        case PIXEN:     *gpr = pixen;                                       break;
        case PIDA:      *gpr = pida;                                        break;
        case PIDD:      *gpr = pidd;                                        break;
        case CSPY0:     *gpr = chan0.cspy;                                  break;
        case CSHD0:     *gpr = chan0.cshd;                                  break;
        case CSPY1:     *gpr = chan1.cspy;                                  break;
        case CSHD1:     *gpr = chan1.cshd;                                  break;

        // Reading exception flags clears the bits. We keep the watermark
        // bits because the hardware would immediately set them again.
        case PRXF:
            *gpr = prxf;
            prxf &= (PRX_WM0 | PRX_WM1);
            core_mask = raise_irqs();
            break;

        case PIXF:
            *gpr = pixf;
            pixf &= (PIX_WM0 | PIX_WM1 | PIX_ENABLE);
            core_mask = raise_irqs();
            break;

        default:
            printf ("%s: dcr=%#lx -> val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(dcr), *gpr);
    }

    return core_mask;
}

/*
 * Emulate write to tree DCR
 *
 * @param dcr   Device control register
 * @param val   Value to be written to DCR
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_t::dcr_write (Dcr dcr, word_t val)
{
    lock_guard_t<spinlock_t> guard (lock);

    switch (dcr) {
        case RDR_MIN ... RDR_MAX:   rdr[dcr - RDR_MIN]  = val;  break;
        case ISR_MIN ... ISR_MAX:   isr[dcr - ISR_MIN]  = val;  break;
        case RCFG:                  rcfg                = val;  break;
        case RTO:                   rto                 = val;  break;
        case FPTR:                  /* read-only */             break;
        case NADDR:                 naddr               = val;  break;
        case VCFG0:                 chan0.vcfg          = val;  break;
        case VCFG1:                 chan1.vcfg          = val;  break;
        case PRXF:                  prxf               |= val;  break;
        case PRXEN:                 prxen               = val;  break;
        case PRDA:                  prda                = val;  break;
        case PRDD:                  prdd                = val;  break;
        case PIXF:                  pixf               |= val;  break;
        case PIXEN:                 pixen               = val;  break;
        case PIDA:                  pida                = val;  break;
        case PIDD:                  pidd                = val;  break;
        case CSPY0:                 chan0.cspy          = val;  break;
        case CSHD0:                 chan0.cshd          = val;  break;
        case CSPY1:                 chan1.cspy          = val;  break;
        case CSHD1:                 chan1.cshd          = val;  break;

        default:
            printf ("%s: dcr=%#lx <- val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(dcr), val);
    }

    return 0;
}

/*
 * Update tree exception status at interrupt controller
 * Caller must hold tree lock!
 * 
 * @return      Bitmask of cores whose exception status was affected  
 */
word_t vtree_t::raise_irqs()
{
    assert (lock.is_locked());
        
    word_t core_mask = 0;

    if (prxf & prxen & PRXEN_MASK_CRITICAL)
        core_mask |= vm->bic.set_irq_status (ptree_t::tree.get_rcv_irq());

    if (pixf & pixen & PIXEN_MASK_CRITICAL)
        core_mask |= vm->bic.set_irq_status (ptree_t::tree.get_inj_irq());

    if ((prxf & PRXEN_MASK_NONCRITICAL) || (pixf & PIXEN_MASK_NONCRITICAL)) {
        pixf &= ~PIX_ENABLE;
        unimplemented();
    }

    return core_mask;
}
