
/*********************************************************************
 *
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       vcpu.cc
 * Description:     Virtual CPU
 *
 * All rights reserved
 *
 ********************************************************************/

#include "config.h"
#include "std.h"
#include "vcpu.h"
#include "vm.h"
#include "vmthread.h"

bool vcpu_t::init (unsigned c, unsigned p, vm_t *v, L4_ThreadId_t space)
{
    int res;

    vcpu = c;
    pcpu = p;
    vm = v;
    
    tid = get_new_tid (vmthread_t::register_vcpu (this));
    
    L4_ThreadId_t disp = vmthread_t::get_tid (pcpu);

    if (L4_IsNilThread (space)) {       // new space
	
        res = L4_ThreadControl(tid, tid, disp, L4_nilthread, (void*)-1);
	
        if (res != 1) {
	        printf("ERROR: ThreadControl failed (ERR=%lu)\n", L4_ErrorCode());
	        return false;
	    }

	    L4_Word_t control;
	    res = L4_SpaceControl (tid, 1, vm->kip_area, vm->utcb_area,
		                       L4_nilthread, &control);
	    if (res != 1) {
	        printf ("ERROR: SpaceControl failed (ERR=%lu)\n", L4_ErrorCode());
	        return false;
	    }

	    space = tid;
    }

    L4_Word_t utcb = L4_Address(vm->utcb_area) + L4_UtcbSize (L4_KernelInterface()) * vcpu;

    // Activate thread
    res = L4_ThreadControl (tid, space, disp, disp, (void *) utcb);
    if (res != 1) {
        printf ("ERROR: ThreadControl failed (ERR=%lu)\n", L4_ErrorCode());
	    return false;
    }

    // Place thread on initial CPU
    L4_Set_ProcessorNo_Prio(tid, pcpu, PRIO_VCPU);

    // Configure exception messages
    L4_Msg_t ctrlxfer_msg;
    L4_Clear (&ctrlxfer_msg);

    L4_AppendFaultConfCtrlXferItems (&ctrlxfer_msg,
                                     L4_CTRLXFER_FAULT_ID_MASK (L4_FAULT_HVM_PROGRAM) |
                                     L4_CTRLXFER_FAULT_ID_MASK (L4_FAULT_HVM_TLB) |
                                     L4_CTRLXFER_FAULT_ID_MASK (L4_FAULT_PAGEFAULT),
                                     L4_CTRLXFER_FAULT_MASK (L4_CTRLXFER_GPREGS0) |
                                     L4_CTRLXFER_FAULT_MASK (L4_CTRLXFER_GPREGS1) |
                                     L4_CTRLXFER_FAULT_MASK (L4_CTRLXFER_GPREGSX) |
                                     L4_CTRLXFER_FAULT_MASK (L4_CTRLXFER_FPU),
                                     0);

    L4_Load (&ctrlxfer_msg);
    L4_ConfCtrlXferItems (tid);

    set_reset_state();

    return vmthread_t::vmthreads[pcpu].vcpu_bootstrap (this, 0xfffffffc);
}

/* pre-initialize the VCPU */
bool vcpu_t::set_reset_state()
{
   L4_Msg_t ctrlxfer_msg;

   L4_ConfigCtrlXferItem_t cfg;
   L4_Init(&cfg);
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_CCR0, 0);
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_CCR1, 0);
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_RSTCFG, 0); /* pin specific */
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_IVPR, 0);
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_PIR, vcpu);
   L4_Set(&cfg, L4_CTRLXFER_CONFIG_PVR, 0x52121880);

   /* XXX: ignore debug right now */

   L4_ExceptCtrlXferItem_t exc;
   L4_Init(&exc);
   L4_Set(&exc, L4_CTRLXFER_EXCEPT_MSR, 0);
   L4_Set(&exc, L4_CTRLXFER_EXCEPT_ESR, 0);
   L4_Set(&exc, L4_CTRLXFER_EXCEPT_MCSR, 0);

   L4_Clear(&ctrlxfer_msg);
   L4_Append(&ctrlxfer_msg, &cfg);
   L4_Append(&ctrlxfer_msg, &exc);

   L4_Load(&ctrlxfer_msg);
   L4_WriteCtrlXferItems(tid);

   /* Initialize Shadow TLB with reset vector mapping
    * TLB0: VAddr = 0xfffff000, 4K
    * TLB1: RPN = 0xfffff000, ERPN=SRAM
    * TLB2: Inhibited, Guarded, SX, SR
    */
   set_tlb_entry (-1,
                  SRAM_BOOT_VIRT,
                  vm->sram.gpa_base | 0xf000, /* allows for relocated SRAM */
                  12,
                  PPC_TLB_INHIBIT | PPC_TLB_GUARDED | PPC_TLB_SX | PPC_TLB_SR,
                  0);

   return true;
}

bool vcpu_t::launch_vcpu(word_t ip)
{
    L4_Msg_t msg;

    L4_Clear(&msg);
    L4_Append (&msg, ip);
    L4_Append (&msg, 0);
    L4_Load (&msg);
    L4_MsgTag_t tag = L4_Send (tid);
    return L4_IpcSucceeded(tag);
}

word_t vcpu_t::emulate_mfdcr (Dcr dcr, word_t *val)
{
    switch (dcr) {

    case DCR_SERDES_MIN ... DCR_SERDES_MAX:     // ignore for now
    case DCR_NETBUS_MIN ... DCR_NETBUS_MAX:
    case DCR_GLOBAL_INT_MIN ... DCR_GLOBAL_INT_MAX:
        *val = 0;
        return 0;

    case DCR_TEST_MIN ... DCR_TEST_MAX:
        vm->test.dcr_read (vtest_t::Dcr (dcr - DCR_TEST_MIN), val);
        return 0;
            
    case tree_t::DCRBASE_MIN ... tree_t::DCRBASE_MAX:
        return vm->tree.dcr_read (vtree_t::Dcr (dcr - tree_t::DCRBASE_MIN), val);

    case torus_t::DCRBASE_MIN ... torus_t::DCRBASE_MAX:
        vm->torus.dcr_read (vtorus_t::Dcr (dcr - torus_t::DCRBASE_MIN), val);
        return 0;

    case torus_t::DCRBASE_DMA_MIN ... torus_t::DCRBASE_DMA_MAX:
        vm->torus.dmadcr_read (vtorus_t::DmaDcr (dcr - torus_t::DCRBASE_DMA_MIN), val);
        return 0;
        
    default:
        printf ("%s: DCR:%#lx -> V:%#lx\n", __func__, static_cast<word_t>(dcr), *val);
        L4_KDB_Enter ("MFDCR: unknown DCR");
        return 0;
    }
}

void vcpu_t::emulate_mtdcr (Dcr dcr, word_t val)
{
    switch (dcr) {

    case DCR_SERDES_MIN ... DCR_SERDES_MAX:     // ignore for now
    case DCR_NETBUS_MIN ... DCR_NETBUS_MAX:
    case DCR_GLOBAL_INT_MIN ... DCR_GLOBAL_INT_MAX:
        return;

    case DCR_TEST_MIN ... DCR_TEST_MAX:
        vm->test.dcr_write (vtest_t::Dcr (dcr - DCR_TEST_MIN), val);
        return;

    case tree_t::DCRBASE_MIN ... tree_t::DCRBASE_MAX:
        vm->tree.dcr_write (vtree_t::Dcr (dcr - tree_t::DCRBASE_MIN), val);
        return;

    case torus_t::DCRBASE_MIN ... torus_t::DCRBASE_MAX:
        vm->torus.dcr_write (vtorus_t::Dcr (dcr - torus_t::DCRBASE_MIN), val);
        return;

    case torus_t::DCRBASE_DMA_MIN ... torus_t::DCRBASE_DMA_MAX:
        vm->torus.dmadcr_write (vtorus_t::DmaDcr (dcr - torus_t::DCRBASE_DMA_MIN), val);
        return;

    default:
        printf ("%s: DCR:%#lx <- V:%#lx\n", __func__, static_cast<word_t>(dcr), val);
        L4_KDB_Enter ("MTDCR: unknown DCR");
    }
}

void vcpu_t::emulate_mfspr (Spr spr, word_t *val)
{
    switch (spr) {

        case SPR_PVR:
            *val = 0x52121880;          // BlueGene
            break;

        default:
            printf ("%s: SPR:%#lx -> V:%#lx\n", __func__, static_cast<word_t>(spr), *val);
            L4_KDB_Enter ("MFSPR: unknown SPR");
    }
}

void vcpu_t::emulate_mtspr (Spr spr, word_t val)
{
    switch (spr) {

        case SPR_PVR:                   // read-only
            break;

        case SPR_DBSR:
            spr_dbsr = val;
            break;

        default:
            printf ("%s: SPR:%#lx <- V:%#lx\n", __func__, static_cast<word_t>(spr), val);
            L4_KDB_Enter ("MTSPR: unknown SPR");
    }
}

word_t  vcpu_t::emulate_device_fpr (paddr_t gpa, unsigned fpr, bool load)
{
    if (gpa >= BGP_PHYS_TREE0 && gpa < BGP_PHYS_TREE0 + BGP_SIZE_TREE0)
        return (vm->tree.chan0.*(load ? &vtree_chan_t::fpr_load : &vtree_chan_t::fpr_store))(vtree_chan_t::Register (gpa & BGP_SIZE_TREE0 - 1), fpr);

    else if (gpa >= BGP_PHYS_TREE1 && gpa < BGP_PHYS_TREE1 + BGP_SIZE_TREE1)
        return (vm->tree.chan1.*(load ? &vtree_chan_t::fpr_load : &vtree_chan_t::fpr_store))(vtree_chan_t::Register (gpa & BGP_SIZE_TREE1 - 1), fpr);
    return 0;
}

word_t vcpu_t::emulate_device_gpr (paddr_t gpa, word_t *reg, bool load)
{
    if (gpa >= BGP_PHYS_TORUS_DMA && gpa < BGP_PHYS_TORUS_DMA + BGP_SIZE_TORUS_DMA)
    {
        assert(BGP_PHYS_TORUS_DMA_GRP(gpa) <= 3);
        return (vm->torus.dma[BGP_PHYS_TORUS_DMA_GRP(gpa)].*(load ? &vtorus_dma_t::gpr_load : &vtorus_dma_t::gpr_store))(vtorus_dma_t::Register (gpa & BGP_SIZE_TORUS_DMA_GRP - 1), reg);
    }

    if (gpa >= BGP_PHYS_TREE0 && gpa < BGP_PHYS_TREE0 + BGP_SIZE_TREE0)
        return (vm->tree.chan0.*(load ? &vtree_chan_t::gpr_load : &vtree_chan_t::gpr_store))(vtree_chan_t::Register (gpa & BGP_SIZE_TREE0 - 1), reg);

    if (gpa >= BGP_PHYS_TREE1 && gpa < BGP_PHYS_TREE1 + BGP_SIZE_TREE1)
        return (vm->tree.chan1.*(load ? &vtree_chan_t::gpr_load : &vtree_chan_t::gpr_store))(vtree_chan_t::Register (gpa & BGP_SIZE_TREE1 - 1), reg);

    if (gpa >= BGP_PHYS_TOMAL && gpa < BGP_PHYS_TOMAL + BGP_SIZE_TOMAL)
        return (vm->tomal.*(load ? &vtomal_t::gpr_load : &vtomal_t::gpr_store))(vtomal_t::Register (gpa & BGP_SIZE_TOMAL - 1), reg);

    if (gpa >= BGP_PHYS_XEMAC && gpa < BGP_PHYS_XEMAC + BGP_SIZE_XEMAC)
        return (vm->xemac.*(load ? &vxemac_t::gpr_load : &vxemac_t::gpr_store))(vxemac_t::Register (gpa & BGP_SIZE_XEMAC - 1), reg);

    if (gpa >= BGP_PHYS_DEVBUS && gpa < BGP_PHYS_DEVBUS + BGP_SIZE_DEVBUS)
        return (vm->devbus.*(load ? &vdevbus_t::gpr_load : &vdevbus_t::gpr_store))(vdevbus_t::Register (gpa & BGP_SIZE_DEVBUS - 1), reg);
    
    if (gpa >= BGP_PHYS_BIC && gpa < BGP_PHYS_BIC + BGP_SIZE_BIC)
        return (vm->bic.*(load ? &vbic_t::gpr_load : &vbic_t::gpr_store))(vbic_t::Register (gpa & BGP_SIZE_BIC - 1), reg);

    if (gpa >= BGP_PHYS_DP83820 && gpa < BGP_PHYS_DP83820 + BGP_SIZE_DP83820)
        return (vm->dp83820.*(load ? &vdp83820_t::gpr_load : &vdp83820_t::gpr_store))(vdp83820_t::Register (gpa & BGP_SIZE_BIC - 1), reg);
        
        printf ("%s: unknown gpa:%lx%08lx load:%u\n", __PRETTY_FUNCTION__,
            static_cast<word_t>(gpa >> 32),
            static_cast<word_t>(gpa), load);

    L4_KDB_Enter ("emulate device access");

    return 0;
}

void vcpu_t::emulate (L4_MsgTag_t tag, L4_Msg_t &msg, L4_Msg_t &rep, word_t ip, ppc_instr_t opcode, word_t gva, paddr_t gpa)
{
    L4_GPRegsCtrlXferItem_t gpregs[2];
    unsigned mr = L4_UntypedWords (tag) + 1;
    
    L4_MsgPutGPRegsCtrlXferItem (&msg, mr, gpregs);
    L4_MsgPutGPRegsCtrlXferItem (&msg, mr, gpregs + 1);
    mr += 2;

    word_t core_mask = 0;

    switch (opcode.primary()) {

        case 31:
	        switch (opcode.secondary()) {

                case 23:    // lwzx
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), true);
                    break;

                case 151:    // stwx
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), false);
                    break;

                case 259:   // mfdcrx
                    core_mask = emulate_mfdcr (Dcr (*gpr (gpregs, opcode.ra())), gpr (gpregs, opcode.rt()));
                    break;

                case 323:   // mfdcr
                    core_mask = emulate_mfdcr (Dcr (opcode.rf()), gpr (gpregs, opcode.rt()));
                    break;

                case 339:   // mfspr
                    emulate_mfspr (Spr (opcode.rf()), gpr (gpregs, opcode.rt()));
                    break;

                case 387:   // mtdcrx
                    emulate_mtdcr (Dcr (*gpr (gpregs, opcode.ra())), *gpr (gpregs, opcode.rt()));
                    break;

                case 451:   // mtdcr
                    emulate_mtdcr (Dcr (opcode.rf()), *gpr (gpregs, opcode.rt()));
                    break;

	            case 454:   // dccci
                    break;

                case 462:   // lfpdx
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    core_mask = emulate_device_fpr (gpa, opcode.rt(), true);
                    break;

                case 467:   // mtspr
                    emulate_mtspr (Spr (opcode.rf()), *gpr (gpregs, opcode.rt()));
                    break;

                case 470:   // dcbi
                    break;

                case 494:   // lfpdux
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    printf ("LFPDUX GPA=%#lx RT=%u\n", static_cast<word_t>(gpa), opcode.rt());
                    break;
                    
                case 534:   // lwbrx
                    core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), true);
                    stwbrx( opcode.rt(), gpr (gpregs, opcode.rt()) );
                    break;

                case 662:   // stwbrx
                    stwbrx( *gpr (gpregs, opcode.rt()), gpr (gpregs, opcode.rt()) );
                    core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), false);
                    stwbrx( *gpr (gpregs, opcode.rt()), gpr (gpregs, opcode.rt()) );
                    break;
                case 966:   // iccci
                    break;

                case 974:   // stfpdx
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    core_mask = emulate_device_fpr (gpa, opcode.rt(), false);
                    break;

                case 1006:   // stfpdux
                    assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) : 0) + *gpr (gpregs, opcode.rb()));
                    printf ("STFPDUX GPA=%#lx RT=%u\n", static_cast<word_t>(gpa), opcode.rt());
                    break;

                default:
                    goto unhandled;
            }
            break;

        case 32:            // lwz
            assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) + opcode.d() : 0));
            core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), true);
            break;

        case 36:            // stw
            assert (gva == (opcode.ra() ? *gpr (gpregs, opcode.ra()) + opcode.d() : 0));
            core_mask = emulate_device_gpr (gpa, gpr (gpregs, opcode.rt()), false);
            break;

        default:
        unhandled:
            printf ("%s: IP:%08lx, OPCODE:%u:%u\n", __func__, ip, opcode.primary(), opcode.secondary());
	        L4_KDB_Enter ("unhandled instruction");
	        return;
    }

    set_event_inject_remote(core_mask);

    L4_GPRegsXCtrlXferItem_t gprx_item;
    L4_Init (&gprx_item);
    L4_Set (&gprx_item, L4_CTRLXFER_GPREGS_IP, ip + 4);
	L4_Append (&rep, &gprx_item);

    L4_MsgAppendGPRegsCtrlXferItem (&rep, gpregs);
    L4_MsgAppendGPRegsCtrlXferItem (&rep, gpregs + 1);
    L4_MsgAppendFPRegsCtrlXferItem (&rep);
}

void vcpu_t::handle_hvm_program (L4_MsgTag_t tag, L4_Msg_t &msg, L4_Msg_t &rep)
{
    emulate (tag, msg, rep, L4_Get (&msg, 1), L4_Get (&msg, 0), 0, 0);
}

void vcpu_t::handle_hvm_tlb (L4_MsgTag_t tag, L4_Msg_t &msg, L4_Msg_t &rep)
{
    word_t gva  = L4_Get (&msg, 0);
    word_t tlb0 = L4_Get (&msg, 4);
    word_t tlb1 = L4_Get (&msg, 5);
    word_t tlb2 = L4_Get (&msg, 6);
    word_t pidx = L4_Get (&msg, 7);

    signed char idx = pidx & 0xff;
    word_t  l2s = ((tlb0 >> 4 & 0xf) << 1) + 10;
    paddr_t gpa = static_cast<paddr_t>(tlb1 & 0xf) << 32 | (tlb1 & ~0x3ff) | (gva & (1ul << l2s) - 1);

#if 0
    printf ("%s: TLB[%d]: GVA:%08lx GPA:%01lx%08lx S:%lx\n",
            __func__,
            idx,
            gva,
            static_cast<word_t>(gpa >> 32),
            static_cast<word_t>(gpa),
            1ul << l2s);
#endif

    // Relocate TLB entries
    paddr_t rel;
    if (vm->sram.relocatable (gpa, rel)) {
        gva &= ~((1ul << l2s) - 1);
        rel &= ~((1ul << l2s) - 1);
#if 0
        printf ("Relocate for GVA:%08lx GPA:%01lx%08lx->%08lx L2S:%lu\n",
                gva,
                static_cast<word_t>(gpa >> 32),
                static_cast<word_t>(gpa),
                static_cast<word_t>(rel), l2s);
#endif
        set_tlb_entry (idx, gva, rel, l2s, tlb2, pidx >> 8);
        return;
    }

    // Page in guest memory for RAM and SRAM on demand
    word_t map;
    if (vm->ram.gpa_to_map (gpa, map) || vm->sram.gpa_to_map (gpa, map)) {

        if (l2s > 22)       // Back at most 4M of GPA at once
            l2s = 22;

        gpa &= ~((1ul << l2s) - 1);
        map &= ~((1ul << l2s) - 1);
#if 0
        printf ("Map FP:%08lx -> %08lx\n", map, static_cast<word_t>(gpa));
#endif
        L4_Fpage_t fp = L4_FpageLog2 (map, l2s) + L4_FullyAccessible;
        L4_Append (&rep, L4_MapItem (fp, gpa));
        return;
    }

    // Probably some device access, emulate instruction
    emulate (tag, msg, rep, L4_Get (&msg, 1), L4_Get (&msg, 2), gva, gpa);
}

word_t vcpu_t::irq_status()
{
    word_t event = 0;

    if (vm->bic.pending (vcpu, vbic_tsel_t::NCRIT))
        event |= L4_CTRLXFER_EXCEPT_EXTERNAL_INPUT;

    if (vm->bic.pending (vcpu, vbic_tsel_t::CRIT))
        event |= L4_CTRLXFER_EXCEPT_CRITICAL_INPUT;

    if (vm->bic.pending (vcpu, vbic_tsel_t::MCHK))
        event |= L4_CTRLXFER_EXCEPT_MACHINE_CHECK;

    return event;
}

void vcpu_t::set_event_inject()
{
    L4_Msg_t msg;
    L4_ExceptCtrlXferItem_t exc;
    
    
    word_t event = irq_status();
    //if (event) printf ("VCPU:%u PCPU:%u Pending interrupts (exregs):%#x\n", vcpu, pcpu, event);
    
    L4_Init (&exc);
    L4_Set (&exc, L4_CTRLXFER_EXCEPT_EVENT, event);

    L4_Clear (&msg);
    L4_Append (&msg, &exc);
    L4_Load (&msg);
    L4_WriteCtrlXferItems (tid);
}

void vcpu_t::set_event_inject_remote(word_t core_mask)
{
    //printf ("Core mask %#lx\n", core_mask);
    core_mask &= ~(1ul << vcpu);
    if (core_mask) 
    {
	for (unsigned i = 0; i < MAX_VCPU; i++)
	    if (core_mask & (1ul << i))
		vmthread_t::vmthreads[i].send_remote();
    }
    
}

bool vcpu_t::dispatch_message (L4_MsgTag_t tag)
{
    L4_Msg_t msg;
    L4_Msg_t rep;

    L4_Store (tag, &msg);
    L4_Clear (&rep);

    word_t fault = 0x1000 - (L4_Label (tag) >> 4);

    switch (fault) {

        case L4_FAULT_HVM_PROGRAM:
            handle_hvm_program (tag, msg, rep);
            break;

        case L4_FAULT_HVM_TLB:
            handle_hvm_tlb (tag, msg, rep);
            //L4_KDB_Enter ("=== VTLB ===");
            break;

        default:
            printf("TID %lx: unhandled fault %lu\n", tid.raw, fault);
    	    L4_KDB_Enter("unhandled fault");
    	    return false;
    }

    word_t event = irq_status();
    //if (event) printf ("VCPU:%u PCPU:%u Pending interrupts (resume):%#lx\n", vcpu, pcpu, event);

    L4_ExceptCtrlXferItem_t exc;
    L4_Init (&exc);
    L4_Set (&exc, L4_CTRLXFER_EXCEPT_EVENT, event);
    L4_Append (&rep, &exc);

    L4_Load (&rep);

    return true;
}

void vcpu_t::set_tlb_entry (int idx, word_t vaddr, paddr_t paddr,
			                int log2size, word_t attribs, int pid, bool valid)
{
#if 0
    printf ("Set TLBE[%d]: V:%lx P:%lx S:%u A:%lx PID:%x V:%u\n",
            idx, vaddr, static_cast<word_t>(paddr), log2size, attribs, pid, valid);
#endif
    
    L4_Msg_t ctrlxfer_msg;

    L4_TLBCtrlXferItem_t tlb;

    if (idx == -1) {
        idx = 0;
        L4_ShadowTLBCtrlXferItemInit (&tlb);
    } else
        L4_TLBCtrlXferItemInit (&tlb, idx / 4);

    L4_TLBCtrlXferItemSet (&tlb, L4_CTRLXFER_TLB_TLB0 (idx % 4),
                           PPC_TLB_RPN (vaddr) | PPC_TLB_SIZE(log2size) |
                           (valid ? PPC_TLB_VALID : 0));
    L4_TLBCtrlXferItemSet (&tlb, L4_CTRLXFER_TLB_TLB1 (idx % 4),
                           PPC_TLB_RPN (paddr) | PPC_TLB_ERPN (paddr >> 32));
    L4_TLBCtrlXferItemSet (&tlb, L4_CTRLXFER_TLB_TLB2 (idx % 4), attribs);
    L4_TLBCtrlXferItemSet (&tlb, L4_CTRLXFER_TLB_PID (idx % 4), pid);

    L4_Clear (&ctrlxfer_msg);
    L4_MsgAppendTLBCtrlXferItem (&ctrlxfer_msg, &tlb);
    L4_Load (&ctrlxfer_msg);
    L4_WriteCtrlXferItems (tid);
}

