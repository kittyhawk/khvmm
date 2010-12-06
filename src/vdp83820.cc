/*********************************************************************
 *                
 * Copyright (C) 2005, 2008,  University of Karlsruhe
 * Copyright (C) 2008         Jan Stoess IBM Corporation
 *                
 * File path:     vdp83820.cc
 * Description:   Virtual NS 83820 Controller
 *                
 * @LICENSE@
 *                
 * $Id:$
 *                
 ********************************************************************/
static const bool debug_memio=0;
static const bool debug_regio=0;

#include "vm.h"
#include "vdp83820.h"
#include "personality.h"
#include "ptree.h"
#include "ptorus.h"

#define Nregdprintf(x...)  printf(x)
#define Ndprintf(x...)     printf(x)

//#define Nregdprintf(x...)
//#define Ndprintf(x...)

#undef DEBUG_PACKETS

bool vdp83820_t::gpdesc_to_mdesc(paddr_t gpdesc, desc_t* &mdesc) const
{
    word_t mpa;
    
    if (!vm->ram.gpa_to_map(gpdesc, mpa))
    {
        printf("%s: guest writes bogus GPA into descriptor\n", __PRETTY_FUNCTION__);
        return false;
    }
    
    mdesc = reinterpret_cast<vdp83820_t::desc_t *>(mpa);
    return true;
}


void vdp83820_t::reset()
{
    memset( (void *) regs, 0, sizeof(regs) );
    
    cfg.mode_1000 = 1;
    cfg.full_duplex_status = 0; // Reverse polarity.
    cfg.link_status = 1;
    cfg.speed_status = 1; // Reverse polarity.
    cfg.big_endian = 1; // Big endian
    cfg.ext_hwhdr = 1; // Extended HW hdr (+16 byte)
    
    rfdr.raw = 0;
    mear = 0x12;
    isr.raw = 0x00608000;
    txcfg = 0x120;
    rxcfg = 0x4;
//  brar 0xffffffff;
    srr  = 0x0103;
//  mibc = 0x2;
    tesr = 0xc000;
    srr  = 0x0004;
    
}

void vdp83820_t::reset_hwaddr()
{
    
    u32_t psetsize =0;
    bgp_uci_computecard_t uci;
    
    fdt_prop_t *prop;
    
    if (! (prop = fdt_t::fdt->find_prop_node ("/u-boot-env/bgp_uci")))
        goto error;
    
    uci = strtoul(prop->string(), NULL, 16);
    
    /* we use the locally admined etherenet range Z2:XX:YY specifically we use 02:XX:YY */
    /* where XX and YY are generated from row,column, midplane, nodecard and a bit from the compute card */
    /* to distinguish each group of 16 nodes */
    hwaddr[1] = uci.rackrow << 4 | uci.rackcolumn;
    hwaddr[2] = uci.midplane << 5 | uci.nodecard << 1;
    
    if (uci.computecard >= 4) {
        hwaddr[2] |= (((uci.computecard - 4) & 0x10) >> 4);
    } else {
        hwaddr[2] |= (uci.computecard & 0x1);
    }

    if (! (prop = fdt_t::fdt->find_prop_node ("/u-boot-env/bgp_psetsize")))
        goto error;
    
    psetsize = prop->u32(0);

    /* mask out lower bits so that all NICs in a pset have the same prefix */
    hwaddr[2] &= ~((psetsize / 16)- 1);

   
    hwaddr[0] = 0x06;
    hwaddr[3] = ptorus_t::torus.get_coordinate(ptorus_t::XAX);
    hwaddr[4] = ptorus_t::torus.get_coordinate(ptorus_t::YAX);
    hwaddr[5] = ptorus_t::torus.get_coordinate(ptorus_t::ZAX);

    
    Ndprintf("%s: hwaddr <%02x:%02x:%02x:%02x:%02x:%02x>\n", __PRETTY_FUNCTION__,
           hwaddr[0],hwaddr[1],hwaddr[2],hwaddr[3],hwaddr[4],hwaddr[5]);
    
    return;
    
error:
    assert(false);
}

word_t vdp83820_t::raise_irq() 
{
    word_t core_mask = 0;

    if(isr.raw & imr && ier)
    {
        Ndprintf("%s: raise irq %d isr %x\n", __PRETTY_FUNCTION__, irq, isr.raw);
        core_mask |= vm->bic.set_irq_status (irq);
    }
    return core_mask;
}
 
vdp83820_t::desc_t *vdp83820_t::find_rxdp(KeyType key_type, word_t key, word_t &mdata)
{
    
    paddr_t gpdesc = get_rxdp();
    desc_t *desc = NULL;
    
    for (;;)
    {
        if (gpdesc == 0)
            return NULL;
        
        if (!gpdesc_to_mdesc(gpdesc, desc))
            unimplemented();
        
        Ndprintf("\trdesc %lx (%lx) [%x:%x %x:%x]\n",
                 (word_t) desc, (word_t)gpdesc, desc->raw32[1], desc->raw32[0], desc->raw32[3], desc->raw32[2]);
        Ndprintf("\tstatus %x %x [sz %d %c%c%c%c%c]\n",
                 desc->raw32[4], desc->raw32[5], desc->cmd_status.rx.size,
                 (desc->flags.used        ? 'U' : 'u'),
                 (desc->cmd_status.rx.own  ? 'O' : 'o'),
                 (desc->cmd_status.rx.ok   ? 'K' : 'k'),
                 (desc->cmd_status.rx.more ? 'M' : 'm'),
                 (desc->cmd_status.rx.intr ? 'I' : 'i'));
        
        if (!vm->ram.gpa_to_map(desc->get_bufptr(), mdata))
        {
            printf("%s: invalid data ptr (map) [%x:%x]\n", __PRETTY_FUNCTION__, desc->raw32[3], desc->raw32[2]);
            unimplemented();
        }
        
        if (desc->cmd_status.rx.own == true)
            return NULL;
        
        
        // Assume there is room for the extended  header
        ext_hwhdr_t *ehdr = (ext_hwhdr_t *) mdata;

        if (desc->flags.used == false)
        {
            //  Descriptor is empty 
            desc->flags.used =  true;
            ehdr->first_pkt = false;
            ehdr->key_type = key_type;
            ehdr->key = key;
            ehdr->len = 0;
            ehdr->total_len = 0; 
           
            desc->extended_status.ip_pkt = 1; // Imply that checksum is complete.
            
            break;
        }
        else
        {
            // Descriptor already (partially) filled, search for dest key
            if ((KeyType) ehdr->key_type == key_type && ehdr->key == key)
                break;
            
            // Follow-up packet, store src_key at right position in rx descriptor (link header dst key)
            Ndprintf("\tdesc not empty src key %lx packet key %lx more %d len %ld total %ld\n", 
                     ehdr->key, key, desc->cmd_status.rx.more, ehdr->len, ehdr->total_len);
            gpdesc = desc->get_linkptr();
            continue;
        }
    }
    
    assert(desc);
    return desc;
}

word_t vdp83820_t::release_complete_rxdps()
{
    Ndprintf ("%s: rxdp %#lx crdd %d waiten %d\n",  __PRETTY_FUNCTION__, 
              (word_t) get_rxdp (), crdd, rxwaiten);
    
    desc_t *desc = NULL;
    paddr_t gpdesc;
    
    if (rxwaiten)
        return false;
    
    if (!crdd)
    {
        gpdesc = get_rxdp();

        if (gpdesc == 0)
            return false;        

        if (!gpdesc_to_mdesc(gpdesc, desc))
            unimplemented();
    }    
    else
    {
        gpdesc = rxdesccache->get_linkptr();
        printf("%s: gpdesc refresh %lx link %lx\n", __PRETTY_FUNCTION__, (word_t) rxdesccache, (word_t) gpdesc);
        
        if (crdd = (gpdesc == 0))
        { 
            printf("%s: gpdesc refresh null, go idle\n", __PRETTY_FUNCTION__);
            rxwaiten = true;
            isr.rx_idle = true;
            return raise_irq();
        }
        
        if (!gpdesc_to_mdesc(gpdesc, desc))
            unimplemented();
         
        // Advance rxdp
        rxdp_lo = desc->link_lo;
        rxdp_hi = desc->link_hi;

    }
    
    bool rcv_irq = false;
    
    for (;;)
    {
        word_t mdata;

        if (desc->cmd_status.rx.own == true)
        {
            printf("%s: RXDP own desc %lx (%lx) [%x:%x %x:%x]\n", __PRETTY_FUNCTION__,
                   (word_t) desc, (word_t)gpdesc, desc->raw32[1], desc->raw32[0], desc->raw32[3], desc->raw32[2]);
            printf("\tstatus %x %x [sz %d %c%c%c%c%c]\n",
                   desc->raw32[4], desc->raw32[5], desc->cmd_status.rx.size,
                   (desc->flags.used        ? 'U' : 'u'),
                   (desc->cmd_status.rx.own  ? 'O' : 'o'),
                   (desc->cmd_status.rx.ok   ? 'K' : 'k'),
                   (desc->cmd_status.rx.more ? 'M' : 'm'),
                   (desc->cmd_status.rx.intr ? 'I' : 'i'));
            
            isr.rx_idle = true;
            rxwaiten = true;
            rcv_irq = true;
            break;
        }
            
        if (!vm->ram.gpa_to_map(desc->get_bufptr(), mdata))
        {
            printf("%s: invalid data ptr (map) [%x:%x]\n", __PRETTY_FUNCTION__, desc->raw32[3], desc->raw32[2]);
            unimplemented();
        }

        ext_hwhdr_t *ehdr = (ext_hwhdr_t *) mdata;
        
        if (!desc->flags.used || !ehdr->first_pkt || ehdr->len != ehdr->total_len)
            break;

        desc->flags.used = false;
        
        desc->cmd_status.rx.own = true;
        desc->cmd_status.rx.ok = true;
        desc->cmd_status.rx.more = false;
        desc->cmd_status.rx.size = ehdr->total_len + EXT_HWHDR_SIZE;
        
        isr.rx_desc |= desc->cmd_status.rx.intr;
        rcv_irq |= desc->cmd_status.rx.intr;
       
        Ndprintf("\tcomplete packet received: %c pkt %lx len %d gpdesc %#lx\n", 
                 (ehdr->key_type == KeyTorus ? 'O' : 'R'), ehdr->key, desc->cmd_status.rx.size, (word_t) gpdesc);
        
#ifdef DEBUG_PACKETS
        for (unsigned i=0; i< ROUND_UP(ehdr->total_len, 32); i+=32)
        {
            word_t *dbgdata = (word_t *) (mdata + EXT_HWHDR_SIZE);
            printf("\t%08lx, %08lx, %08lx, %08lx, %08lx, %08lx, %08lx, %08lx\n", 
                   (dbgdata)[(i/4)+0], (dbgdata)[(i/4)+1], (dbgdata)[(i/4)+2], (dbgdata)[(i/4)+3],
                   (dbgdata)[(i/4)+4], (dbgdata)[(i/4)+5], (dbgdata)[(i/4)+6], (dbgdata)[(i/4)+7]);
        }
#endif        
        // Check for more completed packets
        gpdesc = desc->get_linkptr();
        
        if (crdd = (gpdesc == 0))
        { 
            rxdesccache = desc;
            printf("%s: RXDP null %lx (%lx) [%x:%x %x:%x]\n", __PRETTY_FUNCTION__,
                   (word_t) desc, (word_t)gpdesc, desc->raw32[1], desc->raw32[0], desc->raw32[3], desc->raw32[2]);
            printf("\tstatus %x %x [sz %d %c%c%c%c%c]\n",
                   desc->raw32[4], desc->raw32[5], desc->cmd_status.rx.size,
                   (desc->flags.used        ? 'U' : 'u'),
                   (desc->cmd_status.rx.own  ? 'O' : 'o'),
                   (desc->cmd_status.rx.ok   ? 'K' : 'k'),
                   (desc->cmd_status.rx.more ? 'M' : 'm'),
                   (desc->cmd_status.rx.intr ? 'I' : 'i'));
            
            rxwaiten = true;
            isr.rx_idle = true;
            rcv_irq = true;
            break;
        }
        
        desc_t *ndesc; 
        if (!gpdesc_to_mdesc(gpdesc, ndesc))
            unimplemented();
         
        // Advance rxdp
        rxdp_lo = desc->link_lo;
        rxdp_hi = desc->link_hi;

        desc = ndesc;
        
    }
    
    return (rcv_irq ? raise_irq() : 0);
    
}

bool vdp83820_t::receive_pkt(torus_dmareg_t::dma_sw_src_key_t src_key, u8_t *pkt, word_t pkt_len,  word_t offset)
{
    word_t  mdata;
    word_t len;
    
    desc_t *desc = find_rxdp(KeyTorus, src_key.raw, mdata);
    
    if (!desc) 
    {
        printf("%s: no rxdp found, drop packet\n", __PRETTY_FUNCTION__);
        release_complete_rxdps();
        return false;
    }
    
    ext_hwhdr_t *ehdr = (ext_hwhdr_t *) mdata;
    
    if (offset == 0)
    {
        torus_t::lnk_hdr_t *pkt_lhdr = (torus_t::lnk_hdr_t *) pkt;
        
        if (pkt_lhdr->lnk_proto != BGLINK_P_NET)
        {
            printf("%s: unhandled non-eth torus packet: link hdr dst %d len %d proto %x\n", 
                   __PRETTY_FUNCTION__, pkt_lhdr->dst_key, pkt_lhdr->len, pkt_lhdr->lnk_proto);
            // Need to revoke descriptor 
            unimplemented();
        }

                 
        assert (pkt_lhdr->dst_key == network_id);
        assert (((eth_hdr_t *) (pkt_lhdr + 1))->is_torus_dst());
            
        // Don't copy lnk header
        pkt += TORUS_LNKDHDR_SIZE;
        pkt_len -= TORUS_LNKDHDR_SIZE;
        ehdr->total_len = pkt_lhdr->len - TORUS_LNKDHDR_SIZE;
        offset += EXT_HWHDR_SIZE; 
        
        len = min(ehdr->total_len, pkt_len);
        
        Ndprintf("\tfirst pkt: hdr dst %x len %d proto %x\n", 
                 pkt_lhdr->dst_key, pkt_lhdr->len, pkt_lhdr->lnk_proto);
        

        if (desc->cmd_status.rx.size < ehdr->total_len + EXT_HWHDR_SIZE - TORUS_LNKDHDR_SIZE)
        {
            printf("%s: eth torus packet too large: %d len desc len %d\n", 
                   __PRETTY_FUNCTION__, pkt_lhdr->len, desc->cmd_status.rx.size);
            // Should advance to the next descriptor
            unimplemented();
        }

        ehdr->first_pkt = true;
    }
    else
    {
        
        len = ehdr->first_pkt ? min((ehdr->total_len-offset+TORUS_LNKDHDR_SIZE), pkt_len) : pkt_len;
        offset += EXT_HWHDR_SIZE;
        offset -= TORUS_LNKDHDR_SIZE;
        
        
        assert(!ehdr->first_pkt || offset < ehdr->total_len + TORUS_LNKDHDR_SIZE);
        Ndprintf("\tfollow-up packet: offset %lx len %ld src_key %lx\n", offset, len, src_key.raw);
    }
    
    assert(mdata + offset + len <= mdata + desc->cmd_status.rx.size);
    memcpy ((void *) (mdata + offset), pkt, len);
    ehdr->len += len;
        
    // Store src key and current size in link header (dst key / opt) for future packets

    Ndprintf("\texthdr: hdr key %lx len %ld total %ld\n", ehdr->key, ehdr->len, ehdr->total_len);
    
    // We postpone relasing the packets to raise_rcv_irq()
    return true;
}

bool vdp83820_t::receive_pkt(tree_packet_t::header_t, tree_packet_t::lnk_hdr_t &lhdr)
{
    if (lhdr.src_key == ptree_t::tree.get_nodeid())
        return false;

    Ndprintf ("%s: receive\n",  __PRETTY_FUNCTION__);
    Ndprintf("\tlink <proto %d dst %ld src %ld>\n", 
           lhdr.lnk_proto,  lhdr.dst_key,  lhdr.src_key);
    
    // early check if packet is valid
    if (lhdr.total_pkt < 1 || lhdr.this_pkt >= lhdr.total_pkt) {
        printf("%s: invalid packet tot: %d, this: %d\n", __PRETTY_FUNCTION__,
               lhdr.total_pkt, lhdr.this_pkt);
        return true;
    }

    word_t  mdata;
    desc_t *desc = find_rxdp(KeyTree, lhdr.conn_id, mdata);
    
    if (!desc) 
    {
        printf("%s: no rxdp found, drop packet\n", __PRETTY_FUNCTION__);
        release_complete_rxdps();
        return false;
    }

    
    ext_hwhdr_t *ehdr = (ext_hwhdr_t *) mdata;
    
    ehdr->first_pkt = true;
    
    if (desc->cmd_status.rx.size < ehdr->total_len)
    {
        printf("%s: eth tree packet too large: %ld len desc len %d\n", 
               __PRETTY_FUNCTION__, ehdr->total_len, desc->cmd_status.rx.size);
        // Should advance to the next descriptor
        unimplemented();
    }
    
    word_t old_len = ehdr->len;
    word_t len = tree_packet_t::mtu_payload;
    word_t offset = EXT_HWHDR_SIZE + (lhdr.this_pkt * tree_packet_t::mtu_payload) - lhdr.opt.net.pad_head;
    
    if(lhdr.this_pkt == lhdr.total_pkt-1)
        len -= lhdr.opt.net.pad_tail;
        
    Ndprintf("\tpkt %x this %d total %d ofs %ld len %ld max %d pad %d %d\n", 
            lhdr.conn_id, lhdr.this_pkt, lhdr.total_pkt, offset, len, desc->cmd_status.rx.size,
            lhdr.opt.net.pad_head, lhdr.opt.net.pad_tail);

    if ((mdata + offset) & 0xF)
    {
        fpu_word_t ftmp[15];
        for (word_t i = 0; i < len / 16; i++)
            fpu_t::read (1+i, &ftmp[i]);
        memcpy((void *) (mdata+offset), &ftmp, len & ~0xF);
    }
    else 
    {
        for (word_t i = 0; i  < len / 16; i++)
            fpu_t::read (1+i, (fpu_word_t *)(mdata + offset + (16*i)));
        
    }

    if (len % 16)
    {
        fpu_word_t flast;
        fpu_t::read (1+(len/16), &flast);
        memcpy((void *) (mdata+offset+(len & ~0xF)), &flast, len%16);
    }

    ehdr->total_len = (lhdr.total_pkt * tree_packet_t::mtu_payload) - lhdr.opt.net.pad_head - lhdr.opt.net.pad_tail;
    ehdr->len = old_len + len - ((lhdr.this_pkt == 0) ? lhdr.opt.net.pad_head : 0);
    ehdr->key_type = KeyTree;
    ehdr->key = lhdr.conn_id;
    
    // We postpone relasing the packets to raise_rcv_irq()
    return true;
}


word_t vdp83820_t::transmit()
{
    Ndprintf("%s: transmit txdp %x %x ex %d\n", __PRETTY_FUNCTION__,
             txdp_hi, txdp_lo, cfg.extended_status_enable);
    
    bool snd_irq = false;
    
    for (paddr_t gpdesc = get_txdp(); gpdesc != 0; gpdesc = get_txdp())
    {
        desc_t *desc;
        
        if (!gpdesc_to_mdesc(gpdesc, desc))
            unimplemented();
        
        Ndprintf("\ttdesc %lx (%lx) [%x:%x %x:%x]\n",
                 (word_t) desc, (word_t)gpdesc, desc->raw32[1], desc->raw32[0], desc->raw32[3], desc->raw32[2]);
        Ndprintf("\tstatus %x %x [sz %d %c%c%c%c%c]\n",
                 desc->raw32[4], desc->raw32[5], desc->cmd_status.rx.size,
                 (desc->flags.used        ? 'U' : 'u'),
                 (desc->cmd_status.rx.own  ? 'O' : 'o'),
                 (desc->cmd_status.rx.ok   ? 'K' : 'k'),
                 (desc->cmd_status.rx.more ? 'M' : 'm'),
                 (desc->cmd_status.rx.intr ? 'I' : 'i'));
                 
        // tx_own   == false                      -> not released by driver
        // tx_ok    == false                      -> empty / partially filled
       
        if (!desc->cmd_status.tx.own)
        {
            isr.tx_idle = true;
            snd_irq = true;
            break;
        }
        
        if( (desc->cmd_status.tx.ok) )
        {
            printf("%s: send wrap-around.\n", __PRETTY_FUNCTION__);
            // Error handling
            unimplemented();
        }
        
        paddr_t gdata = desc->get_bufptr();
        word_t  mdata;
        word_t size = desc->cmd_status.tx.size;
        
        if (!vm->ram.gpa_to_map(gdata, mdata))
            unimplemented();

        // Assume there is room for the link header
        ext_hwhdr_t *ehdr = (ext_hwhdr_t *) mdata;

        gdata += EXT_HWHDR_SIZE; 
        mdata += EXT_HWHDR_SIZE;
        eth_hdr_t *ethhdr = (eth_hdr_t *) mdata;

#ifdef DEBUG_PACKETS
        for (unsigned i=0; i< ROUND_UP(size, 32); i+=32)
        {
            word_t *dbgdata = (word_t *) mdata;
            printf("\t%08lx, %08lx, %08lx, %08lx, %08lx, %08lx, %08lx, %08lx\n", 
                   (dbgdata)[(i/4)+0], (dbgdata)[(i/4)+1], (dbgdata)[(i/4)+2], (dbgdata)[(i/4)+3],
                   (dbgdata)[(i/4)+4], (dbgdata)[(i/4)+5], (dbgdata)[(i/4)+6], (dbgdata)[(i/4)+7]);
        }
#endif
        
        if (ethhdr->is_torus_dst())
        {
            torus_t::coordinates_t torus_dst;
            torus_t::dma_sw_src_key_t torus_src_key;
            torus_t::lnk_hdr_t *torus_lhdr = (torus_t::lnk_hdr_t *) (mdata - TORUS_LNKDHDR_SIZE);
            
            // prepare torus link header
            torus_lhdr->dst_key = network_id;
            torus_lhdr->len = 0;
            torus_lhdr->lnk_proto = BGLINK_P_NET;
            torus_lhdr->opt.raw = 0;
            torus_lhdr->len = size;
            
            ethhdr->to_torus_dst(torus_dst);
            torus_src_key.init(network_id, ptree_t::tree.get_nodeid());

            Ndprintf("\ttorus  src_key %lx dst key %x dst [%02d,%02d,%02d] len %d proto %d \n", torus_src_key.raw,
                     torus_lhdr->dst_key, torus_dst[0], torus_dst[1], torus_dst[2], torus_lhdr->len, torus_lhdr->lnk_proto);

            if (!vm->comm_control.torus_pass(torus_dst))
            {
                printf("%s: torus access to [%02d,%02d,%0d2] denied\n", __PRETTY_FUNCTION__,
                       torus_dst[0], torus_dst[1], torus_dst[2]);
                
                // Ignore packet, release descriptor
                desc->flags.used = false;
                desc->cmd_status.tx.own = false;
                desc->cmd_status.tx.ok = true;
                if (desc->cmd_status.tx.intr)
                {
                    isr.tx_ok = true;
                    snd_irq = true;
                } 
                
            }
            else if (!ptorus_t::torus.vmchan.insert_injdesc(gdata-TORUS_LNKDHDR_SIZE, torus_lhdr->len, torus_dst, torus_src_key, desc))
            {
                printf("%s: TORUS fifo full\n", __PRETTY_FUNCTION__);
                L4_KDB_Enter("UNTESTED");
                break;
            }
            
            if (desc->cmd_status.tx.more)
                unimplemented();

        }
        else 
        {
            size+= mdata & 0xF; 
   
            tree_t::header_t tree_hdr = 0;
            
            // prepare tree hw header
#if 0
            if (!ethhdr->is_multicast_dst())
            {
                tree_hdr.p2p.p2p = 1;
                tree_hdr.p2p.vector = (bridge_vector != ~0ul && !ethhdr->is_target_scope_dst(hwaddr)) 
                    ? bridge_vector /* bridge MAC dests outside of reachable scope */ 
                    : *(word_t *)(&ethhdr->dst[2]);
                 
            }
            tree_hdr.p2p.pclass = tree_route;
#endif            
            
            // prepare tree link header
            tree_t::lnk_hdr_t tree_lhdr ALIGN(16);
            
            tree_lhdr.dst_key = network_id;
            tree_lhdr.src_key = ptree_t::tree.get_nodeid();
            tree_lhdr.opt.net.pad_head = mdata & 0xF;
            tree_lhdr.opt.net.pad_tail = (tree_packet_t::mtu_payload - 1) - ((size-1) % tree_packet_t::mtu_payload);
            tree_lhdr.total_pkt = ((size-1)/tree_packet_t::mtu_payload) + 1;
            tree_lhdr.lnk_proto = BGLINK_P_NET;
            tree_lhdr.set_conn_id();

            if (desc->flags.used == true)
            {
                printf("%s: restart tree xmit with pkt %ld", __PRETTY_FUNCTION__, ehdr->len);
                tree_lhdr.this_pkt = ehdr->len;
            }
            else 
            {
                tree_lhdr.this_pkt = 0;
            }
            desc->flags.used = true;
            
            Ndprintf("\ttree hdr %lx vector %lx lhdr dst key %ld src key %ld conn %x pkt %d/%d pad %d %d \n", 
                     tree_hdr.raw, tree_hdr.p2p.vector, tree_lhdr.dst_key, tree_lhdr.src_key, tree_lhdr.conn_id, 
                     tree_lhdr.this_pkt, tree_lhdr.total_pkt, tree_lhdr.opt.net.pad_head, tree_lhdr.opt.net.pad_tail);
            
            if (!vm->comm_control.tree_pass(tree_lhdr.dst_key))
            {
                printf("%s: tree access to network [%06ld] denied\n", __PRETTY_FUNCTION__, tree_lhdr.dst_key);
                // Ignore packet, release descriptor
            }
            else 
            {
                // iterate over fragments and send them out...
                for (; tree_lhdr.this_pkt < tree_lhdr.total_pkt; tree_lhdr.this_pkt++)
                {
                    Ndprintf("\tsend %d of %d data %lx hdr %lx len %ld\n", tree_lhdr.this_pkt, tree_lhdr.total_pkt, 
                             (mdata & ~0xF), (word_t) &tree_lhdr, size);
                
                    if (!ptree_t::tree.vmchan.send_packet (tree_hdr, (mdata & ~0xF), tree_lhdr))
                    {
                        printf("%s: TREE fifo full\n", __PRETTY_FUNCTION__);
                        ptree_t::tree.vmchan.set_inj_pending(true);
                        L4_KDB_Enter("UNTESTED");
                        break;
                    }
                    ehdr->len = tree_lhdr.this_pkt;
                
                    mdata += tree_packet_t::mtu_payload;
                    size -= tree_packet_t::mtu_payload;

                }
            }
            // Release descriptor
            desc->flags.used = false;
            desc->cmd_status.tx.own = false;
            desc->cmd_status.tx.ok = true;
            if (desc->cmd_status.tx.intr)
            {
                isr.tx_ok = true;
                snd_irq = true;
            } 
            ptree_t::tree.vmchan.set_inj_pending(false);
           
            if (desc->cmd_status.tx.more)
                unimplemented();
        }
        
        txdp_lo = desc->link_lo;
        txdp_hi = desc->link_hi;
        
    }

    return (snd_irq ? raise_irq() : 0);
}


word_t vdp83820_t::gpr_load( Register reg, word_t *gpr )
{
    
    if( reg < LAST )
        *gpr = regs[REG_IDX(reg)];

    if( reg == ISR ) 
    {
        isr.raw = 0; // On read, the ISR register clears.
    }

    Nregdprintf ("%s: reg=%#lx -> val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    return raise_irq();
}

word_t vdp83820_t::gpr_store( Register reg, word_t *gpr )
{
    word_t val = *gpr;
    word_t core_mask = 0;
    
    Nregdprintf ("%s: reg=%#lx <- val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);
    
    word_t oldval = val;
    if( reg < LAST && reg != ISR )
    {
        oldval = regs[REG_IDX(reg)];
        regs[REG_IDX(reg)] = val;
    }

    switch( reg ) 
    {
    case CR: 
    {
        if ( cr.non_txen_rxen() )
        {
 
            if( cr.tx_disable ) 
            {                
                Nregdprintf ("%s: TX_DISABLE\n", __PRETTY_FUNCTION__);
                cr.tx_disable = cr.tx_enable = 0;
            }
            else if( cr.tx_enable )
            {
                Nregdprintf ("%s: TX_ENABLE\n", __PRETTY_FUNCTION__);
                core_mask |= transmit();
            }
            if( cr.rx_disable )
            {                
                Nregdprintf ("%s: RX_DISABLE\n", __PRETTY_FUNCTION__);
                cr.rx_disable = cr.rx_enable = 0;
            }
            else if( cr.rx_enable )
            {
                Nregdprintf ("%s: RX_ENABLE\n", __PRETTY_FUNCTION__);
                rxwaiten = false;
                core_mask |= release_complete_rxdps();
            }
            if( cr.tx_reset ) 
            {
                Nregdprintf ("%s: TX RESET\n", __PRETTY_FUNCTION__);
                cr.tx_reset = 0;
                isr.tx_reset_complete = 1;
                core_mask |= raise_irq();
            }
            if( cr.rx_reset ) 
            {
                Nregdprintf ("%s: RX RESET\n", __PRETTY_FUNCTION__);
                cr.rx_reset = 0;
                isr.rx_reset_complete = 1;
                core_mask = raise_irq();
            }
            if( cr.software_int ) 
            {
                Nregdprintf ("%s: RX SOFTWARE_INT\n", __PRETTY_FUNCTION__);
                cr.software_int = 0;
                isr.software_int = 1;
                core_mask |= raise_irq();
            }
            if( cr.reset ) 
            {
                Nregdprintf ("%s: RESET\n", __PRETTY_FUNCTION__);
                reset();
                
                if (!vmchan_enabled) 
                {
                    reset_hwaddr();
                    ptorus_t::torus.vmchan.enable(vm->ram);
                    ptorus_t::torus.vmchan.register_vdp83820(this);
                    ptree_t::tree.vmchan.enable();
                    ptree_t::tree.vmchan.register_vdp83820(this);
                }
                
                cr.raw = 0;
            }
        }
        else if( cr.tx_enable )
        { 
            Nregdprintf ("%s: TX_ENABLE\n", __PRETTY_FUNCTION__);
            core_mask |= transmit();
        }
        else if( cr.rx_enable )
        {
            Nregdprintf ("%s: RX_ENABLE\n", __PRETTY_FUNCTION__);
            rxwaiten = false;
            core_mask |= release_complete_rxdps();
        }
        break;
    }
    case PTSCR:
        ptscr.raw = 0;
        ptscr.rbist_done = 1;	// BIST completed.
        break;
    case CFG: 
        cfg.mode_1000 = 1;
        cfg.full_duplex_status = 0; // Reverse polarity.
        cfg.link_status = 1;
        cfg.speed_status = 1; // Reverse polarity.
        cfg.big_endian = 1; // Always big endian
        cfg.ext_hwhdr = 1;
        break;
    case RFCR:
        if (rfcr.rfaddr_lo == 0 && rfcr.rfaddr_hi <= 2)
        { 
            rfdr.dlo = hwaddr[2 * rfcr.rfaddr_hi];
            rfdr.dhi = hwaddr[2 * rfcr.rfaddr_hi+1];
            Nregdprintf("%s: RFCR %x -> RFDR %x\n", __PRETTY_FUNCTION__, rfcr.raw, rfdr.raw);
        }
        break;
    case RXDP:
        Nregdprintf("%s: RXDP %x (old %lx)\n", __PRETTY_FUNCTION__, rxdp_lo, oldval);
        if (val != oldval) crdd = false;
        core_mask |= release_complete_rxdps();
        break;
    case RXDP_HI: 
        Nregdprintf("%s: RXDPHI %x (old %lx)\n", __PRETTY_FUNCTION__, rxdp_hi, oldval);
        if (val != oldval) crdd = false;
        core_mask |= release_complete_rxdps();
        break;
    case TXDP:
        Nregdprintf("%s: TXDP %x\n", __PRETTY_FUNCTION__, txdp_lo);
        core_mask = transmit();
        break;
    case TXDP_HI:
        Nregdprintf("%s: TXDP %x\n", __PRETTY_FUNCTION__, txdp_hi);
        break;
    default: 
        break;   
    }

    return core_mask;
}
