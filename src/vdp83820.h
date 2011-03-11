/*********************************************************************
 *
 * Copyright (C) 2005,  University of Karlsruhe
 * Copyright (C) 2008,  Jan Stoess IBM Corporation 
 * 
 * Description:     Virtual NS 83820 Controller
 *
 ********************************************************************/
#pragma once

#include "std.h"
#include "fdt.h"
#include "sync.h"
#include "config.h"
#include "tree.h"
#include "torus.h"

class vm_t;

class vdp83820_t {

    typedef u8_t eth_hwaddr_t[6];

    class eth_hdr_t 
    {
    public:
        eth_hwaddr_t        dst;
        eth_hwaddr_t        src;
        u16_t               proto;
    
        bool is_multicast_dst() const { return (dst[0] & 0x01 ); }
        bool is_torus_dst() const { return ((dst[0] & 0x7) == 0x6); }
        bool is_target_scope_dst(eth_hwaddr_t &s) const
            { return (dst[0] == s[0]) && (dst[1] == s[1]) && (dst[2] == s[2]);} 
        bool is_dst(eth_hwaddr_t &s) const
            { return (dst[0] == s[0]) && (dst[1] == s[1]) && (dst[2] == s[2]) && 
                     (dst[3] == s[3]) && (dst[4] == s[4]) && (dst[5] == s[5]);   } 
        void to_torus_dst(torus_t::coordinates_t &c) const 
            { 
                assert(is_torus_dst());
                c[0] = dst[3]; c[1] = dst[4]; c[2] = dst[5];
            }
    };
    
public:

    enum KeyType
    {
        KeyTorus = 0,
        KeyTree = 1
    };
    
    struct ext_hwhdr_t 
    {
    public:
        struct
        { 
            word_t first_pkt    :1;
            word_t key_type     :1; // 0: torus, 1: tree
            word_t              :29;
        };
        word_t key;
        word_t len;
        word_t total_len;
    };

    class desc_t 
    {
    public:
        union {
        
            u32_t raw32[6];
            struct  {    
                // jsXXX: we abuse link_lo bit 0 to mark usage, thus
                //        our packet engine _relies_ on good addresses 
                //        provided by the driver.
                union {
                    u32_t  link_lo;
                    struct
                    {
                        u32_t  reserved :31;
                        u32_t  used      :1;
                    } flags;
                };
                u32_t  link_hi;
                u32_t  bufptr_lo;
                u32_t  bufptr_hi;
                union {
                    struct {
                        u16_t own : 1;
                        u16_t more : 1;
                        u16_t intr : 1;
                        u16_t crc : 1;
                        u16_t ok : 1;
                        u16_t rx_aborted : 1;
                        u16_t rx_overrun : 1;
                        u16_t dst_class : 2;
                        u16_t long_pkt : 1;
                        u16_t runt_pkt : 1;
                        u16_t invalid_symbol_err : 1;
                        u16_t crc_err : 1;
                        u16_t frame_alignment_err : 1;
                        u16_t loopback_pkt : 1;
                        u16_t length_err : 1;
                        u16_t size : 16;
                    } rx;
                    struct {
                        u16_t own : 1;
                        u16_t more : 1;
                        u16_t intr : 1;
                        u16_t crc : 1;
                        u16_t ok : 1;
                        u16_t tx_abort : 1;
                        u16_t tx_fifo_underrun : 1;
                        u16_t carrier_sense_lost : 1;
                        u16_t tx_deferred : 1;
                        u16_t excessive_deferral : 1;
                        u16_t out_of_window_collision : 1;
                        u16_t excessive_collisions : 1;
                        u16_t collision_cnt : 4;
                        u16_t size : 16;
                    } tx;
                } cmd_status;
                struct {
                    u16_t unused : 9;
                    u16_t udp_checksum_err : 1;
                    u16_t udp_pkt : 1;
                    u16_t tcp_checksum_err : 1;
                    u16_t tcp_pkt : 1;
                    u16_t ip_checksum_err : 1;
                    u16_t ip_pkt : 1;
                    u16_t vlan_pkt : 1;
                    u16_t vlan_tag : 16;
                } extended_status;
            };
        } PACKED;
         
        paddr_t get_linkptr() { return (((paddr_t) link_hi << 32) | (link_lo & ~0x1)); };
        paddr_t get_bufptr() { return (((paddr_t) bufptr_hi << 32) | bufptr_lo); };
    };

    struct cr_t {
        union {
            u32_t raw;
            struct {
                u32_t unused2 : 15;
                u32_t rx_queue_select : 4;
                u32_t tx_queue_select : 4;
                u32_t reset : 1;
                u32_t software_int : 1;
                u32_t unused1 : 1;
                u32_t rx_reset : 1;
                u32_t tx_reset : 1;
                u32_t rx_disable : 1;
                u32_t rx_enable : 1;
                u32_t tx_disable : 1;
                u32_t tx_enable : 1;
            };
        };
    
        bool non_txen_rxen()
            { 
                cr_t cr2;
                cr2.raw = raw; 
                cr2.rx_enable = cr2.tx_enable = 0;
                return (cr2.raw != 0);
            }
    };

    struct cfg_t {
        union {
            u32_t raw;
            struct {
                u32_t link_status : 1;
                u32_t speed_status : 2;
                u32_t full_duplex_status : 1;
                u32_t ext_hwhdr : 1;
                u32_t reserved3 : 2;
                u32_t tbi_en : 1;
                u32_t reserved2 : 1;
                u32_t mode_1000 : 1;
                u32_t reserved1 : 1;
                u32_t phy_interrupt_ctrl : 3;
                u32_t timer_test_mode : 1;
                u32_t mrm_dis : 1;
                u32_t mwi_dis : 1;
                u32_t target_64bit_enable : 1;
                u32_t pc64_detected : 1;
                u32_t data_64bit_enable : 1;
                u32_t master_64bit_enable : 1;
                u32_t phy_rst : 1;
                u32_t phy_dis : 1;
                u32_t extended_status_enable : 1;
                u32_t reqalg : 1;
                u32_t sb : 1;
                u32_t pow : 1;
                u32_t exd : 1;
                u32_t pesel : 1;
                u32_t brom_dis : 1;
                u32_t ext_125 : 1;
                u32_t big_endian : 1;
            };
        };
    };

    struct isr_t {
        union {
            u32_t raw;
            struct {
                u32_t reserved : 1;
                u32_t txdesc3 : 1;
                u32_t txdesc2 : 1;
                u32_t txdesc1 : 1;
                u32_t txdesc0 : 1;
                u32_t rxdesc3 : 1;
                u32_t rxdesc2 : 1;
                u32_t rxdesc1 : 1;
                u32_t rxdesc0 : 1;
                u32_t tx_reset_complete : 1;
                u32_t rx_reset_complete : 1;
                u32_t dperr : 1;
                u32_t sserr : 1;
                u32_t rmabt : 1;
                u32_t rtabt : 1;
                u32_t rx_fifo_overrun : 1;
                u32_t high_bits_int : 1;
                u32_t phy : 1;
                u32_t pme : 1;
                u32_t software_int : 1;
                u32_t mib_service : 1;
                u32_t tx_underrun : 1;
                u32_t tx_idle : 1;
                u32_t tx_err : 1;
                u32_t tx_desc : 1;
                u32_t tx_ok : 1;
                u32_t rx_overrun : 1;
                u32_t rx_idle : 1;
                u32_t rx_early : 1;
                u32_t rx_err : 1;
                u32_t rx_desc : 1;
                u32_t rx_ok : 1;
            };
        };
    };
    
    union  rfcr_t 
    {
        u32_t raw;
        struct
        {
            //bit 0 unten 
            u32_t unimplemented : 23;
            u32_t rfaddr_hi     : 8;
            u32_t rfaddr_lo     : 1;
        };
    };
    
    union rfdr_t 
    {
        u32_t raw;
        struct 
        {
            u32_t unused        :14;
            u32_t mask          :2;
            u32_t dhi           :8;
            u32_t dlo           :8;
        };
    };

    union ptscr_t 
    {
        u32_t raw;
        struct 
        {
            u32_t reserved1            :18;
            u32_t rbist_rst            :1;
            u32_t reserved2            :2;
            u32_t rbist_en             :1;
            u32_t rbist_done           :1;
            u32_t rbist_rx1fail        :1;
            u32_t rbist_rx0fail        :1;
            u32_t reserved3            :1;
            u32_t rbist_txfail         :1;
            u32_t rbist_hfail          :1;
            u32_t rbist_rxfail         :1;
            u32_t eeload_en            :1;
            u32_t eebist_en            :1;
            u32_t eebist_fail          :1;
        };
    };
    
   
    enum Register 
    {
	CR             =0x00,
        CFG            =0x04,
        MEAR           =0x08,
        PTSCR          =0x0c,
        ISR            =0x10,
	IMR            =0x14,
        IER            =0x18,
        IHR            =0x1c,
        TXDP           =0x20,
        TXDP_HI        =0x24,
	TXCFG          =0x28,
        GPIOR          =0x2c,
        RXDP           =0x30,
        RXDP_HI        =0x34,
	RXCFG          =0x38,
        PQCR           =0x3c,
        WCSR           =0x40,
        PCR            =0x44,
        RFCR           =0x48,
	RFDR           =0x4c,
        SRR            =0x58,
        VRCR           =0xbc,
        VTCR           =0xc0,
        VDR            =0xc4,
	CCSR           =0xcc,
        TBICR          =0xe0,
        TBISR          =0xe4,
        TANAR          =0xe8,
        TANLPAR        =0xec,
	TANER          =0xf0,
        TESR           =0xf4,
        LAST           =0xf8
    };

    
    enum Misc {
        EXT_HWHDR_SIZE          =   16,
        TREE_LNKDHDR_SIZE       =   sizeof(tree_t::lnk_hdr_t),
        TORUS_LNKDHDR_SIZE      =   sizeof(torus_t::lnk_hdr_t),
        irq                     =   0x110,
    };
    
    static const word_t network_id = 2;         // 
    static const word_t bridge_vector = ~0ul;   // no tree bridge
    static const word_t tree_route    = 0xf;   

private:
    spinlock_t          lock;       // DP83820 Lock
    vm_t *const         vm;         // Parent VM
    
    eth_hwaddr_t        hwaddr;
    bool                vmchan_enabled;
    bool                crdd;
    bool                rxwaiten;
    desc_t              *rxdesccache;
    union
    {
        u32_t               regs[LAST];
        struct 
        {
            cr_t            cr;
            cfg_t           cfg;
            u32_t           mear;
            ptscr_t         ptscr;
            isr_t           isr;
            u32_t           imr;
            u32_t           ier;
            u32_t           ihr;
            u32_t           txdp_lo;
            u32_t           txdp_hi;
            u32_t           txcfg;
            u32_t           gpior;
            u32_t           rxdp_lo;
            u32_t           rxdp_hi;
            u32_t           rxcfg;
            u32_t           pqcr;
            u32_t           wcsr;
            u32_t           pcr;
            rfcr_t          rfcr;
            rfdr_t          rfdr;
            u32_t           srr;
            u32_t           vrcr;
            u32_t           vtcr;
            u32_t           vdr;
            u32_t           ccsr;
            u32_t           tbicr;
            u32_t           tbisr;
            u32_t           tanar;
            u32_t           tanlpar;
            u32_t           taner;
            u32_t           tesr;
            u32_t           last;
        };
    };
    
    static const word_t REG_IDX(const Register reg) 
        {  return (reg / sizeof(volatile u32_t)); }
    
    
    paddr_t get_txdp() { return (((paddr_t) txdp_hi << 32) | txdp_lo); };
    paddr_t get_rxdp() { return (((paddr_t) rxdp_hi << 32) | rxdp_lo); };
    
    vdp83820_t::desc_t *find_rxdp(KeyType, word_t, word_t &);
    word_t release_complete_rxdps();
    
    void reset_hwaddr();
    void reset();

    
    word_t raise_irq();
    
    /* 
     * When protecting DMA physical addresses we assume:
     *   GPA begins at 0
     *   Segment-based GPA->HPA mapping, no paging
     */

    bool gpdesc_to_mdesc(paddr_t gpdesc, vdp83820_t::desc_t *&) const;
    
public:

    word_t gpr_load     (Register, word_t *);
    word_t gpr_store    (Register, word_t *);
    
    vdp83820_t (vm_t *v) : vm (v), vmchan_enabled(false), crdd(false), rxwaiten(false), rxdesccache(NULL) 
        { assert(sizeof(ext_hwhdr_t) == 16); reset(); }
    
    static const word_t is_irq(const word_t i) { return (i == irq); }
    word_t ack_irq(word_t)
        { return raise_irq(); }
    
    word_t raise_send_irq() 
        { 
            isr.tx_ok = true;
            return raise_irq(); 
        }
    word_t raise_rcv_irq() { return release_complete_rxdps(); }
   
    bool is_vdp83820_pkt(tree_t::lnk_hdr_t &lhdr) { return ((lhdr.lnk_proto == BGLINK_P_NET) && (lhdr.dst_key == network_id)); }
    bool is_vdp83820_pkt(torus_dmareg_t::dma_sw_src_key_t src_key) { return src_key.network == network_id; }
    
    word_t transmit();
    
    bool receive_pkt(torus_dmareg_t::dma_sw_src_key_t, u8_t *, word_t,  word_t);
    bool receive_pkt(tree_packet_t::header_t, tree_packet_t::lnk_hdr_t &);

};

