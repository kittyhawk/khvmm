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

#include "access.h"
#include "config.h"
#include "personality.h"
#include "resource.h"
#include "vbic.h"
#include "vcpu.h"
#include "vdevbus.h"
#include "vtorus.h"
#include "vtest.h"
#include "vtomal.h"
#include "vtree.h"
#include "vxemac.h"
#include "vtree.h"
#include "vdp83820.h"

class vm_t
{
public:
    unsigned            num_vcpu;
    L4_Fpage_t          kip_area;
    L4_Fpage_t          utcb_area;
    resource_t          ram;
    resource_t          sram;
    comm_control_t      comm_control;
    
    vcpu_t              vcpu[MAX_VCPU];
    vbic_t              bic;
    vdevbus_t           devbus;

    vtorus_t            torus;
    vtest_t             test;
    vtomal_t            tomal;
    vtree_t             tree;
    vxemac_t            xemac;
    vdp83820_t          dp83820;

    bgp_personality_t *bgp;
    
    void init (unsigned, unsigned, size_t);

    vm_t() : bic     (this),
             torus   (this),
             tree    (this), 
             dp83820 (this) {} 
    
    vcpu_t *get_vcpu (L4_ThreadId_t);
    
    word_t ack_irq(word_t irq);
};
