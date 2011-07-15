/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 *                
 * File path:     main.cc
 * Description:   Kittyhawk VMM implementation
 *                
 * All rights reserved
 *                
 ********************************************************************/

#include "config.h"
#include "fdt.h"
#include "ptree.h"
#include "ptorus.h"
#include "std.h"
#include "vcpu.h"
#include "vm.h"
#include "vmthread.h"

#define VERSION "0.1"
#define KB(x) (x*1024)
#define MB(x) (x*1024*1024)
#define GB(x) (x*1024*1024*1024)

ptree_t ptree_t::tree (BGP_KHVMM_TREE);
ptorus_t ptorus_t::torus(BGP_KHVMM_TORUS_DMA);

vm_t vm[NUM_VMS];
//vm_t vm2;

void allocate_memory()
{
    L4_Word_t tsize = 0;

    for (L4_Word_t s = sizeof (L4_Word_t) * 8 - 1; s >= 10; s--)
    {
        L4_Fpage_t fp;
        int n = -1;

        do {
	        fp = L4_Sigma0_GetAny (L4_nilthread, s, L4_CompleteAddressSpace);
	        n++;
	    } while (!L4_IsNilFpage (fp));

	    L4_Word_t size = n * (1ul << s);
	    tsize += size;
#if 0
	    if (n)
	        printf ("Allocated %d pages of %3ld%cB (log2size %2ld) [%ld%cB]\n",
		            n,
		            s >= 30 ? 1UL << (s-30) :
		            s >= 20 ? 1UL << (s-20) : 1UL << (s-10),
		            s >= 30 ? 'G' : s >= 20 ? 'M' : 'K',
		            s,
		            size >= GB(1) ? size/GB(1) :
		            size >= MB(1) ? size/MB(1) : size/KB(1),
		            size >= GB(1) ? 'G' : size >= MB(1) ? 'M' : 'K');
#endif
    }

    printf ("\nTotal memory: %ld.%ldMB | %ldKB\n",
            tsize / MB(1), ((tsize * 100) / MB(1)) % 100, tsize / KB(1));
}

void map_memory()
{
    extern char _start[], _end[];
    for (char *addr = _start; addr < _end; addr += 4096)
	*(volatile char*)addr;
}

extern "C" int main()
{
	int i;
	bool shared_script = false;
	// Call static constructors
    extern void (*__CTOR_LST__)(), (*__CTOR_END__)();
    for (void (**func)() = &__CTOR_END__; func != &__CTOR_LST__; (*--func)()) ;

    printf ("Kittyhawk VMM version " VERSION "\n");

    L4_KernelInterfacePage_t *kip = static_cast<L4_KernelInterfacePage_t *>(L4_GetKernelInterface());

    if (!kernel_has_feature (kip, "powerpc-hvm")) {
        printf ("Kernel doesn't support hypervisor mode\n");
	    return -1;
    }

    fdt_t::fdt = new (kip->BootInfo) fdt_t;
    L4_Fpage_t fdt_page = L4_FpageAddRights(L4_FpageLog2(kip->BootInfo,0x1111),0x111);
    L4_Sigma0_GetPage_RcvWindow(L4_nilthread,fdt_page,fdt_page);
    if (!(fdt_t::fdt->is_valid())) {
    	printf("Invalid FDT!\n");
    	return -1;
    }

    map_memory(); 
    allocate_memory();
#if 0 //we want this done by memcached
    //copy the kernel and ramdisk images from the first VM to all the others if necessary.
    printf("\n");
    for (i = 1; i < NUM_VMS; ++i) {
    	printf("Checking for VM%d's kernel image... ",i);
    	if (*(reinterpret_cast<int *>(((RAM_SIZE / NUM_VMS * i) & ~0x3FFFFF)
    			+ RAM_MAP + KERNEL_OFFSET)) != 0x27051956) {
    		printf("not found, copying from VM0\n");
			memcpy(reinterpret_cast<void *>(((RAM_SIZE / NUM_VMS * i)
						& ~0x3FFFFF) + RAM_MAP + KERNEL_OFFSET),
					reinterpret_cast<void *>(RAM_MAP + KERNEL_OFFSET),
					KERNEL_SIZE);
    	} else
    		printf("found\n");

    	printf("Checking for VM%d's ramdisk image... ",i);
    	if (*(reinterpret_cast<int *>(((RAM_SIZE / NUM_VMS * i) & ~0x3FFFFF)
    			+ RAM_MAP + RAMDISK_OFFSET)) != 0x27051956) {
    		printf("not found, copying from VM0\n");
			memcpy(reinterpret_cast<void *>(((RAM_SIZE / NUM_VMS * i)
						& ~0x3FFFFF) + RAM_MAP + RAMDISK_OFFSET),
					reinterpret_cast<void *>(RAM_MAP + RAMDISK_OFFSET),
					RAMDISK_SIZE);
    	} else
    		printf("found\n");

    	printf("Checking for VM%d's u-boot script image... ",i);
    	if (*(reinterpret_cast<int *>(((RAM_SIZE / NUM_VMS * i) & ~0x3FFFFF)
    			+ RAM_MAP + SCRIPT_OFFSET)) != 0x27051956) {
    		printf("not found, copying from VM0\n");
    		shared_script = true;
			memcpy(reinterpret_cast<void *>(((RAM_SIZE / NUM_VMS * i)
						& ~0x3FFFFF) + RAM_MAP + SCRIPT_OFFSET),
					reinterpret_cast<void *>(RAM_MAP + SCRIPT_OFFSET),
					SCRIPT_SIZE);
    	} else
    		printf("found\n");
    }

    if (shared_script)
    	printf("\n!!!WARNING!!! Using the same u-boot script for more than one VM will most likely break your network configuration!\n");
    printf("\n");
#endif
    vmthread_t::init (L4_NumProcessors (kip));
#if 0
    for (i = 0; i < NUM_VMS; ++i)
    	vm[i].init (L4_NumProcessors(kip),
    			L4_NumProcessors(kip),
    			(RAM_SIZE / NUM_VMS) & ~0x3FFFFF,
    			(RAM_SIZE / NUM_VMS * i) & ~0x3FFFFF,
    			i);
#else //just init VM0 (dom0) here.
	vm[0].init (L4_NumProcessors(kip),
			L4_NumProcessors(kip),
			(RAM_SIZE / NUM_VMS) & ~0x3FFFFF,
			(RAM_SIZE / NUM_VMS * 0) & ~0x3FFFFF,
			0);
#endif
    ptree_t::tree.init(vmthread_t::get_tid(0));
    ptorus_t::torus.init(vmthread_t::get_tid(0));

    L4_Sleep(L4_Never);
    assert(false);
}
