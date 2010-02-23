/*********************************************************************
 *
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vm.h
 * Description:     Virtual Machine
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class vcpu_t;

class vmthread_t
{
private:
    static vcpu_t *     registry[64];
    static unsigned     registry_idx;
    static word_t	tid_system_base;
    char                stack[STACK_SIZE] __attribute__((aligned(16)));

    static bool remote_vcpu_bootstrap (vcpu_t *, word_t);

    bool handle_remote (L4_MsgTag_t);
    bool handle_hwirq(word_t irq, word_t &core_mask);

    void message_loop();
    static void run();

public:
    unsigned            cpu;
    L4_ThreadId_t       tid;
    L4_ThreadId_t       control_tid;
    static vmthread_t   vmthreads[MAX_PCPU];
    
    static L4_ThreadId_t get_tid (unsigned pcpu)
        {
            return vmthreads[pcpu].tid;
        }

    static unsigned register_vcpu (vcpu_t *vcpu)
        {
            registry[++registry_idx] = vcpu;
            return registry_idx;
        }

    bool vcpu_bootstrap (vcpu_t *, word_t);

    bool call_remote (void *, word_t = 0, word_t = 0, word_t = 0, word_t = 0);
    bool send_remote();

    bool start (unsigned, L4_ThreadId_t);
    static void init (unsigned);
};
