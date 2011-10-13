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
#include <libmemcached/memcached.h>


class vm_t
{
public:
	enum Dcr {
		HV_MEMCACHED_ST					= 0x5c0,
		HV_MEMCACHED_SERVER_ST			= 0x5c1,
		HV_MEMCACHED_CONTINUUM_ST		= 0x5c2,
		HV_MEMCACHED_SERVER_INFO_ST 	= 0x5c3,
		HV_MEMCACHED_KERNEL_IMAGE_NAME	= 0x5c4,
		HV_MEMCACHED_RAMDISK_IMAGE_NAME = 0x5c5,
		HV_MEMCACHED_CHUNK				= 0x5c6
	};

    unsigned            num_vcpu;
    unsigned			index;
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
    
    memcached_st		*memc;
    char				*kernel_image_name, *ramdisk_image_name;

    void init (unsigned, unsigned, size_t, unsigned, unsigned);

    vm_t() : bic     (this),
             torus   (this),
             tree    (this), 
             dp83820 (this) {} 
    
    vcpu_t *get_vcpu (L4_ThreadId_t);
    
    word_t ack_irq(word_t irq);

    void handle_hypervisor_access(Dcr reg,word_t value);

private:
    void get_images();
    memcached_return memcached_khvmm_client_init(memcached_st *ptr);
    int khvmm_torus_alloc_rx(torus_client_info_t *info, torus_channel_t *chan, torus_dma_space_t *msp, size_t buf_size);
    int khvmm_torus_alloc_tx(torus_client_info_t *info, torus_channel_t *chan, torus_dma_space_t *msp, size_t fifo_size, size_t buf_size);
    torus_dma_space_t *create_dma_space(size_t size);
    memcached_return memcached_khvmm_client_create(memcached_st *ptr);

};
