/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 * Copyright (C) 2008, Jan Stoess, IBM Corporation
 *
 * File path:       tree.h
 * Description:     Generic BlueGene Tree Device
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once
#include "std.h" 

class tree_packet_t
{
public:
    /* hardware header */
    struct header_t
    {
        union {
            word_t raw;
            struct {
                word_t pclass           : 4;
                word_t p2p		: 1;
                word_t irq		: 1;
                word_t vector           : 24;
                word_t csum_mode	: 2;
            } p2p;
            struct {
                word_t pclass           : 4;
                word_t p2p		: 1;
                word_t irq		: 1;
                word_t op		: 3;
                word_t opsize           : 7;
                word_t tag		: 14;
                word_t csum_mode	: 2;
            } bcast;
        } PACKED;
    
        inline void operator = (word_t r) { this->raw = r; }
    
        header_t(word_t r=0) { raw = r; } 

        void set_p2p(word_t pclass, word_t vector, bool irq = false)
            {
                raw = 0;
                p2p.pclass = pclass;
                p2p.irq = irq;
                p2p.vector = vector;
                p2p.p2p = 1;
            }

        void set_broadcast(word_t pclass, word_t tag = 0, bool irq = false)
            {
                raw = 0;
                bcast.pclass = pclass;
                bcast.irq = irq;
                bcast.tag = tag;
            }
    };

    enum DstKeys { NUM_DST_KEYS = 1ul << 24 };

    /* link layer */
    class lnk_hdr_t
    {

    public:
        union   
        {
            fpu_word_t raw;
            struct 
            {
                word_t dst_key; 
                word_t src_key; 
                u16_t conn_id; 
                u8_t this_pkt; 
                u8_t total_pkt;
                u16_t lnk_proto;	// 1 eth, 2 con, 3...
                union {
                    u16_t raw;
                    struct {
                        u16_t option    : 4;
                        u16_t pad_head	: 4;
                        u16_t pad_tail	: 8;
                    } net;
                    struct {
                        u16_t len;
                    } con;
                } PACKED opt;
            };
        } PACKED;
        void set_conn_id() { static u8_t _id = 0;  conn_id = _id++; }
    };

    enum Misc 
    {
        mtu             = 256,
        mtu_payload     = mtu - sizeof(lnk_hdr_t)
    };

};
    
    
class tree_chan_t : public tree_packet_t
{
protected:
    const unsigned    channel;
    
    static unsigned const fifo_size = 8;

public:
    struct status_t 
    {
        union {
            word_t raw;
            struct {
                word_t inj_pkt          : 4;
                word_t inj_qwords	: 4;
                word_t                  : 4;      
                word_t inj_hdr          : 4;
                word_t rcv_pkt          : 4;
                word_t rcv_qwords	: 4;
                word_t                  : 3;
                word_t irq		: 1;
                word_t rcv_hdr          : 4;
            };
        };
    
        inline void operator = (word_t r) { raw = r; }
        status_t(word_t r) { raw = r;} 

    };


    enum Register
    {
        PID         = 0x0,      // Processor Injection Data
        PIH         = 0x10,     // Processor Injection Header
        PRD         = 0x20,     // Processor Reception Data
        PRH         = 0x30,     // Processor Reception Header
        PSR         = 0x40,     // Processor Status Register
        PSR_OTHER   = 0x50,     // Processor Status Register (Other Channel)
    };

    tree_chan_t (unsigned c) : channel (c) {}
};

class tree_t : public tree_packet_t
{
public:
    
    // See: arch/include/bpcore/bgp_dcrmap.h
    enum DcrBase
    {
        // Tree (0xc00-0xc7f)
        DCRBASE                    = 0xc00,
        DCRBASE_MIN                = DCRBASE,
        DCRBASE_MAX                = DCRBASE_MIN + 0x7f,
    };
    
    enum Dcr
    {
        // Class Definition Registers
        RDR_MIN     = 0x0,
        RDR_MAX     = 0x7,
        ISR_MIN     = 0x8,
        ISR_MAX     = 0x9,

        // Arbiter Control Registers
        RCFG        = 0x10,
        RTO         = 0x11,
        RTIME       = 0x12,
        RSTAT       = 0x13,
        HD00        = 0x14,
        HD01        = 0x15,
        HD10        = 0x16,
        HD11        = 0x17,
        HD20        = 0x18,
        HD21        = 0x19,
        HDI0        = 0x1a,
        HDI1        = 0x1b,
        FORCEC      = 0x1c,
        FORCER      = 0x1d,
        FORCEH      = 0x1e,
        XSTAT       = 0x1f,

        // Global Registers
        FPTR        = 0x40,
        NADDR       = 0x41,
        VCFG0       = 0x42,
        VCFG1       = 0x43,

        // Processor Reception Registers
        PRXF        = 0x44,
        PRXEN       = 0x45,
        PRDA        = 0x46,
        PRDD        = 0x47,

        // Processor Injection Registers
        PIXF        = 0x48,
        PIXEN       = 0x49,
        PIDA        = 0x4a,
        PIDD        = 0x4b,

        CSPY0       = 0x4c,
        CSHD0       = 0x4d,
        CSPY1       = 0x4e,
        CSHD1       = 0x4f
    };

    enum
    {
        PRXEN_MASK_CRITICAL     = 0x3f,
        PRXEN_MASK_NONCRITICAL  = 0xff0000,

        PIXEN_MASK_CRITICAL     = 0x7e,
        PIXEN_MASK_NONCRITICAL  = 0x3ff0000,

        PRX_WM1     = 1ul << 0,     //  VC1 payload FIFO above watermark
        PRX_WM0     = 1ul << 1,     //  VC0 payload FIFO above watermark
        PRX_HFU1    = 1ul << 2,     //  VC1 header FIFO under-run error
        PRX_HFU0    = 1ul << 3,     //  VC0 header FIFO under-run error
        PRX_PFU1    = 1ul << 4,     //  VC1 payload FIFO under-run error
        PRX_PFU0    = 1ul << 5,     //  VC0 payload FIFO under-run error
        PRX_UE      = 1ul << 16,    //  Uncorrectable SRAM ECC error
        PRX_COLL    = 1ul << 17,    //  FIFO read collision error
        PRX_ADDR1   = 1ul << 18,    //  P1 bad (unrecognized), address error
        PRX_ADDR0   = 1ul << 19,    //  P0 bad (unrecognized), address error
        PRX_ALIGN1  = 1ul << 20,    //  P1 address alignment error
        PRX_ALIGN0  = 1ul << 21,    //  P0 address alignment error
        PRX_APAR1   = 1ul << 22,    //  P1 address parity error
        PRX_APAR0   = 1ul << 23,    //  P0 address parity error

        PIX_ENABLE  = 1ul << 0,     //  Injection interface enable (if enabled in PIXEN)
        PIX_WM1     = 1ul << 1,     //  VC1 payload FIFO at or below watermark
        PIX_WM0     = 1ul << 2,     //  VC0 payload FIFO at or below watermark
        PIX_HFO1    = 1ul << 3,     //  VC1 header FIFO overflow error
        PIX_HFO0    = 1ul << 4,     //  VC0 header FIFO overflow error
        PIX_PFO1    = 1ul << 5,     //  VC1 payload FIFO overflow error
        PIX_PFO0    = 1ul << 6,     //  VC0 payload FIFO overflow error
        PIX_UE      = 1ul << 16,    //  Uncorrectable SRAM ECC error
        PIX_COLL    = 1ul << 17,    //  FIFO write collision error
        PIX_DPAR1   = 1ul << 18,    //  P1 data parity error
        PIX_DPAR0   = 1ul << 19,    //  P0 data parity error
        PIX_ADDR1   = 1ul << 20,    //  P1 bad (unrecognized), address error
        PIX_ADDR0   = 1ul << 21,    //  P0 bad (unrecognized), address error
        PIX_ALIGN1  = 1ul << 22,    //  P1 address alignment error
        PIX_ALIGN0  = 1ul << 23,    //  P0 address alignment error
        PIX_APAR1   = 1ul << 24,    //  P1 address parity error
        PIX_APAR0   = 1ul << 25     //  P0 address parity error
    };

    static const word_t VCFG_RCV_FIFO_WM(const word_t wm) 
        {  assert(wm < 8); return (wm << 8); }
    static const word_t VCFG_INJ_FIFO_WM(const word_t wm) 
        {  assert(wm < 8); return wm; }
    

    // For now, we rely on U-Boots hardcccoded interrupt map
    enum Irq
    {
        irq_min         = 0xa0,
        inj_irq         = 0xb4,
        rcv_irq         = 0xb5,
        rcv_irq_vc0     = 0xb6,
        rcv_irq_vc1     = 0xb7,
        irq_max         = 0xb7
    };


    static bool is_irq(const word_t irq) 
        { return (irq >= irq_min && irq <= irq_max); }

    tree_t () 
        { 
            assert(sizeof(tree_t::header_t) == 4);
            assert(sizeof(tree_t::lnk_hdr_t) == 16); 
        }

};
