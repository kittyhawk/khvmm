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
#include "malloc.h"
#include <assert.h>
#include <libmemcached/memcached_bluegene.h>

#define CHUNK_SIZE 4096

extern vm_t vm[NUM_VMS];

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

void vm_t::init (unsigned v, unsigned p, size_t memsize, unsigned memoffset, unsigned vm_number)
{
    if (v > MAX_VCPU)
        v = MAX_VCPU;

    if (p > MAX_PCPU)
        p = MAX_PCPU;

    printf ("Initializing VM (%u vCPUs, %u pCPUs, %uMB RAM)\n", v, p, memsize / (1024*1024));

    num_vcpu = v;
    index = vm_number;

    void *kip = L4_KernelInterface();
    kip_area = L4_FpageLog2 ((L4_Word_t) kip, L4_KipAreaSizeLog2 (kip));

    word_t utcb_base = L4_MyLocalId().raw;
    utcb_base &= ~(L4_UtcbAreaSize (kip) - 1);
    size_t size = 1 << L4_UtcbAreaSizeLog2(kip);
    //printf("area size: %d, sz: %d\n", size, L4_UtcbSize(kip) * 2 * numcpu);

    if (size < L4_UtcbSize(kip) * 2 * num_vcpu)
        size = L4_UtcbSize(kip) * 2 * num_vcpu;

    utcb_area = L4_Fpage (utcb_base, size);

    ram = resource_t (RAM_REAL + memoffset, RAM_GPA, memsize, RAM_MAP + memoffset);
    printf ("Resource RAM:  GPA:%08lx MAP:%08lx S:%lx\n",
            RAM_GPA, RAM_MAP + memoffset, memsize);

    sram = resource_t (SRAM_REAL, SRAM_GPA, SRAM_SIZE, SRAM_MAP);
    printf ("Resource SRAM: GPA:%08lx MAP:%08lx S:%lx\n",
            SRAM_GPA, SRAM_MAP, SRAM_SIZE);

    // load the first module
    elf_hdr_t::load_file (0x1700000, ram.map_base, sram.map_base + sram.size);

    // set up personality page
    bgp = new (SRAM_MAP + SRAM_PERS_TABLE_OFFS) bgp_personality_t (CPU_FREQ, memsize);

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

void vm_t::handle_hypervisor_access(Dcr reg,word_t value) {
	switch (reg) {
		case HV_MEMCACHED_ST:
			memc = (memcached_st *)malloc(
					sizeof(memcached_st)
					);
			paddr_t guest_memc;
			ram.gpa_to_hpa(value,guest_memc);
			memcpy( memc,
					(void *)guest_memc,
					sizeof(memcached_st)
					);
			//printf("Received memcached_st\n");
			break;
		case HV_MEMCACHED_SERVER_ST:
			memc->hosts = (memcached_server_st *)malloc(
					sizeof(memcached_server_st) * (word_t)memc->number_of_hosts
					);
			paddr_t guest_hosts;
			ram.gpa_to_hpa(value,guest_hosts);
			memcpy( memc->hosts,
					(void *)guest_hosts,
					sizeof(memcached_server_st) * (word_t)memc->number_of_hosts
					);
			//printf("Received memcached_server_st\n");
			break;
		case HV_MEMCACHED_CONTINUUM_ST:
			//continuum is only read, so it can stay in guest memory.
			paddr_t guest_continuum;
			ram.gpa_to_hpa(value,guest_continuum);
			memc->continuum = (memcached_continuum_item_st *)guest_continuum;
			//printf("Received continuum\n");
			break;
		case HV_MEMCACHED_SERVER_INFO_ST: {
			torus_server_info_t *server_info_buf = (torus_server_info_t *)malloc(
					sizeof(torus_server_info_t) * (word_t)memc->number_of_hosts
					);
			paddr_t guest_server_info;
			ram.gpa_to_hpa(value,guest_server_info);
			memcpy( server_info_buf,
					(void *)guest_server_info,
					sizeof(torus_server_info_t) * (word_t)memc->number_of_hosts
					);
			//printf("Received server torus info\n");
			int i;
			for (i = 0; i < (word_t)memc->number_of_hosts; ++i) {
				memc->hosts[i].torus_info = &server_info_buf[i];
				memc->hosts[i].root = memc;
			}
			//printf("Fixed broken pointers\n");
			break;
		}
		case HV_MEMCACHED_KERNEL_IMAGE_NAME:
			//read only, stays in guest memory
			//char *kernel_image_name;
			paddr_t guest_kernel_image_name;
			ram.gpa_to_hpa(value,guest_kernel_image_name);
			kernel_image_name = (char *)guest_kernel_image_name;
			//printf("Time to get the kernel image named %s\n",kernel_image_name);
			break;

		case HV_MEMCACHED_RAMDISK_IMAGE_NAME:
			//read only, stays in guest memory
			//char *ramdisk_image_name;
			paddr_t guest_ramdisk_image_name;
			ram.gpa_to_hpa(value,guest_ramdisk_image_name);
			ramdisk_image_name = (char *)guest_ramdisk_image_name;
			//printf("Time to get the ramdisk image named %s\n",ramdisk_image_name);
			get_images();
			break;
		case HV_MEMCACHED_CHUNK:
			paddr_t chunk_addr;
			ram.gpa_to_hpa(value,chunk_addr);
			memcpy((void *)(0x30000000 + RAMDISK_OFFSET),(void *)chunk_addr,CHUNK_SIZE);
			break;
	    default:
	        printf ("%s: DCR:%#lx <- V:%#lx\n", __func__, static_cast<word_t>(reg), value);
	        L4_KDB_Enter ("MTDCR: unknown DCR");
	}
}

extern "C" int memcached_bg_remote_get(torus_client_info_t *client_info, void *rxbuf, int rxtype,
                            torus_server_info_t *server_info, unsigned server_offset, unsigned long len, bool poll);

void vm_t::get_images() {
#if 0
	printf("Trying to get images\n");
	printf("Torus info:\n");
	ptorus_t::torus.dump_dcrs();
	ptorus_t::torus.dma->dump_rcv_channel(9,64);
	ptorus_t::torus.dma->dump_inj_channel(32,64);
#endif
	memcached_khvmm_client_create(memc);
	if (memcached_khvmm_client_init(memc) != MEMCACHED_SUCCESS) {
		printf("bg_client_init failed\n");
		return;
	}
	memcached_return rc;

	//declarations go here!
	size_t string_length;
	uint32_t flags;
	int i;
	torus_server_info_t *server_info;
	char *rcv_buf, *chunk_key;
	char *image_ptr;

	for (i = 0; i < memc->number_of_hosts; ++i) {
		server_info = memc->hosts[i].torus_info;
	    if ((server_info->hashtable_rx = (torus_channel_t *) malloc(sizeof(torus_channel_t))) == NULL)
	    {
	        printf("%s: allocating hashtable structure failed\n", __PRETTY_FUNCTION__);
	    }
	    if (khvmm_torus_alloc_rx(memc->torus_info, server_info->hashtable_rx, NULL, server_info->hashtable_size) < 0)
	    {
	        printf("%s: allocating hashtable rx channel failed\n", __PRETTY_FUNCTION__);
	    }
#if 1
	    if ((server_info->hashtable = (item **)mspace_malloc(server_info->hashtable_rx->dma_space->msp, server_info->hashtable_size)) == NULL)
	    {
	        printf("%s: allocating hashtable failed\n", __PRETTY_FUNCTION__);
	    } else
	    	server_info->hashtable_rx->dma_space->refcnt++;
	    if (memcached_bg_remote_get(memc->torus_info, server_info->hashtable, 2, server_info, server_info->hashtable_offset, server_info->hashtable_size, true) < 0)
	    {
	        printf("%s: retrieving hashtable via remote get failed\n", __PRETTY_FUNCTION__);
	    }
#endif
	    server_info->refcnt = 1;
	}
#if 0
	printf("Torus info:\n");
	ptorus_t::torus.dump_dcrs();
	ptorus_t::torus.dma->dump_rcv_channel(9,64);
	ptorus_t::torus.dma->dump_inj_channel(32,64);
	printf("Server info:\n");
	printf("Pointer address: %p\n",memc->hosts);
	if (memc->hosts) {
		printf("First hostname: %s\n",memc->hosts->hostname);
		printf("First torus info pointer: %p\n",memc->hosts->torus_info);
		if (memc->hosts->torus_info) {
			torus_server_info_t *server_info = memc->hosts->torus_info;
			printf("Torus coordinates: %d:%d:%d\n",server_info->coordinates.x,server_info->coordinates.y,server_info->coordinates.z);
		}
	}
#endif
	//get kernel image
	i = 0;
	chunk_key = (char *)malloc(strlen(kernel_image_name) + 10);
	image_ptr = (char *)(0x30000000 + KERNEL_OFFSET);
	do {
		sprintf(chunk_key,"%s.%d",kernel_image_name,++i);
		rcv_buf = memcached_get(memc,chunk_key,strlen(chunk_key),&string_length,&flags,&rc);
		memcpy(image_ptr,rcv_buf,string_length);
		//printf("%s -> %p\n",chunk_key,image_ptr);
		//printf("%s (%d bytes)\n",chunk_key,string_length);
		image_ptr += string_length;
	} while (string_length == CHUNK_SIZE);
	free(chunk_key);

	//get ramdisk image
	i = 0;
	chunk_key = (char *)malloc(strlen(ramdisk_image_name) + 10);
	image_ptr = (char *)(0x30000000 + RAMDISK_OFFSET);
	do {
		sprintf(chunk_key,"%s.%d",ramdisk_image_name,++i);
		rcv_buf = memcached_get(memc,chunk_key,strlen(chunk_key),&string_length,&flags,&rc);
		if (rcv_buf == NULL) {
			printf("Chunk %s failed\n",chunk_key);
			string_length=CHUNK_SIZE;
		} else
			memcpy(image_ptr,rcv_buf,string_length);
		//printf("%s (%d bytes)\n",chunk_key,string_length);
		//printf("%s -> %p\n",chunk_key,image_ptr);
		image_ptr += string_length;
		if ((unsigned int)image_ptr > (unsigned int)(0x50000000 - CHUNK_SIZE))
			image_ptr = (char *)(0x30000000 + RAMDISK_OFFSET);
	} while (string_length == CHUNK_SIZE);
	free(chunk_key);

	//all done! now boot...
#if 0
	vm[1].init (num_vcpu,
			num_vcpu,
			(RAM_SIZE / NUM_VMS) & ~0x3FFFFF,
			(RAM_SIZE / NUM_VMS * 1) & ~0x3FFFFF,
			1);
#endif
}

memcached_return vm_t::memcached_khvmm_client_init(memcached_st *ptr)
{
	int i;
	int cfd;
	torus_client_info_t *client_info = ptr->torus_info;

	assert(client_info);

	if (!memcached_bg_enabled(ptr))
		return MEMCACHED_SUCCESS;

	if (++client_info->refcnt > 1)
		return MEMCACHED_SUCCESS;

#if 1
	//dprintf(client_info, 1, "%s: (%p)\n", __PRETTY_FUNCTION__, ptr);


	client_info->root =ptr;

	client_info->coordinates.x = ptorus_t::torus.get_coordinate(torus_packet_t::XAX);
	client_info->coordinates.y = ptorus_t::torus.get_coordinate(torus_packet_t::YAX);
	client_info->coordinates.z = ptorus_t::torus.get_coordinate(torus_packet_t::ZAX);

	client_info->dma = (torus_dma_s *)BGP_KHVMM_TORUS_DMA;

	ptorus_t::torus.set_memcached_dma_inj_region((paddr_t)mspace_least_addr(theMspace),(paddr_t)mspace_least_addr(theMspace) + (paddr_t)mspace_max_footprint(theMspace) - 1);
	ptorus_t::torus.set_memcached_dma_rcv_region((paddr_t)mspace_least_addr(theMspace),(paddr_t)mspace_least_addr(theMspace) + (paddr_t)mspace_max_footprint(theMspace) - 1);
	/*
	 * We allocate a chunk sufficiently large to host all TX and RX buffers;
	 *  each buffer will get its own mspace (see palloc.c)
	 */

	// Alloc channels
	if ((client_info->slabs_rx = (torus_channel_t *)malloc(sizeof(torus_channel_t))) == NULL)
	{
		printf("%s: allocating slabs rx structure failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if (khvmm_torus_alloc_rx(client_info, client_info->slabs_rx, NULL, DEFAULT_TORUS_RX_BUFSIZE) < 0)
	{
		printf("%s: allocating slabs rx channel failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if ((client_info->slabkeys_rx = (torus_channel_t *)malloc(sizeof(torus_channel_t))) == NULL)
	{
		printf("%s: allocating slab keys rx structure failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if (khvmm_torus_alloc_rx(client_info, client_info->slabkeys_rx, NULL, DEFAULT_TORUS_KEYSRX_BUFSIZE) < 0)
	{
		printf("%s: allocating slab keys rx channel failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if ((client_info->get_tx = (torus_channel_t *) malloc(sizeof(torus_channel_t))) == NULL)
	{
		printf("%s: allocating get tx structure failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if (khvmm_torus_alloc_tx(client_info, client_info->get_tx, NULL, DEFAULT_TORUS_FIFO_SIZE, client_info->gettx_size) < 0) {
		printf("%s: allocating get tx channel failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if ((client_info->set_tx = (torus_channel_t *) malloc(sizeof(torus_channel_t))) == NULL)
	{
		printf("%s: allocating get tx structure failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	if (khvmm_torus_alloc_tx(client_info, client_info->set_tx, NULL, DEFAULT_TORUS_FIFO_SIZE, client_info->settx_size) < 0)
	{
		printf("%s: allocating set tx channel failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}
#if 0
	dprintf(client_info, 1, "%s: torus client initialized [%d,%d,%d] dma %p slabs %p set %p get %p fdt %d\n", __PRETTY_FUNCTION__,
			client_info->coordinates.x, client_info->coordinates.y, client_info->coordinates.z, client_info->dma, client_info->slabs_rx,
			client_info->set_tx->dma_space->msp, client_info->get_tx->dma_space->msp, client_info->fdt);
#endif
#endif
	return MEMCACHED_SUCCESS;

failure:
	if (client_info->get_tx)
		free(client_info->get_tx);
	if (client_info->set_tx)
		free(client_info->set_tx);
#if 0 //we don't use file descriptors for these
	if (client_info->fdt != -1)
		close(client_info->fdt);
	if (client_info->bigphys_fdt != -1)
		close(client_info->bigphys_fdt);
	if (client_info->dma) //this one was just statically assigned
		munmap(client_info->dma, 0x3000);
#endif
	if (client_info)
		free(client_info);
	return MEMCACHED_FAILURE;

}

int vm_t::khvmm_torus_alloc_rx(torus_client_info_t *info, torus_channel_t *chan, torus_dma_space_t *msp, size_t buf_size)
{
	long long pbuf;
	int ctr, ctr_grp, ctr_idx;

	if (((chan->dma_space = msp) == NULL) &&
		((chan->dma_space = create_dma_space(buf_size)) == NULL))
	{
		printf("%s: rx DMA space allocation failed\n", __PRETTY_FUNCTION__);
		goto failure;
	}

	chan->dma_space->refcnt++;

	chan->buf = mspace_least_addr(chan->dma_space->msp);
	chan->buf_size = mspace_max_footprint(chan->dma_space->msp);


#if 1
	pbuf  = (long long)chan->buf;

	//ctr = ioctl(info->fdt, TORUS_ALLOC_RX_COUNTER);
	ctr = ptorus_t::torus.alloc_rx_ctr();

	if (ctr < 0)
	{
	//eprintf("rx counter allocation failed\n");
		goto failure;
	}

	chan->ctr_id = ctr;

	ctr_grp = ctr / BGP_TORUS_COUNTERS;
	ctr_idx = ctr % BGP_TORUS_COUNTERS;

	info->dma[ctr_grp].rcv.counter_enable[ctr_idx / 32] =
		(0x80000000 >> (ctr_idx % 32));
	chan->rx.ctr = &info->dma[ctr_grp].rcv.counter[ctr_idx];

	//printf("Counter register address is %p\n",chan->rx.ctr);

	chan->rx.ctr->base = pbuf >> 4;
	chan->rx.ctr->counter = 0xffffffff;
	chan->rx.ctr->limit = (pbuf + chan->buf_size - 1) >> 4;

	chan->rx.received = 0;
#if 0
	printf("%s: allocated rx chan: (dma %p buf=%p/%Lx, size=%dK ctr=%d)\n", __PRETTY_FUNCTION__,
			chan->dma_space, chan->buf, pbuf, buf_size / 1024, ctr);
#endif
#endif
	return 0;

failure:
	if (chan->dma_space && --chan->dma_space->refcnt == 0)
		free(chan->dma_space);

	return -1;

}

int vm_t::khvmm_torus_alloc_tx(torus_client_info_t *info, torus_channel_t *chan, torus_dma_space_t *msp, size_t fifo_size, size_t buf_size)
{
    long long pfifo, pbuf;
    int i;
    int ctr, ctr_grp, ctr_idx;
    int fifo, fifo_grp, fifo_idx;


    if (((chan->dma_space = msp) == NULL) &&
        ((chan->dma_space = create_dma_space(fifo_size + buf_size)) == NULL))

    {
        printf("%s: rx DMA space allocation failed\n", __PRETTY_FUNCTION__);
        goto failure;
    }

    if ((chan->tx.fifo_desc = (torus_inj_desc_t *)mspace_malloc(chan->dma_space->msp, fifo_size)) == NULL)
    {
        printf("%s: rx DMA fifo allocation failed\n", __PRETTY_FUNCTION__);
        goto failure;
    } else {
    	chan->dma_space->refcnt++;
    }
    chan->dma_space->refcnt++;

    pfifo = (long long)chan->tx.fifo_desc;

    chan->buf = mspace_least_addr(chan->dma_space->msp);
    chan->buf_size = mspace_max_footprint(chan->dma_space->msp);
    pbuf  = (long long)chan->buf;

    chan->tx.fifo_size = fifo_size / sizeof(torus_inj_desc_t);
    chan->tx.fifo_idx = 0;

#if 1
    //ctr = ioctl(info->fdt, TORUS_ALLOC_TX_COUNTER);
    //fifo = ioctl(info->fdt, TORUS_ALLOC_TX_FIFO);
    fifo = ptorus_t::torus.alloc_tx_fifo();
    ctr = ptorus_t::torus.alloc_tx_ctr();

    if ((ctr < 0) || (fifo < 0))
    {
	//eprintf("%s: tx counter or fifo allocation failed\n", __PRETTY_FUNCTION__);
        goto failure;
    }

    chan->ctr_id = ctr;
    chan->fifo_id = fifo;

    ctr_grp = ctr / BGP_TORUS_COUNTERS;
    ctr_idx = ctr % BGP_TORUS_COUNTERS;

    fifo_grp = fifo / BGP_TORUS_INJ_FIFOS;
    fifo_idx = fifo % BGP_TORUS_INJ_FIFOS;


    chan->tx.fifo = &info->dma[fifo_grp].inj.fifo[fifo_idx];
    //init_fifo(chan->tx.fifo, pfifo, fifo_size);
    ptorus_t::torus.diable_inj_fifo(fifo_grp,fifo_idx);
    ptorus_t::torus.dma->init_inj_fifo(fifo,pfifo,fifo_size);
    ptorus_t::torus.enable_inj_fifo(fifo_grp,fifo_idx);

    chan->tx.fifo_start = chan->tx.fifo->start;
    info->dma[ctr_grp].inj.counter_enable[ctr_idx / 32] =
        (0x80000000 >> (ctr_idx % 32));

    info->dma[ctr_grp].inj.counter[ctr_idx].base = pbuf >> 4;
    info->dma[ctr_grp].inj.counter[ctr_idx].counter = 0xffffffff;

    info->dma[fifo_grp].inj.dma_activate = 0x80000000 >> fifo_idx;

    for (i = 0; i < chan->tx.fifo_size; i++)
    {
	torus_inj_desc_t *desc = &chan->tx.fifo_desc[i];
	memset(desc, 0, sizeof(*desc));
    }
#if 0
    printf("%s: allocated tx chan: (dma %p, fifo=%p/%Lx, buf=%p/%Lx size=%dK/%dK ctr=%d, fifo=%d\n", __PRETTY_FUNCTION__,
            chan->dma_space, chan->tx.fifo_desc, pfifo, chan->buf, pbuf, buf_size / 1024, fifo_size / 1024, ctr, fifo);
#endif
#endif
    return 0;

failure:
    if (chan->dma_space && --chan->dma_space->refcnt == 0)
        free(chan->dma_space);

    return -1;
}

torus_dma_space_t *vm_t::create_dma_space(size_t size)
{
	torus_dma_space_t *dma_space;

	//extern void *create_mspace(size_t capacity, int locked);

	if ((dma_space = (torus_dma_space_t *)malloc(sizeof(torus_dma_space_t))) == NULL)
		return NULL;

	if ((dma_space->msp = theMspace) == NULL)
	{
		free(dma_space);
		return NULL;
	}

	return dma_space;
}

memcached_return vm_t::memcached_khvmm_client_create(memcached_st *ptr)
{
    if ((ptr->torus_info = (torus_client_info_t *) malloc(sizeof(torus_client_info_t))) == NULL)
    {
        printf("failed to allocate torus info structure\n");
        return MEMCACHED_MEMORY_ALLOCATION_FAILURE;
    }

    memset(ptr->torus_info, 0, sizeof(torus_client_info_t));

    //dprintf(ptr->torus_info, 1, "%s: (%p)\n", __PRETTY_FUNCTION__, ptr);

    ptr->torus_info->getitem_size = DEFAULT_TORUS_GETITEM_SIZE;
    ptr->torus_info->settx_size =   DEFAULT_TORUS_SETTX_SIZE;
    ptr->torus_info->gettx_size =   DEFAULT_TORUS_GETTX_SIZE;

    return MEMCACHED_SUCCESS;
}
