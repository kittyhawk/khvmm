/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vbic.cc
 * Description:     Virtual BlueGene Interrupt Controller
 *
 * All rights reserved
 *
 ********************************************************************/

#include "lock_guard.h"
#include "vm.h"
#include "vbic.h"

/*
 * Make interrupt pending in a particular group
 * Caller must hold group lock!
 *
 * @param irq   Bit number of interrupt to make pending
 * @return      Bitmask of cores whose exception status was affected
 */

word_t vbic_group_t::set_irq (const word_t irq)
{
    assert (lock.is_locked());
    assert ((irq >> 5) == grp);
    
    word_t core_mask = 0;
    word_t girq = irq & 31;
    vbic_tsel_t ts = tsel (girq);
    
    status |= 1ul << (31 - girq);

    if (ts.type() == vbic_tsel_t::BCAST && ts.core())
        core_mask |= 0xf;
    else
        core_mask |= 1ul << ts.core();

    return core_mask;
}

/*
 * Make interrupts pending in a particular group
 * Caller must hold group lock!
 *
 * @param set   Bitmask of interrupts to make pending
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_group_t::set_status (word_t set)
{
    assert (lock.is_locked());

    word_t core_mask = 0;

    status |= set;
    
    // Iterate over all IRQs in the group to see which cores are affected
    while (set)
    {
        word_t girq = lzw(set);
        vbic_tsel_t ts = tsel (girq);

        set &= ~(1ul << (31 - girq));

        if (ts.type() == vbic_tsel_t::BCAST && ts.core())
            core_mask |= 0xf;
        else
            core_mask |= 1ul << ts.core();
    }

    return core_mask;
}

/*
 * Clear interrupts pending in a particular group
 * Caller must hold group lock!
 *
 * @param set   Bitmask of interrupts to clear pending
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_group_t::clr_status (word_t set)
{
    word_t core_mask = 0;
    assert (lock.is_locked());
  
    status &= ~set;
    
    // Callback virtual devices to recompute status 
    lock.unlock();
    
    while (set)
    {
        word_t girq = lzw(set);
        set &= ~(1ul << (31 - girq));
        core_mask |= vm->ack_irq((grp << 5) + girq);
    }        
    lock.lock();
    
    return core_mask;
}


/*
 * Compute pending interrupts for particular type/core within a group
 * Caller must hold group lock!
 *
 * @param core  Core number for which pending interrupts are checked
 * @param type  Interrupt type (non-critical, critical, machine-check)
 * @return      Bitmask of pending interrupts within the group
 */
word_t vbic_group_t::pending (unsigned core, vbic_tsel_t::Type type) const
{
    assert (lock.is_locked());

    word_t val = 0;
    word_t set = status;
    
    while (set)
    {
        word_t girq = lzw(set);
        vbic_tsel_t ts = tsel (girq);
        
        set &= ~(1ul << (31 - girq));

        if ((ts.type() == vbic_tsel_t::BCAST && vbic_tsel_t::Type (ts.core()) == type) ||
            (ts.type() == type && ts.core() == core))
            val |= (1ul << (31 - girq));

    }

    return val;
}

/*
 * Emulate GP register load from group register
 *
 * @param reg   Group register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_group_t::gpr_load (Register reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (lock);

    switch (reg) {

        case STATUS:
            *gpr = status;
            break;

        case STATUS_RD_CLR:
            *gpr = status; 
            return clr_status(~0ul);

        case TARGET_MIN ... TARGET_MAX:
            *gpr = target[(reg - TARGET_MIN) / sizeof *gpr];
            break;

        case NCRIT_MIN ... NCRIT_MAX:
            *gpr = pending ((reg - NCRIT_MIN) / sizeof *gpr, vbic_tsel_t::NCRIT);
            break;

        case CRIT_MIN ... CRIT_MAX:
            *gpr = pending ((reg - CRIT_MIN) / sizeof *gpr, vbic_tsel_t::CRIT);
            break;

        case MCHK_MIN ... MCHK_MAX:
            *gpr = pending ((reg - MCHK_MIN) / sizeof *gpr, vbic_tsel_t::MCHK);
            break;

        case STATUS_CLR:                /* write-only */
        case STATUS_SET:                /* write-only */
        default:
            printf ("%s: reg=%#lx val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    }

    return 0;
}

/*
 * Emulate GP register store to group register
 *
 * @param reg   Group register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_group_t::gpr_store (Register reg, word_t *gpr)
{
    lock_guard_t<spinlock_t> guard (lock);

    switch (reg) {

    case STATUS:
        status = 0;
        /* fall through */
    case STATUS_SET:
        return set_status (*gpr);

    case STATUS_CLR:
        return clr_status(*gpr);

    case TARGET_MIN ... TARGET_MAX:
        target[(reg - TARGET_MIN) / sizeof *target] = *gpr;
        break;

    case STATUS_RD_CLR:             /* read-only */
    case NCRIT_MIN ... MCHK_MAX:    /* read-only */
    default:
        printf ("%s: reg=%#lx val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    }

    return 0;
}

/*
 * Compute hierarchy register for particular type/core
 *
 * @param core  Core number for which pending groups are checked
 * @param type  Interrupt type (non-critical, critical, machine-check)
 * @return      Bitmask of interrupt groups with pending interrupts
 */
word_t vbic_t::pending (unsigned core, vbic_tsel_t::Type type) 
{
    word_t val = 0;

    for (unsigned grp = 0; grp < group_count; grp++)
    {
        lock_guard_t<spinlock_t> guard (group[grp].lock);
        if (group[grp].pending (core, type))
            val |= 1ul << 31 - grp;
    }

    return val;
}

/*
 * Emulate GP register load from interrupt controller register
 *
 * @param reg   Interrupt controller register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_t::gpr_load (Register reg, word_t *gpr)
{
    switch (reg) {

        case GRP_MIN ... GRP_MAX:
            return group[reg / group_size].gpr_load (vbic_group_t::Register (reg % group_size), gpr);

        case NCRIT_MIN ... NCRIT_MAX:
            *gpr = pending ((reg - NCRIT_MIN) / sizeof *gpr, vbic_tsel_t::NCRIT);
            break;

        case CRIT_MIN ... CRIT_MAX:
            *gpr = pending ((reg - CRIT_MIN) / sizeof *gpr, vbic_tsel_t::CRIT);
            break;

        case MCHK_MIN ... MCHK_MAX:
            *gpr = pending ((reg - MCHK_MIN) / sizeof *gpr, vbic_tsel_t::MCHK);
            break;

        default:
            printf ("%s: reg=%lx gpr=%p\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), gpr);
    }

    return 0;
}

/*
 * Emulate GP register store to interrupt controller register
 *
 * @param reg   Interrupt controller register
 * @param gpr   Pointer to general purpose register in register file
 * @return      Bitmask of cores whose exception status was affected
 */
word_t vbic_t::gpr_store (Register reg, word_t *gpr)
{
    switch (reg) {

        case GRP_MIN ... GRP_MAX:
            return group[reg / group_size].gpr_store (vbic_group_t::Register (reg % group_size), gpr);

        default:
            printf ("%s: reg=%lx val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    }
    
    
    return 0;
}
