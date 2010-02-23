/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vbic.h
 * Description:     Virtual BlueGene Interrupt Controller
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"
#include "sync.h"
#include "lock_guard.h"

class vm_t;
/*
 * Target Selector
 */
class vbic_tsel_t
{
    private:
        unsigned    val;

    public:
        enum Type
        {
            BCAST,
            NCRIT,
            CRIT,
            MCHK
        };

        explicit vbic_tsel_t (unsigned v) : val (v) {}

        Type type() const       { return Type (val >> 2); }
        unsigned core() const   { return val & 3; }
};

/*
 * Interrupt Group
 */
class vbic_group_t
{
    friend class vbic_t;

private:
    const word_t grp;
    spinlock_t  lock;                       // Group lock
    vm_t        *vm;                        
    u32_t       status;
    u32_t       target[4];

    vbic_tsel_t tsel (const word_t girq) const
        {
            return vbic_tsel_t (target[girq / 8] >> (28 - 4 * (girq % 8)) & 0xf);
        }

public:
    enum Register
    {
        STATUS          = 0x0,              // rw
        STATUS_RD_CLR   = 0x4,              // ro
        STATUS_CLR      = 0x8,              // wo
        STATUS_SET      = 0xc,              // wo
        TARGET_MIN      = 0x10,             // rw
        TARGET_MAX      = 0x1c,
        NCRIT_MIN       = 0x20,             // ro
        NCRIT_MAX       = 0x2c,
        CRIT_MIN        = 0x30,             // ro
        CRIT_MAX        = 0x3c,
        MCHK_MIN        = 0x40,             // ro
        MCHK_MAX        = 0x4c
    };

    word_t pending (unsigned, vbic_tsel_t::Type) const;

    word_t set_irq     (const word_t);
    word_t set_status  (word_t);
    word_t clr_status  (word_t);

    word_t gpr_load     (Register, word_t *);
    word_t gpr_store    (Register, word_t *);

    vbic_group_t(word_t g) : grp (g) { }
};

/*
 * Interrupt Controller
 */
class vbic_t
{
private:
    static unsigned const group_count = 15;
    static unsigned const group_size  = 0x80;

    vbic_group_t group0; vbic_group_t group1;
    vbic_group_t group2; vbic_group_t group3;
    vbic_group_t group4; vbic_group_t group5;
    vbic_group_t group6; vbic_group_t group7;
    vbic_group_t group8; vbic_group_t group9;
    vbic_group_t groupA; vbic_group_t groupB;
    vbic_group_t groupC; vbic_group_t groupD;
    vbic_group_t groupE; vbic_group_t groupF;

    vbic_group_t * const group;

public:
    enum Register
    {
        // Group Registers
        GRP_MIN     = 0x0,
        GRP_MAX     = 0x77c,

        // Hierarchy Registers
        NCRIT_MIN   = 0x780,
        NCRIT_MAX   = 0x78c,
        CRIT_MIN    = 0x790,
        CRIT_MAX    = 0x79c,
        MCHK_MIN    = 0x7a0,
        MCHK_MAX    = 0x7ac
    };

    word_t set_irq_status (const word_t irq)
        {
        
            const word_t grp = irq >> 5;
            assert (grp < group_count);
            
            lock_guard_t<spinlock_t> guard (group[grp].lock);
            return group[grp].set_irq(irq);
            
        }

    vbic_t(vm_t *v) : group0(0x0), group1(0x1),
                      group2(0x2), group3(0x3),
                      group4(0x4), group5(0x5),
                      group6(0x6), group7(0x7),
                      group8(0x8), group9(0x9),
                      groupA(0xa), groupB(0xb),
                      groupC(0xc), groupD(0xd),
                      groupE(0xe), groupF(0xf),
                      group(&group0)
        { for (word_t g=0; g<group_count; g++) group[g].vm = v; }
    
    word_t pending (unsigned, vbic_tsel_t::Type);

    word_t gpr_load     (Register, word_t *);
    word_t gpr_store    (Register, word_t *);
};
