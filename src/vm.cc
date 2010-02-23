/*********************************************************************
 *
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vm.cc
 * Description:     Virtual Machine
 *
 * All rights reserved
 *
 ********************************************************************/

#include "config.h"
#include "elf.h"
#include "personality.h"
#include "std.h"
#include "vm.h"
#include "vmthread.h"
#include "ptree.h"
#include "ptorus.h"

word_t vm_t::ack_irq(word_t irq)
{
    if (irq < 32) 
        return 0;
    if (torus.is_irq(irq)) 
       return torus.ack_irq(irq);
    if (tree.is_irq(irq)) 
        return tree.ack_irq(irq);
    if (dp83820.is_irq(irq)) 
        return dp83820.ack_irq(irq);
            
    printf("unhandled hwirq ack %d\n", (int) irq);
    L4_KDB_Enter("BUG");
    
    return 0;
}

void vm_t::init (unsigned v, unsigned p, size_t memsize)
{
    if (v > MAX_VCPU)
        v = MAX_VCPU;

    if (p > MAX_PCPU)
        p = MAX_PCPU;

    printf ("Initializing VM (%u vCPUs, %u pCPUs, %uMB RAM)\n", v, p, memsize / (1024*1024));

    num_vcpu = v;

    void *kip = L4_KernelInterface();
    kip_area = L4_FpageLog2 ((L4_Word_t) kip, L4_KipAreaSizeLog2 (kip));

    word_t utcb_base = L4_MyLocalId().raw;
    utcb_base &= ~(L4_UtcbAreaSize (kip) - 1);
    size_t size = 1 << L4_UtcbAreaSizeLog2(kip);
    //printf("area size: %d, sz: %d\n", size, L4_UtcbSize(kip) * 2 * numcpu);

    if (size < L4_UtcbSize(kip) * 2 * num_vcpu)
        size = L4_UtcbSize(kip) * 2 * num_vcpu;

    utcb_area = L4_Fpage (utcb_base, size);

    ram = resource_t (RAM_REAL, RAM_GPA, RAM_SIZE, RAM_MAP);
    printf ("Resource RAM:  GPA:%08lx MAP:%08lx S:%lx\n",
            RAM_GPA, RAM_MAP, RAM_SIZE);

    sram = resource_t (SRAM_REAL, SRAM_GPA, SRAM_SIZE, SRAM_MAP);
    printf ("Resource SRAM: GPA:%08lx MAP:%08lx S:%lx\n",
            SRAM_GPA, SRAM_MAP, SRAM_SIZE);

    // load the first module
    elf_hdr_t::load_file (0x1700000, ram.map_base, sram.map_base + sram.size);

    // set up personality page
    bgp = new (SRAM_MAP + SRAM_PERS_TABLE_OFFS) bgp_personality_t (CPU_FREQ, RAM_SIZE);

    // bring up virtual CPUs
    for (unsigned i = 0; i < num_vcpu; i++)
        vcpu[i].init (i, i % p, this, i == 0 ? L4_nilthread : vcpu[0].tid);
}

vcpu_t *vm_t::get_vcpu (L4_ThreadId_t tid)
{
    for (unsigned i = 0; i < num_vcpu; i++)
        if (vcpu[i].tid == tid)
            return vcpu + i;

    return 0;
}
