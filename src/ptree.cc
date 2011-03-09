/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       ptree.cc
 * Description:     Physical BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/

#include "fpu.h"
#include "fdt.h"
#include "ptree.h"
#include "vm.h"

extern vm_t vm[NUM_VMS];

#undef DEBUG_PACKETS
//#define DEBUG_PACKETS

#define TREE_IRQMASK_INJ	(PIX_APAR0  | PIX_APAR1  | \
                                 PIX_ALIGN0 | PIX_ALIGN1 |\
                                 PIX_ADDR0  | PIX_ADDR1  |\
                                 PIX_DPAR0  | PIX_DPAR1  |\
                                 PIX_COLL   | PIX_UE     |\
                                 PIX_PFO0   | PIX_PFO1   |\
                                 PIX_HFO0   | PIX_HFO1)

#define TREE_IRQMASK_REC	(PRX_APAR0  | PRX_APAR1  |\
                                 PRX_ALIGN0 | PRX_ALIGN1 |\
                                 PRX_ADDR0  | PRX_ADDR1  |\
                                 PRX_COLL   | PRX_UE     |\
                                 PRX_PFU0   | PRX_PFU1   |\
                                 PRX_HFU0   | PRX_HFU1   |\
				 PRX_WM0    | PRX_WM1 )

void ptree_chan_t::set_inj_wm_irq (bool i)
{ 
    tree->dcr_write(tree_t::PIXEN, i 
                    ? tree->dcr_read(tree_t::PIXEN) | (tree_t::PIX_WM0 >> channel)
                    : tree->dcr_read(tree_t::PIXEN) & ~(tree_t::PIX_WM0 >> channel)); 
}

bool ptree_chan_t::send_packet (word_t head, fpu_word_t *data, lnk_hdr_t * const lhdr)
{
    status_t status = gpr_read (PSR);
    
    while (status.inj_hdr >= fifo_size) 
    {   
        printf("%s: send fifo FULL!\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    
#ifdef DEBUG_PACKETS
    printf ("tree channel %d send head: %#010lx\n", channel, head);
    for (unsigned i = 0; i < 16; i++)
        printf ("\tdata[%03u]: %#010lx %#010lx %#010lx %#010lx\n", i,
                data[i].x[0], data[i].x[1], data[i].x[2], data[i].x[3]);
#endif
    gpr_write (tree_chan_t::PIH, head);
    
    fpu_word_t old;
    fpu_t::read  (0, &old);
    fpr_write (tree_chan_t::PID, (lhdr ? &lhdr->raw : data++));
    for (unsigned i = 0; i < 15; i++)
        fpr_write (tree_chan_t::PID, data + i);
    fpu_t::write (0, &old);
    
    return true;
}

word_t ptree_chan_t::recv_packet()
{
    status_t status = gpr_read (PSR);
    word_t core_mask = 0;

    fpu_word_t saved_fprs[16];
    
    for (int i = 0; i < 16; i++)
        fpu_t::read(i, saved_fprs + i);

    bool vmchan_rcv_irq = false;
    
    while (status.rcv_hdr && status.rcv_pkt) 
    {
        // FIFO header
        header_t hdr = gpr_read(PRH);

        // FIFO Data
        for (int i = 0; i < 16; i++)
            fpr_read(PRD, NULL, i);

        // KDB Traffic
        lnk_hdr_t lhdr ALIGN(16);    
        fpu_t::read(0, &lhdr.raw);

#ifdef DEBUG_PACKETS
        printf ("tree channel %d receive head: %#010lx\n",  channel, hdr.raw);
        printf("\tstatus %lx <hdr %ld pkt %ld>\n",
               status.raw,  status.rcv_hdr,  status.rcv_pkt);
        printf("\tlink <proto %d dst %ld src %ld>\n",
               lhdr.lnk_proto,  lhdr.dst_key,  lhdr.src_key);
        for (unsigned i = 0; i < 16; i++)
        {
            fpu_word_t data;
            fpu_t::read(i, &data);
            printf ("\tdata[%03u]: %#010lx %#010lx %#010lx %#010lx\n", i,
                    data.x[0], data.x[1], data.x[2], data.x[3]);
        }
#endif

        if (channel == tree->l4dbgcon.tree_channel &&
            lhdr.lnk_proto == tree->l4dbgcon.proto_id &&
            lhdr.dst_key == tree->l4dbgcon.snd_id &&
            lhdr.src_key != tree->nodeid)
        {
            // thrash payload (hopefully an ESC/breakin anyway) 
            L4_KDB_Enter("u/l breakin");
        }
        else if (tree->vmchan.is_pkt(channel, lhdr))
        {
            vmchan_rcv_irq |= tree->vmchan.recv_packet(hdr, lhdr);
        }
        else if (lhdr.src_key != tree->nodeid)
        {
            if (channel == 0)
            	for (int i = 0; i < NUM_VMS; ++i)
            		core_mask |= vm[i].tree.chan0.recv_packet (hdr.raw);
            else
            	for (int i = 0; i < NUM_VMS; ++i)
            		core_mask |= vm[i].tree.chan1.recv_packet (hdr.raw);
        }
        
        status = gpr_read (PSR);
    }
    
    if (vmchan_rcv_irq)
        core_mask = tree->vmchan.raise_rcv_irq();

    
    for (int i = 0; i < 16; i++)
        fpu_t::write (i, saved_fprs);

    return core_mask;
}

word_t ptree_chan_t::handle_inj_wm_irq()
{
    // Currently implemented for vmchan only
    
     if (!tree->vmchan.is_inj_pending(channel)) 
     {
         printf("%s: unimplemented non-vmchan watermark irq\n", __PRETTY_FUNCTION__);
         unimplemented();
     }
     
     printf("%s: retransmit pending vmchan pkts", __PRETTY_FUNCTION__);
     L4_KDB_Enter("Untested");
     return tree->vmchan.handle_inj_pending();
}


bool ptree_t::handle_hwirq(word_t irq, word_t &core_mask)
{
    if (irq == inj_irq)
    {
        word_t pixf = dcr_read( PIXF );
        
        if (pixf & PIX_WM0)
            core_mask |= chan0.handle_inj_wm_irq();
        if (pixf & PIX_WM1)
            core_mask |= chan1.handle_inj_wm_irq();
    }
    else
    {
        word_t prxf = dcr_read( PRXF );
    
        while (prxf)
        {
            
            if (prxf & PRX_WM0)
                core_mask |= chan0.recv_packet();
            if (prxf & PRX_WM1)
                core_mask |= chan1.recv_packet();
            
            prxf = dcr_read( PRXF );
        }
    }
    return true;
}

void ptree_t::init(L4_ThreadId_t irq_handler)
{
    fdt_prop_t *prop;
    
    if (!fdt_t::fdt->find_head_node("/plb/tree"))
	goto error;

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tree/dcr-reg")))
	goto error;
    
    assert(DCRBASE == prop->u32(0));
    
    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tree/nodeid")) )
	goto error;
    nodeid = prop->u32(0);

    // For now, we rely on U-Boot's hardcccoded interrupt map
    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tree/interrupts")) )
	goto error;

    if (prop->length() != 24 * sizeof(word_t))
        goto error;
    
    for (word_t i=0; i<4; i++)
        assert(prop->u32(i) == irq_max - (3-i));
    for (word_t i=4; i<24; i++)
        assert(prop->u32(i) == irq_min + (i-4));

    for (word_t i=0; i<24; i++)
    {
        //if (!L4_AssociateInterrupt(L4_GlobalId (irq, 1), irq_handler))
        //  goto error;
    }

    // Attach to inject IRQ and receive IRQs
    assert(inj_irq == prop->u32(0));
    assert(rcv_irq == prop->u32(1));
    assert(rcv_irq_vc0 == prop->u32(2));
    assert(rcv_irq_vc1 == prop->u32(3));

    for (word_t i=0; i<4; i++)
    {
        if (!L4_AssociateInterrupt(L4_GlobalId (prop->u32(i), 1), irq_handler))
            goto error;
    }
    
    nodeaddr = dcr_read(NADDR);

    for (int i = 0; i < NUM_VMS; ++i)
    	vm[i].tree.set_nodeaddr(nodeaddr);

    printf("TREE: node id %06lx addr %lx irq <%d,%d,%d,%d>\n", 
           nodeid, nodeaddr,  inj_irq, rcv_irq, rcv_irq_vc0, rcv_irq_vc1);
    
   	
    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tree/reg")) )
	goto error;
   
    for (word_t chnidx = 0; chnidx < 2; chnidx++)
    {
	printf("\tchan %ld base %lx size %ld\n", chnidx, 
               (word_t) prop->u64(chnidx * 3), (word_t) prop->u32(chnidx * 3 + 2));
    }
    
    /* Set watermarks.  Currently receive watermarks fire the moment
     * one packet is in the queue (this should change in the future
     * using the signal bit to coalesce interrutps).  The inject
     * watermarks are set to half the queue length.  They get
     * activated when the inject fifos overrun and we need to back-log
     * packets.  The moment at least half the queue is empty the IRQ
     * fires and new packets can be injected.
     */
    dcr_write(VCFG0, VCFG_RCV_FIFO_WM(0) | VCFG_INJ_FIFO_WM(4));
    dcr_write(VCFG1, VCFG_RCV_FIFO_WM(0) | VCFG_INJ_FIFO_WM(4));
    
    dcr_write(PIXEN, TREE_IRQMASK_INJ);
    dcr_write(PRXEN, TREE_IRQMASK_REC);
      
    // clear exception flags
    dcr_write(PIXF, 0);
    dcr_write(PRXF, 0);
   
    
    if (!fdt_t::fdt->find_head_node("/l4"))
	goto error;

    if (! (prop = fdt_t::fdt->find_prop_node("/l4/dbgcon")) )
	goto error;
   
    {
	/* format: sndid,rcvid,dest */
	const char *dbgcon = prop->string();
	
	l4dbgcon.snd_id = strtoul(dbgcon, NULL, 10);
	l4dbgcon.rcv_id = strtoul(++dbgcon, NULL, 10);
	l4dbgcon.dest_node = strtoul(++dbgcon, NULL, 10);
    }

    if (!fdt_t::fdt->find_head_node("/plb/tty"))
	goto error;

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tty/tree-channel")) )
	goto error;
   
    l4dbgcon.tree_channel = prop->u32(0);
    
    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tty/tree-route")) )
	goto error;
    l4dbgcon.tree_route = prop->u32(0);

    if (! (prop = fdt_t::fdt->find_prop_node("/plb/tty/link-protocol")) )
	goto error;
    l4dbgcon.proto_id = prop->u32(0);

    printf("\tdbgcon snd: %ld rcv: %ld dst node %ld tree channnel %ld route %ld proto %ld\n", 
           l4dbgcon.snd_id,  l4dbgcon.rcv_id, l4dbgcon.dest_node,  l4dbgcon.tree_channel,
           l4dbgcon.tree_route,  l4dbgcon.proto_id);

    return;
    
error:
    
    assert(false);
}
