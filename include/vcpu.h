/*********************************************************************
 *
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vcpu.h
 * Description:     Virtual CPU
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "powerpc.h"

class vm_t;
class vmthread_t;

class vcpu_t
{
    private:
    // See: arch/include/bpcore/bgp_dcrmap.h
    enum Dcr
    {
        // Serdes Config (0x200-0x3ff)
        DCR_SERDES_MIN              = 0x200,
        DCR_SERDES_MAX              = DCR_SERDES_MIN + 0x1ff,

        // Test Interface (0x400-0x47f)
        DCR_TEST_MIN                = 0x400,
        DCR_TEST_MAX                = DCR_TEST_MIN + 0x7f,

        // Netbus Arbiter (0x630-0x63f)
        DCR_NETBUS_MIN              = 0x630,
        DCR_NETBUS_MAX              = DCR_NETBUS_MIN + 0xf,
        
        /* Global interrupt (0x660-0x66f)  */
        DCR_GLOBAL_INT_MIN          = 0x660,
        DCR_GLOBAL_INT_MAX          = 0x660 + 0xf,

        /* BGP Tree */
        DCR_TREE_MIN				= 0xc00,
        DCR_TREE_MAX				= 0xc7f
        
    };

    enum Spr
    {
        SPR_PVR     = 0x11f,    // Processor Version Register
        SPR_DBSR    = 0x130,    // Debug Status Register
    };

    word_t  spr_dbsr;

    static inline word_t *gpr (L4_GPRegsCtrlXferItem_t *gpregs, unsigned n)
        {
            return (gpregs + n / 16)->regs.reg + n % 16;
        }

    word_t  emulate_mfdcr       (Dcr, word_t *);
    void    emulate_mtdcr       (Dcr, word_t);
    void    emulate_mfspr       (Spr, word_t *);
    void    emulate_mtspr       (Spr, word_t);
    word_t  emulate_device_fpr  (paddr_t, unsigned, bool);
    word_t  emulate_device_gpr  (paddr_t, word_t *, bool);

    void emulate                (L4_MsgTag_t, L4_Msg_t &, L4_Msg_t &, word_t, ppc_instr_t, word_t, paddr_t);
    void handle_hvm_tlb         (L4_MsgTag_t, L4_Msg_t &, L4_Msg_t &);
    void handle_hvm_program     (L4_MsgTag_t, L4_Msg_t &, L4_Msg_t &);

public:
    L4_ThreadId_t tid;
    unsigned	  vcpu;
    unsigned	  pcpu;
    vm_t *        vm;
		
    bool init(unsigned vcpu, unsigned pcpu, vm_t *vm, L4_ThreadId_t space);
    bool set_reset_state();
    bool launch_vcpu(word_t ip);
    bool dispatch_message(L4_MsgTag_t tag);
    vmthread_t *get_vmthread();
    word_t  irq_status();
		
    void set_event_inject();
    void set_event_inject_remote(word_t core_mask);

    void set_tlb_entry (int idx, word_t vaddr, paddr_t paddr,
                        int log2size, word_t attribs, int pid,
                        bool valid = true);
};
