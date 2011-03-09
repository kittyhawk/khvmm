/*********************************************************************
 *
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       ptorus.cc
 * Description:     Physical BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/
#include "ptorus.h"
#include "fdt.h"
#include "ptree.h"
#include "vm.h"

#define DMA_BASE_CONTROL_INIT  ( DMA_BASE_CONTROL_USE_DMA    |  \
                                 DMA_BASE_CONTROL_L3BURST_EN |  \
                                 DMA_BASE_CONTROL_ITIU_EN    |  \
                                 DMA_BASE_CONTROL_RTIU_EN    |  \
                                 DMA_BASE_CONTROL_IMFU_EN    |  \
                                 DMA_BASE_CONTROL_RMFU_EN)
extern vm_t vm[NUM_VMS];

#define Tdprintf(x...)     

void ptorus_dma_t::enable_inj_counter_hitzero_irq(word_t counter, word_t vector)
{
    assert(counter < nr_counters);
    DmaDcr dcr = DmaDcr(iDMA_COUNTER_INT_ENABLE_GROUP0 + vector);
    torus->dmadcr_write(dcr, torus->dmadcr_read(dcr) | (0x80000000 >> inj_ctr_subgroup(counter)));
}

void ptorus_dma_t::enable_rcv_fifo_threshold_irq(word_t fifo, word_t type)
{
    assert(fifo < nr_rcv_fifos-1);
    DmaDcr dcr = DmaDcr(rDMA_FIFO_INT_ENABLE_TYPE0 + type);
    torus->dmadcr_write(dcr, torus->dmadcr_read(dcr) | (0x80000000 >> (rcv_fifo_normal_id(fifo))));
}


bool ptorus_t::handle_hwirq(const word_t irq, word_t &core_mask)
{
    set_pending(irq);
      
    if (!vmchan.is_enabled())
    	for (int i = 0; i < NUM_VMS; ++i)
    		return vm[i].torus.handle_hwirq(irq, core_mask);
            
    Tdprintf("TORUS: irq %ld (%#lx)\n", irq, irq);
    
    switch(irq)
    {
    case irq_cnt_thr_min:
    case irq_cnt_thr_max:
    case irq_dma_cnt_thr_min:
    case irq_dma_cnt_thr_max:
        /* Count thresholds, shouldn't fire */
        printf("%s: torus/dma fatal count threshold irq %ld\n", __PRETTY_FUNCTION__, irq);
        L4_KDB_Enter("torus error");
        break;
        
    case irq_inj_fifo_wm_min:
    case irq_inj_fifo_wm_max:
        /* Inj count thresholds, shouldn't fire */
        printf("%s: inj watermark irq %ld\n", __PRETTY_FUNCTION__, irq);
        L4_KDB_Enter("torus error");

    case irq_rcv_fifo_wm_min:
    case irq_rcv_fifo_wm_max:
        /* Rcv count thresholds, ignore */
        Tdprintf("%s: rcv watermark irq %ld\n", __PRETTY_FUNCTION__, irq);
        break;
    case irq_inj_fifo_thr_min:
    case irq_inj_fifo_thr_max:
        /* Inj count thresholds, shouldn't fire */
        printf("%s: inj thresholds irq %ld\n", __PRETTY_FUNCTION__, irq);
        L4_KDB_Enter("torus error");
        break;
    case irq_rcv_fifo_thr_min:
    case irq_rcv_fifo_thr_max:
    {
        Tdprintf("%s: rcv threshold irq %ld\n", __PRETTY_FUNCTION__, irq);
        
        word_t rfifo = vmchan.rcv_fifo_no;
        ptorus_dma_t *rdma = vmchan.dma;
        word_t nemask = (0x80000000 >> (rdma->rcv_fifo_id(rfifo) % 32));
        bool ret = true;
        
        Tdprintf("empty %lx\n", rdma->get_rcv_fifo_not_not_empty_mask(rfifo));
       
        // We currently use vmchan only
        while (rdma->get_rcv_fifo_not_not_empty_mask(rfifo))
        {
            Tdprintf("empty %lx\n", rdma->get_rcv_fifo_not_not_empty_mask(rfifo));
            assert(rdma->get_rcv_fifo_not_not_empty_mask(rfifo)  == nemask);
            ret &= vmchan.handle_rcv_thr_zero(core_mask);
        }
        
        return ret;        
    }
    break;
        
    case irq_inj_ctr_zero_min:
    case irq_inj_ctr_zero_max:
    { 
        Tdprintf("%s: inj ctr zero irq %ld\n", __PRETTY_FUNCTION__, irq);
               
        word_t ictr = vmchan.inj_counter_no;
        ptorus_dma_t *idma = vmchan.dma;
        
        // We currently use vmchan only
        assert(idma->get_inj_counter_group_status() == (0x80000000 >> (idma->inj_ctr_subgroup(ictr))));
        assert(idma->get_inj_counter_hitzero_mask((ictr + 32) % 32) == 0);

        return vmchan.handle_inj_ctr_zero(core_mask);
    }        
    break;
    case irq_rcv_ctr_zero_min:
    case irq_rcv_ctr_zero_max:
        printf("%s: rcv ctr zero irq %ld\n", __PRETTY_FUNCTION__, irq);
        unimplemented();
        break;
    case irq_link_check_pkt:
        printf("%s: link check pkt irq %ld\n", __PRETTY_FUNCTION__, irq);
        L4_KDB_Enter("torus error");
        break;
    case irq_fatal_err:
    case irq_dma_fatal_err:
    case irq_rcv_fifo_inv_rd_min:
    case irq_rcv_fifo_inv_rd_max:
    case irq_inj_ctr_err_min:
    case irq_inj_ctr_err_max:
    case irq_rcv_ctr_err_min:
    case irq_rcv_ctr_err_max:
    case irq_dma_write_err_min:
    case irq_dma_write_err_max:     
    case irq_dd2_err_min:
    case irq_dd2_err_max:
        /* Errors */
        printf("%s: error irq %ld\n", __PRETTY_FUNCTION__, irq);
#ifndef NODEBUG
        dump_dcrs();
        dma[vmchan.group].dump_inj_channel();
        dma[vmchan.group].dump_rcv_channel();
#endif
        L4_KDB_Enter("torus error");
    };

    return false;
}

void ptorus_t::ack_hwirq(word_t irq)
{
    assert(is_pending(irq));
    
    clear_pending(irq);
    //Tdprintf("TORUS: ack irq %ld (%#lx)\na", irq, irq);
    
    L4_Set_MsgTag( L4_Niltag );
    L4_MsgTag_t tag = L4_Reply (L4_GlobalId (irq, 1));

    assert(L4_IpcSucceeded (tag)); 
}

void ptorus_t::dma_reset()
{
    /* pull and release reset */
    dmadcr_write(DMA_RESET, 0xffffffff);
    
    L4_Sleep (L4_TimePeriod (50*1000));
    dmadcr_write(DMA_RESET, 0);
    L4_Sleep (L4_TimePeriod (50*1000));

    /* enable DMA use */
    dmadcr_write(DMA_BASE_CONTROL, DMA_BASE_CONTROL_USE_DMA);

    /* clear rest */
    for (word_t i = 0; i < nr_dma_regions; i++) {
	set_dma_rcv_region(i, 1, 0);
	set_dma_inj_region(i, 1, 0);
    }

    /* set injection fifo watermarks */
    dmadcr_write(DmaDcr(iDMA_TS_FIFO_WM0+0), 0x14141414);
    dmadcr_write(DmaDcr(iDMA_TS_FIFO_WM0+1), 0x14141414);

    /* set reception fifo watermarks */
    for (word_t i = 0; i < nr_dma_groups; i++)
	dmadcr_write(DmaDcr(rDMA_TS_FIFO_WM0+i), 0);

    /* set local fifo watermark */
    dmadcr_write(iDMA_LOCAL_FIFO_WM_RPT_CNT_DELAY, 0x37000000);
    dmadcr_write(rDMA_LOCAL_FIFO_WM_RPT_CNT_DELAY, 0);

    dmadcr_write(iDMA_SERVICE_QUANTA, 0);

    dmadcr_write(DmaDcr(rDMA_FIFO_CLEAR_MASK0+0),0xFF000000);
    dmadcr_write(DmaDcr(rDMA_FIFO_CLEAR_MASK0+1),0x00FF0000);
    dmadcr_write(DmaDcr(rDMA_FIFO_CLEAR_MASK0+2),0x0000FF00);
    dmadcr_write(DmaDcr(rDMA_FIFO_CLEAR_MASK0+3),0x000000FF);
    dmadcr_write(rDMA_FIFO_HEADER_CLEAR_MASK,0x08040201);

    /* disable all counters, init all fifos so that nothing bad can happen... */
    for (word_t group = 0; group < nr_dma_groups; group++) {
        torus.dma[group].reset();

	for (word_t i = 0; i < torus_dma_t::nr_inj_fifos; i++) 
	    reset_inj_fifo(group, i);

    }

    /* clear error count dcrs */
    for (word_t i = 0; i < 9; i++)
	dmadcr_write(DmaDcr(DMA_CE_COUNT0+i), 0);

    /* set all the error count threshold and enable all interrupts */
    dmadcr_write(DMA_CE_COUNT_THRESHOLD, 1);

    /* clear the errors associate with the UE's on initial sram writes */
    dmadcr_write(DMA_CLEAR0, ~0U);
    dmadcr_write(DMA_CLEAR1, ~0U);

    for (word_t i = 0; i < nr_dma_groups; i++)
	dmadcr_write(DmaDcr(DMA_FATAL_ERROR_ENABLE0+i), ~0ul);

    /* enable the state machines (enable DMA, L3Burst, inj, rcv, imfu, rmfu) */
    dmadcr_write(DMA_BASE_CONTROL, DMA_BASE_CONTROL_INIT);
    
    isync();
  
}

void ptorus_t::vmchan_t::enable(resource_t &ram)
{
    if (enabled)
        return;
    
    enabled = true;
    
    assert(torus);
    assert(dma == &torus->dma[group]);
    
    torus->dma_reset();
    
    torus->config_inj_fifo(group, inj_fifo_no, fifomap, 1, 0);
    
    /* set DMA ranges to guest ram */
    torus->set_dma_inj_region(0, ram.hpa_base, ram.hpa_base + ram.size);
    torus->set_dma_rcv_region(0, ram.hpa_base, ram.hpa_base + ram.size);

    /* add DMA ranges for descriptors */
    torus->set_kvmm_dma_inj_region((paddr_t) inj_fifo, (paddr_t) (inj_fifo + inj_fifo_size));
    torus->set_kvmm_dma_rcv_region((paddr_t) rcv_fifo, (paddr_t) (rcv_fifo + rcv_fifo_size));

    /* Set all pids to point at the KHVMM reception fifo. */
    for (word_t p = 0; p < nr_pids; p++)
	torus->set_rcv_fifo_map(p, group, rcv_fifo_no);
    
    
    // Init KHVMM inj and rcv queue
    memset( (void *) inj_fifo, 0, inj_fifo_size * sizeof(torus_dma_t::inj_desc_t));
    memset( (void *) rcv_fifo, 0, rcv_fifo_size * sizeof(torus_dma_t::rcv_desc_t));

    dma->init_inj_fifo(inj_fifo_no, (paddr_t) inj_fifo, 
                                    inj_fifo_size * sizeof(torus_dma_t::inj_desc_t));
    dma->init_rcv_fifo(rcv_fifo_no, (paddr_t) rcv_fifo, 
                                    rcv_fifo_size * sizeof(torus_dma_t::rcv_desc_t));

    torus->enable_inj_fifo(group, inj_fifo_no);
    dma->enable_inj_counter(inj_counter_no, ram.hpa_base);
    dma->enable_inj_counter_hitzero_irq(inj_counter_no);
    dma->activate_inj_dma(inj_fifo_no);
    
    torus->enable_rcv_fifo(group, rcv_fifo_no);
    torus->set_rcv_fifo_threshold(((rcv_fifo_size-1) * 256) / 16);
    dma->enable_rcv_fifo_threshold_irq(rcv_fifo_no);

    return;
}

bool ptorus_t::vmchan_t::insert_injdesc(paddr_t data, size_t size, coordinates_t dst, dma_sw_src_key_t src_key, vdp83820_t::desc_t *dpdesc)
{
    Tdprintf("\tinj_fifo_add %ld inj_fifo_rem %ld inj_idx %ld inj_fifo_cnt %d\n",
             inj_fifo_add, inj_fifo_rem, (inj_fifo_add % inj_fifo_size), inj_fifo_cnt());
    
    if (inj_fifo_cnt() == inj_fifo_size)
    {
        printf ("%s: inj FIFO full\n", __PRETTY_FUNCTION__);
        unimplemented();
        return false;
    }
    word_t inj_idx = inj_fifo_add++ % inj_fifo_size; 
   
    ptorus_dma_t::inj_desc_t *desc = &inj_fifo[inj_idx]; 
    
    assert(inj_descs[inj_idx] == NULL);
    inj_descs[inj_idx] = dpdesc;
    
    desc->fifo.set(pid, dst);
    desc->dma_hw.set(dma->inj_ctr_id(inj_counter_no), data, size); 
    desc->dma_sw.src_key = src_key;
    
    sync();

    dma->inc_inj_counter(inj_counter_no, size);
    dma->set_inj_fifo_tail(inj_fifo_no, (paddr_t) &inj_fifo[inj_fifo_add % inj_fifo_size]); 

    return true;
}

bool ptorus_t::vmchan_t::handle_inj_ctr_zero(word_t &core_mask)
{
    core_mask = 0;
    
    // Find out which packets are done
    word_t inj_head_idx = (word_t)  (dma->get_inj_fifo_head(inj_fifo_no) - (paddr_t) inj_fifo) / sizeof(ptorus_dma_t::inj_desc_t);

    Tdprintf("tx done until head %ld inj_fifo_rem %ld\n", inj_head_idx, inj_fifo_rem);

    bool send_irq = false;
     
    for (word_t idx = (inj_fifo_rem % inj_fifo_size); idx != inj_head_idx; inj_fifo_rem++, idx = (inj_fifo_add % inj_fifo_size))
    {

        if (idx == inj_head_idx)
            break;

        assert(inj_descs[idx] != NULL);
        if (inj_descs[idx]->cmd_status.tx.intr)
            send_irq = true;
        
        inj_descs[idx]->cmd_status.tx.own = false;
        inj_descs[idx]->cmd_status.tx.ok = true;
        inj_descs[idx] = NULL;

        Tdprintf("tx done %ld inj_fifo_rem %ld\n", idx, inj_fifo_rem);
        
    }
    dma->clear_inj_counter_hitzero_mask(inj_counter_no);
    
    if (send_irq)
        core_mask = vnic->raise_send_irq();
    
    return true;

}

bool ptorus_t::vmchan_t::handle_rcv_thr_zero(word_t &core_mask)
{
    core_mask = 0;
    
    dma->clear_rcv_threshold_mask(rcv_fifo_no);
    
    // Find out which packets are done
    word_t rcv_tail_idx = (word_t) (dma->get_rcv_fifo_tail(rcv_fifo_no) - (paddr_t) rcv_fifo) / sizeof(ptorus_dma_t::rcv_desc_t);
    bool rcv_irq = false;
    
    Tdprintf("rx done from head %ld until tail %ld\n", (rcv_fifo_add % rcv_fifo_size), rcv_tail_idx);
    assert((rcv_fifo_add % rcv_fifo_size) != rcv_tail_idx);
    
    for (word_t idx = (rcv_fifo_add % rcv_fifo_size); idx != rcv_tail_idx; rcv_fifo_add++, idx = (rcv_fifo_add % rcv_fifo_size))
    {
        rcv_desc_t *rdesc = &rcv_fifo[idx];
        
        Tdprintf("%s got packet from %03d network %03d pkt %d  counter %d \n",  __PRETTY_FUNCTION__,
               rdesc->dma_sw.src_key.node, rdesc->dma_sw.src_key.network, rdesc->dma_sw.src_key.pkt, rdesc->dma_sw.offset);

        if (vnic->is_vdp83820_pkt(rdesc->dma_sw.src_key))
            rcv_irq |= vnic->receive_pkt(rdesc->dma_sw.src_key, rdesc->data, rdesc->fifo.get_size(), rdesc->dma_sw.offset);
    }
    
    // Advance start  
    dma->set_rcv_fifo_head(rcv_fifo_no, (paddr_t) &rcv_fifo[rcv_tail_idx]);
   
    if (rcv_irq)
        core_mask = vnic->raise_rcv_irq();
    
    return true;

}



void ptorus_t::init(L4_ThreadId_t irq_handler)
{
    fdt_prop_t *prop;
    word_t interrupt_count;
    
    if (!fdt_t::fdt->find_head_node("/plb/torus"))
	goto error;

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/torus/dcr-reg")))
	goto error;

    assert(DCRBASE == prop->u32(0));
    
    if (! (prop = fdt_t::fdt->find_prop_node("/plb/torus/coordinates")))
	goto error;

    for (word_t i = 0; i < 3; i++)
        coordinates[i] = prop->u32(i);

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/torus/dimension")))
	goto error;

    for (word_t i = 0; i < 3; i++)
        dimension[i] = prop->u32(i);

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/torus/interrupts")) )
	goto error;

    // For now, we rely on U-Boot's hardcccoded interrupt map
    interrupt_count = prop->length() / sizeof(word_t);
    for (word_t i = 0; i < interrupt_count; i++)
    {
        word_t irq = prop->u32(i);
        assert(irq >= irq_min && irq <= irq_max);
        if (!L4_AssociateInterrupt(L4_GlobalId (irq, 1), irq_handler))
            goto error;
    }

    printf("TORUS: coord <%01d,%01d,%01d> dim <%01d,%01d,%01d>\n",
	   coordinates[0], coordinates[1], coordinates[2], dimension[0], dimension[1], dimension[2]);
    
    return;
error:
   assert(false);

}

#ifndef NODEBUG
#define DUMP_DCR(desc, x) printf("Torus: " desc "(%x): %08lx\n", x, mfdcrx(x))
void ptorus_t::dump_dcrs()
{
    int idx;

    DUMP_DCR("torus reset", 0xc80);
    DUMP_DCR("number nodes", 0xc81);
    DUMP_DCR("node coordinates", 0xc82);
    DUMP_DCR("neighbour plus", 0xc83);
    DUMP_DCR("neighbour minus", 0xc84);
    DUMP_DCR("cutoff plus", 0xc85);
    DUMP_DCR("cutoff minus", 0xc86);
    DUMP_DCR("VC threshold", 0xc87);
    DUMP_DCR("torus loopback", 0xc92);
    DUMP_DCR("torus non-rec err", 0xcdc);
    DUMP_DCR("dma reset", 0xd00);
    DUMP_DCR("base ctrl", 0xd01);
    for (idx = 0; idx < 8; idx++) {
	DUMP_DCR("inj min valid", 0xd02 + idx * 2);
	DUMP_DCR("inj max valid", 0xd03 + idx * 2);
	DUMP_DCR("rec min valid", 0xd14 + idx * 2);
	DUMP_DCR("rec max valid", 0xd15 + idx * 2);
    }
    DUMP_DCR("inj fifo enable", 0xd2c);
    DUMP_DCR("inj fifo enable", 0xd2d);
    DUMP_DCR("inj fifo enable", 0xd2e);
    DUMP_DCR("inj fifo enable", 0xd2f);
    DUMP_DCR("rec fifo enable", 0xd30);
    DUMP_DCR("rec hdr fifo enable", 0xd31);
    DUMP_DCR("inj fifo prio", 0xd32);
    DUMP_DCR("inj fifo prio", 0xd33);
    DUMP_DCR("inj fifo prio", 0xd34);
    DUMP_DCR("inj fifo prio", 0xd35);
    DUMP_DCR("remote get inj fifo threshold", 0xd36);
    DUMP_DCR("remote get inj service quanta", 0xd37);
    DUMP_DCR("rec fifo type", 0xd38);
    DUMP_DCR("rec header fifo type", 0xd39);
    DUMP_DCR("threshold rec type 0", 0xd3a);
    DUMP_DCR("threshold rec type 1", 0xd3b);
    for (idx = 0; idx < 32; idx++)
	DUMP_DCR("inj fifo map", 0xd3c + idx);
    DUMP_DCR("rec fifo map 00", 0xd60);
    DUMP_DCR("rec fifo map 00", 0xd61);
    DUMP_DCR("rec fifo map 01", 0xd62);
    DUMP_DCR("rec fifo map 01", 0xd63);
    DUMP_DCR("rec fifo map 10", 0xd64);
    DUMP_DCR("rec fifo map 10", 0xd65);
    DUMP_DCR("rec fifo map 11", 0xd66);
    DUMP_DCR("rec fifo map 11", 0xd67);
    DUMP_DCR("inj fifo irq enable grp0", 0xd6d);
    DUMP_DCR("inj fifo irq enable grp1", 0xd6e);
    DUMP_DCR("inj fifo irq enable grp2", 0xd6f);
    DUMP_DCR("inj fifo irq enable grp3", 0xd70);
    DUMP_DCR("rec fifo irq enable type0", 0xd71);
    DUMP_DCR("rec fifo irq enable type1", 0xd72);
    DUMP_DCR("rec fifo irq enable type2", 0xd73);
    DUMP_DCR("rec fifo irq enable type3", 0xd74);
    DUMP_DCR("rec hdr irq enable", 0xd75);
    DUMP_DCR("inj cntr irq enable", 0xd76);
    DUMP_DCR("inj cntr irq enable", 0xd77);
    DUMP_DCR("inj cntr irq enable", 0xd78);
    DUMP_DCR("inj cntr irq enable", 0xd79);
    DUMP_DCR("rec cntr irq enable", 0xd7a);
    DUMP_DCR("rec cntr irq enable", 0xd7b);
    DUMP_DCR("rec cntr irq enable", 0xd7c);
    DUMP_DCR("rec cntr irq enable", 0xd7d);
    DUMP_DCR("fatal error enable", 0xd7e);
    DUMP_DCR("ce count inj fifo0", 0xd8a);
    DUMP_DCR("ce count inj fifo1", 0xd8b);
    DUMP_DCR("ce count inj counter", 0xd8c);
    DUMP_DCR("ce count inj desc", 0xd8d);
    DUMP_DCR("ce count rec fifo0", 0xd8e);
    DUMP_DCR("ce count rec fifo1", 0xd8f);
    DUMP_DCR("ce count rec counter", 0xd90);
    DUMP_DCR("ce count local fifo0", 0xd91);
    DUMP_DCR("ce count local fifo1", 0xd92);
    DUMP_DCR("fatal error 0", 0xd93);
    DUMP_DCR("fatal error 1", 0xd94);
    DUMP_DCR("fatal error 2", 0xd95);
    DUMP_DCR("fatal error 3", 0xd96);
    DUMP_DCR("wr0 bad address", 0xd97);
    DUMP_DCR("rd0 bad address", 0xd98);
    DUMP_DCR("wr1 bad address", 0xd99);
    DUMP_DCR("rd1 bad address", 0xd9a);
    DUMP_DCR("imfu err 0", 0xd9b);
    DUMP_DCR("imfu err 1", 0xd9c);
    DUMP_DCR("rmfu err", 0xd9d);
    DUMP_DCR("rd out-of-range", 0xd9e);
    DUMP_DCR("wr out-of-range", 0xd9f);
    DUMP_DCR("dbg dma warn", 0xdaf);
}

void ptorus_dma_t::dump_inj_channel(word_t ch, word_t ctr)
{
    word_t i;
    word_t chmin = (ch == nr_inj_fifos) ? 0 : ch;
    word_t chmax = (ch == nr_inj_fifos) ? nr_inj_fifos : ch + 1;
    word_t ctmin = (ctr == nr_counters) ? 0 : ctr;
    word_t ctmax = (ctr == nr_counters) ? nr_counters : ctr+1;
    for (i = chmin; i < chmax; i++) 
    {
	printf("inj fifo %ld: start=%08lx, end=%08lx, head=%08lx, tail=%08lx\n",
	       i, 
               gpr_read(INJ_FIFO_REG(i, FIFO_START)), 
               gpr_read(INJ_FIFO_REG(i, FIFO_END)), 
               gpr_read(INJ_FIFO_REG(i, FIFO_HEAD)), 
               gpr_read(INJ_FIFO_REG(i, FIFO_TAIL)));
    }
    printf("counter enabled: %08lx, %08lx\n",
           gpr_read(INJ_CNTR_CNF_REG(0, CNF_ENABLED)),
           gpr_read(INJ_CNTR_CNF_REG(32, CNF_ENABLED)));

    printf("counter hit zero: %08lx, %08lx\n", gpr_read(INJ_CNTR_CNF_REG(0, CNF_HITZERO)),
           gpr_read(INJ_CNTR_CNF_REG(32, CNF_HITZERO)));

    for (i = ctmin; i < ctmax; i++) {
	printf("  ctr %02ld: count=%08lx, base=%08lx, enabled=%ld, hitzero=%ld\n",
               i, 
               gpr_read(INJ_CTR_REG(i, CTR_CTR)),
               gpr_read(INJ_CTR_REG(i, CTR_BASE)),
               ((gpr_read(INJ_CNTR_CNF_REG(i, CNF_ENABLED)) >> (31-i%32)) & 1),
               ((gpr_read(INJ_CNTR_CNF_REG(i, CNF_HITZERO)) >> (31-i%32)) & 1));
    }
}

void ptorus_dma_t::dump_rcv_channel(word_t ch, word_t ctr)
{
    word_t i;
    word_t chmin = (ch == nr_rcv_fifos) ? 0 : ch;
    word_t chmax = (ch == nr_rcv_fifos) ? nr_rcv_fifos : ch + 1;
    word_t ctmin = (ctr == nr_counters) ? 0 : ctr;
    word_t ctmax = (ctr == nr_counters) ? nr_counters : ctr+1;

    for (i = chmin; i < chmax; i++) 
    {
	printf("rcv fifo %ld: start=%08lx, end=%08lx, head=%08lx, tail=%08lx\n",
	       i, 
               gpr_read(RCV_FIFO_REG(i, FIFO_START)), 
               gpr_read(RCV_FIFO_REG(i, FIFO_END)), 
               gpr_read(RCV_FIFO_REG(i, FIFO_HEAD)), 
               gpr_read(RCV_FIFO_REG(i, FIFO_TAIL)));
    }

    printf("counter enabled: %08lx, %08lx\n",
           gpr_read(RCV_CNTR_CNF_REG(0, CNF_ENABLED)),
           gpr_read(RCV_CNTR_CNF_REG(32, CNF_ENABLED)));

    printf("counter hit zero: %08lx, %08lx\n",
           gpr_read(RCV_CNTR_CNF_REG(0, CNF_HITZERO)),
           gpr_read(RCV_CNTR_CNF_REG(32, CNF_HITZERO)));

    for (i = ctmin; i < ctmax; i++) {
	printf("  ctr %02ld: count=%08lx, base=%08lx, enabled=%ld, hitzero=%ld\n",
               i, 
               gpr_read(RCV_CTR_REG(i, CTR_CTR)),
               gpr_read(RCV_CTR_REG(i, CTR_BASE)),
               ((gpr_read(RCV_CNTR_CNF_REG(i, CNF_ENABLED)) >> (31-i%32)) & 1),
               ((gpr_read(RCV_CNTR_CNF_REG(i, CNF_HITZERO)) >> (31-i%32)) & 1));
    }
}

#endif /* NODEBUG */
