
/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *                
 * File path:     vmthread.cc
 * Description:   VM Thread
 *                
 * All rights reserved
 *                
 ********************************************************************/

#include "config.h"
#include "ptorus.h"
#include "ptree.h"
#include "sync.h"
#include "vcpu.h"
#include "vmthread.h"

vmthread_t vmthread_t::vmthreads[MAX_PCPU];
word_t vmthread_t::tid_system_base;

vcpu_t * vmthread_t::registry[64];
unsigned vmthread_t::registry_idx;

bool vmthread_t::remote_vcpu_bootstrap (vcpu_t *vcpu, word_t ip)
{
    return vcpu->launch_vcpu (ip);
}

bool vmthread_t::vcpu_bootstrap (vcpu_t *vcpu, word_t ip)
{
    return call_remote (reinterpret_cast<void *>(remote_vcpu_bootstrap),
                        reinterpret_cast<word_t>(vcpu),
                        ip);
}

bool vmthread_t::handle_remote (L4_MsgTag_t tag)
{
    L4_Msg_t msg;
    L4_MsgStore (tag, &msg);

    bool (*func)(word_t, word_t, word_t, word_t) = reinterpret_cast<bool(*)(word_t, word_t, word_t, word_t)>(L4_Get (&msg, 0));
    bool res = func (L4_Get (&msg, 1),
                     L4_Get (&msg, 2),
                     L4_Get (&msg, 3),
                     L4_Get (&msg, 4));

    // Create reply message
    L4_Clear   (&msg);
    L4_Append  (&msg, static_cast<word_t>(res));
    L4_MsgLoad (&msg);

    return true;
}

bool vmthread_t::handle_hwirq(word_t irq, word_t &core_mask)
{
    if (ptorus_t::torus.is_irq(irq)) 
	return ptorus_t::torus.handle_hwirq(irq, core_mask);
    if (ptree_t::tree.is_irq(irq)) 
	return ptree_t::tree.handle_hwirq(irq, core_mask);
    
    printf("unhandled hwirq %d\n", (int) irq);
    L4_KDB_Enter("BUG");
    
    return false;
}

bool vmthread_t::call_remote (void *func, word_t p0, word_t p1, word_t p2, word_t p3)
{
    L4_Msg_t msg;
    L4_Clear (&msg);
    L4_Set_Label(&msg, 0);
    L4_Append (&msg, reinterpret_cast<word_t>(func));
    L4_Append (&msg, p0);
    L4_Append (&msg, p1);
    L4_Append (&msg, p2);
    L4_Append (&msg, p3);
    L4_Load   (&msg);
    L4_MsgTag_t tag = L4_Call (tid);

    return L4_IpcSucceeded (tag) && L4_Label (tag);
}

bool vmthread_t::send_remote()
{
    //printf ("Kicking PCPU:%u TID:%#lx\n", cpu, tid.raw);
    L4_MsgTag_t tag;
    
#warning jsXXX: fix deadlock-hazard on sending IPIs 
    
    do {
        L4_LoadMR(0, 0);
        tag = L4_Reply (tid);
    } while (L4_IpcFailed (tag));
    return true;    
}

__attribute__((noreturn))
    void vmthread_t::message_loop()
{
    L4_ThreadId_t src, dst, control_ltid;
    L4_MsgTag_t tag;
    
    control_ltid = L4_LocalIdOf(control_tid);
    
    printf ("VMM thread started on CPU %u (%lx)\n", cpu, L4_Myself().raw);

    L4_Accept(L4_UntypedWordsAcceptor);
    L4_LoadMR(0, 0);
    L4_Send(control_tid); // handshake with control thread

    for (dst = L4_nilthread;;) {

    	tag = L4_ReplyWait (dst, &src);
	
    	if (L4_IpcFailed (tag)) {
            printf ("Error: %#lx\n", L4_ErrorCode());
            L4_KDB_Enter ("IPC error");
    	    continue;
    	}

        // Control thread 
        if (src.raw == control_ltid.raw) {
            if (handle_remote (tag))
		dst = src;
            continue;
        }

        // Other VMthread
	if (L4_IsLocalId (src)) {
	    //printf ("Kicking VCPU via exregs:%u\n", cpu);
            vcpu_t *vcpu = registry[cpu + 1];
            vcpu->set_event_inject();
            dst = L4_nilthread;
	    continue;
	}

	// HW IRQ
	word_t tno = L4_ThreadNo(src);
	if (tno < tid_system_base)
	{
	    word_t core_mask = 0;
            
            dst = handle_hwirq(tno, core_mask) ? src : L4_nilthread;
	    
	    vcpu_t *vcpu = registry[cpu+1];
            if (vcpu) 
	    {
		if (core_mask & (1ul << cpu))
		    vcpu->set_event_inject();
		vcpu->set_event_inject_remote(core_mask);

	    }
	    continue;
	}

        // Figure out which vCPU sent us a message
	vcpu_t *vcpu = registry[L4_Version (src)];

#if 0
	printf ("vcpu:%p got message from (tag:%lx, label:%lx, t:%lu, u:%lu)\n",
                vcpu, tag.raw,
                L4_Label (tag), L4_TypedWords (tag), L4_UntypedWords (tag));
#endif

        if (!vcpu || !vcpu->dispatch_message (tag))
	    continue;

        dst = vcpu->tid;
    }
}

void vmthread_t::run()
{
    for (unsigned i = 0; i < MAX_PCPU; i++)
	    if (vmthreads[i].tid == L4_Myself())
	        vmthreads[i].message_loop();

    L4_KDB_Enter ("launching CPU thread failed");
    L4_Sleep (L4_Never);
}

bool vmthread_t::start (unsigned cpuid, L4_ThreadId_t ctrl)
{
    tid = get_new_tid();
    cpu = cpuid;
    control_tid = ctrl;

    smp_mem_barrier();

    // place new thread within UTCB area
    void * kip = L4_KernelInterface();
    L4_ThreadId_t mylocalid = L4_MyLocalId();
    L4_Word_t utcb_base = *(L4_Word_t *) &mylocalid;
    utcb_base &= ~(L4_UtcbAreaSize (kip) - 1);
    void *utcb_location = reinterpret_cast<void*>
	(utcb_base + L4_UtcbSize (kip) * (L4_ThreadNo(tid) - L4_ThreadNo(L4_Myself()) ));
    
    tid_system_base = L4_ThreadIdSystemBase (kip);

    printf("Starting CPU %u: Tid: %lx, Ctrl: %lx\n", cpu, tid.raw, control_tid.raw);

    if (L4_ThreadControl(tid, L4_Myself(), L4_Myself(), L4_Myself(), utcb_location) != 1) {
    	printf("Failed to create VMM thread %lx (err=%lx)\n", tid.raw, L4_ErrorCode());
    	return false;
    }

    L4_Set_UserDefinedHandleOf (tid, cpu);
    L4_Set_ProcessorNo_Prio (tid, cpu, PRIO_VMTHREAD);


    L4_Msg_t msg;
    L4_Clear(&msg);
    L4_Append(&msg, reinterpret_cast<word_t>(&run));
    L4_Append(&msg, reinterpret_cast<word_t>(&stack[STACK_SIZE - 128]));
    L4_MsgLoad(&msg);
    L4_Call(tid); /* call rendezvous with thread */

    return true;
}

void vmthread_t::init (unsigned num_pcpu)
{
    if (num_pcpu > MAX_PCPU)
        num_pcpu = MAX_PCPU;

    // launch one handler thread per physical processor
    for (unsigned i = 0; i < num_pcpu; i++)
        if (!vmthreads[i].start (i, L4_MyLocalId())) {
            L4_KDB_Enter("failed thread launch");
            return;
        }
}
