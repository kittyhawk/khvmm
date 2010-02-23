/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, Jan Stoess, IBM Corporation
 *
 * File path:       personality.h
 * Description:     Personality Page
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"
class bgp_uci_computecard_t
{ 
public:
    union
    {
        u32_t raw;
        struct 
        {
            // "Rxy-Mm-Nnn-Jxx": R<RackRow><RackColumn>-M<Midplane>-N<NodeCard>-J<ComputeCard>
            unsigned int component   :  5;  // when _bgp_uci_component_computecard
            unsigned int rackrow     :  4;  // 0..f
            unsigned int rackcolumn  :  4;  // 0..f
            unsigned int midplane    :  1;  // 0=bottom, 1=top
            unsigned int nodecard    :  4;  // 00..15: 00=bf, 07=tf, 08=br, 15=tr)
            unsigned int computecard :  6;  // 04..35 (00-01 iocard, 02-03 reserved, 04-35 computecard)
            unsigned int _zero       :  8;  // zero's
        };
    };
        
    inline void operator = (word_t r) { this->raw = r; }
};


class bgp_personality_kernel_t
{
private:
    u32_t uci;
    u32_t mhz;
    u32_t policy;
    u32_t process;
    u32_t trace;
    u32_t node;
    u32_t l1;
    u32_t l2;
    u32_t l3;
    u32_t l3_select;
    u32_t shared_mem;
    u32_t clockstop0;
    u32_t clockstop1;

    enum
    {
        EN_GLOBAL_INTS = 1ul << 26,
        EN_COLLECTIVE  = 1ul << 25,
        EN_TORUS       = 1ul << 24,
        EN_DMA         = 1ul << 17,
        EN_TREEBLAST   = 1ul << 6,
    };

public:
    bgp_personality_kernel_t (unsigned);
};

class bgp_personality_ddr_t
{
    private:
        u32_t ddr_flags;
        u32_t srbs0;
        u32_t srbs1;
        u32_t pbx0;
        u32_t pbx1;
        u32_t memconfig0;
        u32_t memconfig1;
        u32_t parmctl0;
        u32_t parmctl1;
        u32_t miscctl0;
        u32_t miscctl1;
        u32_t cmdbufmode0;
        u32_t cmdbufmode1;
        u32_t refrinterval0;
        u32_t refrinterval1;
        u32_t odtctl0;
        u32_t odtctl1;
        u32_t datastrobecalib0;
        u32_t datastrobecalib1;
        u32_t dqsctl;
        u32_t throttle;
        u16_t sizemb;
        u8_t  chips;
        u8_t  cas;

    public:
        bgp_personality_ddr_t (size_t);
};

class bgp_personality_network_t
{
    private:
        u32_t blockid;
        u8_t  xnodes;
        u8_t  ynodes;
        u8_t  znodes;
        u8_t  xcoord;
        u8_t  ycoord;
        u8_t  zcoord;
        u16_t psetnum;
        u32_t psetsize;
        u32_t rankinpset;
        u32_t ionodes;
        u32_t rank;
        u32_t ionoderank;
        u16_t treeroutes[16];

    public:
        bgp_personality_network_t();
};

class bgp_ipaddr_t
{
    private:
        u8_t octet[16];
};

class bgp_personality_ethernet_t
{
    private:
        u16_t        mtu;
        u8_t         emacid[6];
        bgp_ipaddr_t ip;
        bgp_ipaddr_t netmask;
        bgp_ipaddr_t broadcast;
        bgp_ipaddr_t gateway;
        bgp_ipaddr_t nfsserver;
        bgp_ipaddr_t servicenode;
        char         nfsexport[32];
        char         nfsmount[32];
        u8_t         securitykey[32];
};

class bgp_personality_t
{
    private:
        u16_t                       crc;
        u8_t                        version;
        u8_t                        size;
        bgp_personality_kernel_t    kernel;
        bgp_personality_ddr_t       ddr;
        bgp_personality_network_t   net;
        bgp_personality_ethernet_t  eth;
        u32_t                       pad[2];

    public:
        static inline void *operator new (size_t size, word_t addr)
        {
            return memset (reinterpret_cast<void *>(addr), 0, size);
        }

        bgp_personality_t (unsigned, size_t);
};
