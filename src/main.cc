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

vm_t vm;

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

    map_memory(); 
    allocate_memory();

    vmthread_t::init (L4_NumProcessors (kip));

    vm.init (L4_NumProcessors(kip), L4_NumProcessors(kip), RAM_SIZE);
    
    ptree_t::tree.init(vmthread_t::get_tid(0));
    ptorus_t::torus.init(vmthread_t::get_tid(0));

    L4_Sleep(L4_Never);
    assert(false);
}
