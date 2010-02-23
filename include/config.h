/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       config.h
 * Description:     Configuation
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#define MAX_VCPU        4
#define MAX_PCPU        4
#define STACK_SIZE      (16 * 1024)

#define CPU_FREQ        850000000ul

#define RAM_GPA         0x0ul
#define RAM_REAL        0x10000000ul
#define RAM_MAP         0x10000000ul
#define RAM_SIZE        0x40000000ul

#define SRAM_GPA        0x0afff8000ul
#define SRAM_MAP        RAM_MAP + RAM_SIZE
#define SRAM_SIZE	0x8000ul
#define SRAM_REAL       0x7ffff8000ull

#define SRAM_BOOT_VIRT	0xfffff000ul

#define SRAM_PERS_TABLE_OFFS    0x7800ul
#define SRAM_MBOX_TABLE_OFFS    0x7fd0ul

#define SRAM_MBOX_TO_CORE_OFFS  0x1000ul
#define SRAM_MBOX_TO_CORE_SIZE  0x1000ul

#define SRAM_MBOX_TO_HOST_OFFS  0x7400ul
#define SRAM_MBOX_TO_HOST_SIZE  0x100ul

// See: arch/include/bpcore/bgp_PhysicalMap.h
#define BGP_PHYS_TORUS_DMA      0x600000000ull
#define BGP_SIZE_TORUS_DMA      0x4000
#define BGP_SIZE_TORUS_DMA_GRP  0x1000
#define BGP_PHYS_TORUS_DMA_GRP(pa)    ((word_t) ((pa - BGP_PHYS_TORUS_DMA) >> 12))


#define BGP_PHYS_TREE0          0x610000000ull
#define BGP_SIZE_TREE0          0x400

#define BGP_PHYS_TREE1          0x611000000ull
#define BGP_SIZE_TREE1          0x400

#define BGP_PHYS_TOMAL          0x720000000ull
#define BGP_SIZE_TOMAL          0x4000

#define BGP_PHYS_XEMAC          0x720004000ull
#define BGP_SIZE_XEMAC          0x1000

#define BGP_PHYS_DEVBUS         0x720005000ull
#define BGP_SIZE_DEVBUS         0x1000

#define BGP_PHYS_BIC            0x730000000ull
#define BGP_SIZE_BIC            0x1000

#define BGP_PHYS_TREE0          0x610000000ull
#define BGP_SIZE_TREE0          0x400

#define BGP_PHYS_DP83820        0x740000000ull
#define BGP_SIZE_DP83820        0x1000


// sigma0 remapping
#define BGP_SIGMA0_TORUS_DMA	0xff000000
#define BGP_SIGMA0_TORUS0	0xff010000
#define BGP_SIGMA0_TORUS1	0xff018000
#define BGP_SIGMA0_TREE0	0xff020000
#define BGP_SIGMA0_TREE1	0xff021000
#define BGP_SIGMA0_TOMAL	0xff030000

// khvmmm remapping
#define BGP_KHVMM_TREE          0x90000000
#define BGP_KHVMM_TORUS_DMA	0x90010000


/* L4 scheduling */
#define PRIO_VMTHREAD	128
#define PRIO_VCPU	10

/* tree / torus link protocol */
#define BGLINK_P_NET	0x01
#define BGLINK_P_CON	0x10
