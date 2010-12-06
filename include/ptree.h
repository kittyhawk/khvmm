/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       ptree.h
 * Description:     Physical BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "config.h"
#include "fdt.h"
#include "fpu.h"
#include "ibm440.h"
#include "powerpc.h"
#include "std.h"
#include "tree.h"
#include "vdp83820.h"

class ptree_t;

class ptree_chan_t : public tree_chan_t
{
    friend class ptree_t;

private:
    ptree_t *   tree;
    word_t      base;

    word_t gpr_read (Register reg)
        {
    		//printf("Reading from gpr addr %#lx\n",base + reg);
    		//sync();
            return *reinterpret_cast<word_t volatile *>(base + reg);
        }

    void gpr_write (Register reg, word_t val)
        {
            *reinterpret_cast<word_t volatile *>(base + reg) = val;
        }

    void fpr_write (Register reg, fpu_word_t * const val=NULL, const word_t fpr=0)
        {
            if (val)
                fpu_t::write (fpr, val);

            fpu_t::read (fpr, reinterpret_cast<fpu_word_t *>(base + reg));
        }

    void fpr_read (Register reg, fpu_word_t * const val=NULL, const word_t fpr=0)
        {
            fpu_t::write (fpr, reinterpret_cast<fpu_word_t *>(base + reg));

            if (val)
                fpu_t::read (fpr, val);
        }

public:
    ptree_chan_t (ptree_t *t, unsigned c, u64_t phys, word_t virt) : tree_chan_t (c), tree (t), base (virt)
        {
            word_t phys_low = (word_t)phys;
            word_t phys_high = (word_t)(phys >> 32);
			L4_Sigma0_GetPage (L4_nilthread, L4_Fpage(phys_low, 0x1000), phys_high, L4_Fpage(virt, 0x1000));
        }

    word_t recv_packet();
    bool send_packet (word_t head, fpu_word_t *data, lnk_hdr_t * const lhdr=NULL);
    
    void set_inj_wm_irq(bool i);
    word_t handle_inj_wm_irq();
};

class ptree_t : public tree_t
{
    friend class ptree_chan_t;
private:
    
    // For now, we rely on U-Boots hardcccoded interrupt map
    word_t nodeid;
    word_t nodeaddr;

    ptree_chan_t chan0;
    ptree_chan_t chan1;

    class vmchan_t
    {
        // The channel's nic client 
        vdp83820_t   *vnic;

        // The channel's tree channel
        ptree_chan_t &chan;
        
        bool enabled;
        bool inj_pending;

    public: 
        static const word_t ch = 1;
        
        bool is_enabled() { return enabled; }
        void enable() { enabled = true; }
       
        void register_vdp83820(vdp83820_t *v) { assert(vnic == NULL || vnic == v); vnic = v; }
        
        bool is_pkt(word_t channel, lnk_hdr_t &lhdr) 
            { return enabled && ch == channel && vnic && vnic->is_vdp83820_pkt(lhdr); }

        bool is_inj_pending(word_t channel) 
            { return enabled && ch == channel && inj_pending; }
        void set_inj_pending(bool p) 
            { if (inj_pending != p) { inj_pending = p; chan.set_inj_wm_irq(p); } }
        word_t handle_inj_pending() 
            { return vnic->transmit(); }
        
        word_t send_packet(header_t hdr, word_t data, lnk_hdr_t &lhdr) 
            { return chan.send_packet(hdr.raw, (fpu_word_t *) data, &lhdr);  }

        word_t recv_packet(header_t hdr, lnk_hdr_t &lhdr) 
            { return vnic->receive_pkt(hdr, lhdr) ; }
        
        word_t raise_rcv_irq() 
            { return vnic->raise_rcv_irq(); }
        
        
        vmchan_t(ptree_chan_t &c) : vnic(NULL), 
                                    chan(c),
                                    enabled (false),
                                    inj_pending (false)
            {}
    };
    
    struct 
    {
        word_t snd_id;
        word_t rcv_id;
        word_t dest_node;
        word_t proto_id;
        word_t tree_route;
        word_t tree_channel;
    } l4dbgcon;

    word_t dcr_read (Dcr reg)
        {
            return mfdcrx(DCRBASE + reg );
        }

    void dcr_write (Dcr reg, word_t val)
        {
            mtdcrx( DCRBASE + reg, val);
        }

public:
    ptree_chan_t *const chan;

    vmchan_t vmchan;
    static ptree_t tree;
    
    word_t get_nodeid() const { return nodeid; }
    word_t get_nodeaddr() const { return nodeaddr; }
    
    bool handle_hwirq(word_t irq, word_t &core_mask);
 
    word_t const get_rcv_irq() { return rcv_irq; }
    word_t const get_inj_irq() { return inj_irq; }
    
    ptree_t (word_t addr) : chan0 (this, 0, BGP_PHYS_TREE0, addr),
                            chan1 (this, 1, BGP_PHYS_TREE1, addr + 0x1000),
                            chan (&chan0),
                            vmchan (chan[vmchan.ch]) {}
        
    void init(L4_ThreadId_t irq_handler);
};
