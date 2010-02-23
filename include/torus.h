/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vtorus.h
 * Description:     Generic BlueGene DMA Device
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class torus_packet_t
{
public:
    typedef u8_t coordinate_t;
    enum Axes { XAX=0, YAX=1, ZAX=2, AXES=3, NUM_COORD = 1ul << (8 * sizeof(coordinate_t)) };
    typedef coordinate_t coordinates_t[AXES];
    
    
    // Fifo header
    class header_t 
    {
    public:
        union {
            u8_t raw8[8];
            u32_t raw32[2];
            struct {
                u32_t csum_skip         : 7;	// number of shorts to skip in chksum
                u32_t sk		: 1;	// 0= use csum_skip, 1 skip pkt
                u32_t dirhint		: 6;	// x-,x+,y-,y+,z-,z+
                u32_t deposit		: 1;	// multicast deposit
                u32_t pid0		: 1;	// destination fifo group MSb
                u32_t size		: 3;	// size: (size + 1) * 32bytes
                u32_t pid1		: 1;	// destination fifo group LSb
                u32_t dma		: 1;	// 1=DMA mode, 0=Fifo mode
                u32_t dyn_routing	: 1;	// 1=dynamic routing, 0=deterministic routing
                u32_t virt_channel	: 2;	// channel (0=Dynamic CH0, 1=Dynamic CH1, 2=Bubble, 3=Prio)
                u32_t dest_x		: 8;
                u32_t dest_y		: 8;
                u32_t dest_z		: 8;
                u32_t reserved          : 16;
            };
        } PACKED;

        coordinates_t &get_dest()
            { return (coordinates_t &) raw8[3]; }
        
        void set(word_t p, coordinates_t d)
            {
                sk = 1;		// skip checksum
                size = 7;       // always full 240 bytes packets
                pid0 = (p >> 1) & 0x1;
                pid1 = p & 0x1;
                dyn_routing = 1;
                dest_x = d[0];
                dest_y = d[1];
                dest_z = d[2];
            }
        word_t get_size() { return ((size + 1) * 32) - 16; } 
   };

    
    class lnk_hdr_t
    {
    public:
        u32_t dst_key;
        u32_t len;
        u16_t lnk_proto;  // net, con, ...
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
        } opt;
    } ;

};

class torus_dmareg_t : public torus_packet_t
{
public:
    enum DmaDcr
    {
        // -------------------
        // ---- Controls -----
        // -------------------

        DMA_RESET                               = 0x00, // All bits reset to 1.
        DMA_BASE_CONTROL                        = 0x01, 

        iDMA_MIN_VALID_ADDR0                    = 0x02,  // + 2*g, g in the interval [0:7]:
        iDMA_MAX_VALID_ADDR0                    = 0x03,  //  32bit 16Byte aligned Physical Addresses containing (0..3 of UA | 0..27 of PA).

                 
        iDMA_INJ_RANGE_TLB                      = 0x12,
        rDMA_REC_RANGE_TLB                      = 0x13,

        rDMA_MIN_VALID_ADDR0                    = 0x14, //  32bit 16Byte aligned Physical Addresses containing (0..3 of UA | 0..27 of PA).
        rDMA_MAX_VALID_ADDR0                    = 0x15, // +2g, g in the interval [0:7]
                


        iDMA_TS_FIFO_WM0                        = 0x24, // +j, j in the interval [0:1]


        iDMA_LOCAL_FIFO_WM_RPT_CNT_DELAY        = 0x26,

        rDMA_TS_FIFO_WM0                        = 0x27, // +p, p in the interval [0:3]

        rDMA_LOCAL_FIFO_WM_RPT_CNT_DELAY        = 0x2b,
            
        iDMA_FIFO_ENABLE0                       = 0x2c, // +i,i in the interval [0:4]
        rDMA_FIFO_ENABLE                        = 0x30,	// each bit, if '1', enables a reception fifo
        rDMA_FIFO_ENABLE_HEADER                 = 0x31,

        iDMA_FIFO_PRIORITY0                     = 0x32, // +i, i in the interval [0:3]
        iDMA_FIFO_RGET_THRESHOLD                = 0x36,
        iDMA_SERVICE_QUANTA                     = 0x37,

        rDMA_FIFO_TYPE                          = 0x38,
        rDMA_FIFO_TYPE_HEADER                   = 0x39,

        rDMA_FIFO_THRESH0                       = 0x3a,
        rDMA_FIFO_THRESH1                       = 0x3b,

        iDMA_TS_INJ_FIFO_MAP0                   = 0x3c,  // +k, 8 bits for every dma injection fifo,  k in the interval [0:31]
        iDMA_LOCAL_COPY0                        = 0x5c,  // +i one bit for every dma injection fifo, i in the interval [0:3]
                

        rDMA_TS_REC_FIFO_MAP_G0_PID00_XY	= 0x60, // torus recv group 0, (pid0, pid1) = "00"
        rDMA_TS_REC_FIFO_MAP_G0_PID00_ZHL	= 0x61, // XP		((x & 0xFF) << 24)
        rDMA_TS_REC_FIFO_MAP_G0_PID01_XY	= 0x62, // XM		((x & 0xFF) << 16)
        rDMA_TS_REC_FIFO_MAP_G0_PID01_ZHL	= 0x63, // YP		((x & 0xFF) <<  8)
        rDMA_TS_REC_FIFO_MAP_G1_PID10_XY        = 0x64, // YM 		((x & 0xFF) <<  0)
        rDMA_TS_REC_FIFO_MAP_G1_PID10_ZHL       = 0x65, // ZP 		((x & 0xFF) << 24)
        rDMA_TS_REC_FIFO_MAP_G1_PID11_XY        = 0x66, // ZM 		((x & 0xFF) << 16)
        rDMA_TS_REC_FIFO_MAP_G1_PID11_ZHL       = 0x67, // HIGH		((x & 0xFF) <<  8)
                                                        // LOCAL	((x & 0xFF) <<  0)
            
        rDMA_FIFO_CLEAR_MASK0                   = 0x68, // +ii, ii in the interval [0:3]  group 0, group 1, ..., group 3
        rDMA_FIFO_HEADER_CLEAR_MASK             = 0x6c,
 
        iDMA_FIFO_INT_ENABLE_GROUP0		= 0x6d, // +g, g in the interval [0:3]  group 0, group 1, group2, and group 3
        rDMA_FIFO_INT_ENABLE_TYPE0		= 0x71, // +t, t in the interval [0:3]  type 0, type 1, ..., type 3
        rDMA_HEADER_FIFO_INT_ENABLE		= 0x75,


        iDMA_COUNTER_INT_ENABLE_GROUP0          = 0x76, // +g, g in the interval [0:3]  group 0, group 1, ..., group 3
        rDMA_COUNTER_INT_ENABLE_GROUP0          = 0x7a, // +g, g in the interval [0:3]  group 0, group 1, ..., group 3

        // ----------------------------
        // ---- Fatal Error Enables -----
        // ----------------------------
        
        DMA_FATAL_ERROR_ENABLE0                 = 0x7e, // +e, e in the interval [0:3], bit definition in the fatal errors at = 0x93 - = 0x96

        // -------------------------------
        // ---- Backdoor Access Regs -----
        // -------------------------------
        DMA_LF_IMFU_DESC_BD_CTRL                = 0x82,
        DMA_LF_IMFU_DESC_BACKDOOR_WR_DATA9      = 0x83,   // +i, 128 bit backdoor write data
        DMA_ARRAY_BD_CTRL			= 0x87,  // fifo/counter array backdoor control

        // -------------------------------------
        // ---- Torus Link Checker Control -----
        // -------------------------------------
        DMA_TS_LINK_CHK_CTRL			= 0x88,

        // --------------------
        // ---- Threshold -----
        // --------------------
        DMA_CE_COUNT_THRESHOLD			= 0x89, // correctable ecc error count threshold, reset to = 0xFFFFFFFF

        // ----------------------------------
        // ---- Correctable error count -----
        // ----------------------------------

        DMA_CE_COUNT0                           = 0x8A, // +c, c in the interval [0:8]  count 0, count 1, ..., count 8
        DMA_CE_COUNT_INJ_FIFO0			= 0x8A,
        DMA_CE_COUNT_INJ_FIFO1			= 0x8B,
        DMA_CE_COUNT_INJ_COUNTER		= 0x8C,
        DMA_CE_COUNT_INJ_DESC                   = 0x8D,
        DMA_CE_COUNT_REC_FIFO0                  = 0x8E,
        DMA_CE_COUNT_REC_FIFO1                  = 0x8F,
        DMA_CE_COUNT_REC_COUNTER                = 0x90,
        DMA_CE_COUNT_LOCAL_FIFO0                = 0x91,
        DMA_CE_COUNT_LOCAL_FIFO1                = 0x92,

        // -----------------
        // ---- Status -----
        // -----------------
        
        DMA_FATAL_ERROR0                        = 0x93, // +e in the interval [0:3]  error0, error1, ..., error 3
        DMA_PQUE_WR0_BAD_ADDR                   = 0x97,
        DMA_PQUE_RD0_BAD_ADDR                   = 0x98,
        DMA_PQUE_WR1_BAD_ADDR                   = 0x99,
        DMA_PQUE_RD1_BAD_ADDR                   = 0x9a,
        DMA_MFU_STAT0                           = 0x9b,
        DMA_MFU_STAT1                           = 0x9c,
        DMA_MFU_STAT2                           = 0x9d,
        DMA_L3_RD_ERROR_ADDR                    = 0x9e,
        DMA_L3_WR_ERROR_ADDR                    = 0x9f,


        DMA_LF_IMFU_DESC_BD_RD_DATA0		= 0xa0, // +i, i in the interval [0:3]
        DMA_LF_IMFU_DESC_BD_RD_ECC              = 0xa4,
        DMA_ARRAY_RD_ECC                        = 0xa5,
        DMA_TS_LINK_CHK_STAT                    = 0xa6,

        // ---- Debug -----
        
        DMA_iFIFO_DESC_RD_FLAG0                 = 0xa7, // +i, i in the interval [0:3]
        DMA_INTERNAL_STATE0                     = 0xab,  // +j, j in the interval [0:1]

        DMA_PQUE_POINTER                        = 0xad,
        DMA_LOCAL_FIFO_POINTER                  = 0xae,

        // ---- Clears -----
        DMA_CLEAR0                              = 0xb1,
        DMA_CLEAR1                              = 0xb2,

    };
    
    enum {
        DMA_BASE_CONTROL_USE_DMA     = 1ul << (31 -0),           // Use DMA and *not* the Torus if 1, reset state is 0.
        DMA_BASE_CONTROL_STORE_HDR   = 1ul << (31 -1),           // Store DMA Headers in Reception Header Fifo (debugging)
        DMA_BASE_CONTROL_PF_DIS      = 1ul << (31 -2),           // Disable Torus Prefetch Unit (should be 0)
        DMA_BASE_CONTROL_L3BURST_EN  = 1ul << (31 -3),           // Enable L3 Burst when 1 (should be enabled, except for debugging)
        DMA_BASE_CONTROL_ITIU_EN     = 1ul << (31 -4),           // Enable Torus Injection Data Transfer Unit (never make this zero)
        DMA_BASE_CONTROL_RTIU_EN     = 1ul << (31 -5),           // Enable Torus Reception Data Transfer Unit
        DMA_BASE_CONTROL_IMFU_EN     = 1ul << (31 -6),           // Enable DMA Injection Fifo Unit Arbiter
        DMA_BASE_CONTROL_RMFU_EN     = 1ul << (31 -7),           // Enable DMA Reception fifo Unit Arbiter
        DMA_BASE_CONTROL_L3PF_DIS    = 1ul << (31 -8),           // Disable L3 Read Prefetch (should be 0)
    };

    class dma_hw_header_t 
    {
    public:
        union 
        {
            u32_t raw32[4];
            struct 
            {
                u32_t			: 30;
                u32_t prefetch		: 1;
                u32_t local_copy        : 1;
                u32_t			: 24;
                u32_t counter		: 8;
                u32_t base;
                u32_t length;
            };
        } PACKED;

        void set(word_t c, paddr_t b, word_t l)
            {
                prefetch = local_copy = 0;
                counter = c;
                base = b;
                length = l;
            }
              
    };
    
    class dma_sw_src_key_t
    {
    public:
        union 
        {
            word_t raw;
            struct
            {
                u8_t pkt;
                union {
                    coordinates_t coord;
                    struct 
                    {
                        u8_t  network;
                        u16_t node;
                    } PACKED;
                }PACKED;
            };
        };
        
        void init(word_t nw, word_t no) 
            { assert(no < 0x10000 && nw < 256); inc_pkt(); network = nw; node = no; }
        
        void init(coordinates_t cid) 
            { inc_pkt(); coord[0]=cid[0]; coord[1]=cid[1]; coord[2]=cid[2];}
        
    private: 
        void inc_pkt() { static u8_t _pkt = 0;  pkt = _pkt++; }

    };
        
    class dma_sw_header_t
    {
    public:
        union {
            u32_t raw32[2];
            struct 
            {
                u32_t offset;
                union 
                {
                    struct 
                    {
                        u8_t counter_id;
                        u8_t bytes;
                        u8_t unused1    : 6;
                        u8_t pacing	: 1;
                        u8_t remote_get	: 1;
                        u8_t unused2;
                    } dput;
                    dma_sw_src_key_t src_key;   
                };
            };
        } PACKED;
    };

    union inj_desc_t {
        u32_t raw32[8];
        struct {
            dma_hw_header_t  dma_hw;
            header_t         fifo;
            dma_sw_header_t  dma_sw;
        };
    } PACKED;

    union rcv_desc_t {
        u32_t raw32[256 / sizeof(u32_t)];
        u8_t raw8[256];
        struct {
            header_t            fifo;
            dma_sw_header_t     dma_sw;
            u8_t                data[];
        };
    } PACKED;

    // FIFO and counter depth 
    static unsigned const nr_inj_fifos = 32;
    static unsigned const nr_rcv_fifos =  9;
    static unsigned const nr_counters  = 64;

};

class torus_dma_t : public torus_dmareg_t
{
protected:
    const word_t group;
    
public:
    
    enum Register
    {
        INJ_FIFO0                   = 0x0,    // + 16*f, f in the interval [0:31]
        
        INJ_FIFO_NOT_EMPTY          = 0x200,
        INJ_FIFO_AVAILABLE          = 0x208,
        INJ_FIFO_THRESHOLD          = 0x210,
        INJ_FIFO_THRESHOLD_CLR      = 0x218,
        INJ_FIFO_DMA_ACTIVATED      = 0x220,
        INJ_FIFO_DMA_ACTIVATE       = 0x224,
        INJ_FIFO_DEACTIVATE         = 0x228,
        
        INJ_CNTR_CNF                = 0x300,  // + 4*s, s in the interval [0:1]
        INJ_CNTR_GROUP_STS          = 0x330,
        
        INJ_COUNTER0                = 0x400, // +16*c, c in the interval [0:63]

        RCV_FIFO0                   = 0x800, // + 16*f, f in the interval [0:8]

        GLOBAL_INTS0                = 0x900, // + 4*i, i in the interval [0:15]

        RCV_FIFO_NOT_EMPTY0         = 0xa00, // + 4*s, s in the interval [0:1]
        RCV_FIFO_AVAILABLE0         = 0xa08, // + 4*s, s in the interval [0:1]
        RCV_FIFO_THRESHOLD0         = 0xa10, // + 4*s, s in the interval [0:1]
        RCV_FIFO_THRESHOLD_CLR0     = 0xa18, // + 4*s, s in the interval [0:1]
        
        RCV_CNTR_CNF                = 0xb00, // + 4*s, s in the interval [0:1]
        RCV_CNTR_GROUP_STS          = 0xb30,
        
        RCV_COUNTER0                = 0xc00, // +16*c, c in the interval [0:63]
        
        INJ_FIFO_SIZE               = 0x200,
        RCV_FIFO_SIZE               = 0x090,
        INJ_COUNTER_SIZE            = 0x400,
        RCV_COUNTER_SIZE            = 0x400,
     
    };
    
    enum FifoRegister 
    {
        FIFO_START                  = 0x0,
        FIFO_END                    = 0x4,
        FIFO_HEAD                   = 0x8,
        FIFO_TAIL                   = 0xc,
    };

    enum CtrRegister 
    {
        CTR_CTR                     = 0x0,
        CTR_INC                     = 0x4,
        CTR_BASE                    = 0x8,
        CTR_LIMIT                   = 0xc,
    };

    enum CnfRegister 
    {
        CNF_ENABLED                 = 0x00,  // + 4*s, s in the interval [0:1]
        CNF_ENABLE                  = 0x08,  // + 4*s, s in the interval [0:1]
        CNF_DISABLE                 = 0x10,  // + 4*s, s in the interval [0:1]
        CNF_HITZERO                 = 0x20,  // + 4*s, s in the interval [0:1]
        CNF_HITZERO_CLR             = 0x28,  // + 4*s, s in the interval [0:1]
    };
        
    static const word_t INJ_FIFO_IDX(const Register reg) 
        {  return ((reg - INJ_FIFO0) / sizeof(fifo_t)); }
    static const word_t RCV_FIFO_IDX(const Register reg) 
        { return ((reg - RCV_FIFO0) / sizeof(fifo_t)); }
    
    static const word_t INJ_FIFO_REG(const Register reg) 
        {  return ((reg - INJ_FIFO0) / sizeof(word_t)); }
    static const word_t RCV_FIFO_REG(const Register reg) 
        { return ((reg - RCV_FIFO0) / sizeof(word_t)); }
    static const word_t INJ_CTR_IDX(const Register reg) 
        {  return ((reg - INJ_COUNTER0) / sizeof(counter_t)); }
    static const word_t RCV_CTR_IDX(const Register reg) 
        { return ((reg - RCV_COUNTER0) / sizeof(counter_t)); }
    static const Register INJ_FIFO_REG(const word_t fifo, const FifoRegister reg) 
        {  return Register(INJ_FIFO0 + (sizeof(fifo_t) * fifo) + reg);  }
    static const Register RCV_FIFO_REG(const word_t fifo, const FifoRegister reg) 
        {  return Register(RCV_FIFO0 + (sizeof(fifo_t) * fifo) + reg);  }
    static const Register INJ_CTR_REG(const word_t ctr, CtrRegister reg) 
        {  return Register(INJ_COUNTER0 + (sizeof(counter_t) * ctr) + reg);  }
    static const Register RCV_CTR_REG(const word_t ctr, CtrRegister reg) 
        {  return Register(RCV_COUNTER0 + (sizeof(counter_t) * ctr) + reg); }

    static const Register INJ_CNTR_CNF_REG(const word_t ctr, CnfRegister reg) 
        { return Register(INJ_CNTR_CNF + reg +  4 * (ctr / 32));  }
    static const Register RCV_CNTR_CNF_REG(const word_t ctr, CnfRegister reg) 
        { return Register(RCV_CNTR_CNF + reg +  4 * (ctr / 32));  }

    static const word_t INJ_CTR_MASK(const word_t ctr) 
        {  return (0x80000000 >> (ctr % 32)); }
    
    const word_t inj_ctr_id(const word_t ctr) 
        {  return (group * nr_counters) + ctr; } 
    const word_t inj_ctr_subgroup(const word_t ctr) 
        {  return ((group * nr_counters) + ctr) / 8; }
    const word_t rcv_fifo_id(const word_t fifo)
        { return (group * (nr_rcv_fifos-1)) + fifo; }
    const word_t rcv_fifo_normal_id(const word_t fifo)
        { return (group * (nr_rcv_fifos-1)) + fifo; }

   
    struct fifo_t {
        u32_t start;
        u32_t end;
        u32_t head;
        u32_t tail;
    };
        
    struct counter_t {
        volatile u32_t counter;
        volatile u32_t increment;
        u32_t base;
        u32_t limit;
    };
    
    enum Dir {
        dir_xplus = 0x20,
        dir_xminus = 0x10,
        dir_yplus = 0x08,
        dir_yminus = 0x04,
        dir_zplus = 0x02,
        dir_zminus = 0x01
    };
    
   
    torus_dma_t (word_t g) : group (g) {}
    
    
};

class torus_t : public torus_dmareg_t
{
public:
    static unsigned const nr_dma_groups  =  4;
    static unsigned const nr_dma_regions =  8;
    static unsigned const nr_pids        =  4;
    
    // Default torus fifo map
    static unsigned const default_fifomap = 0x80;

    enum DcrBase
    {
        // Tree (0xc00-0xc7f)
        DCRBASE                    = 0xc80,
        DCRBASE_MIN                = DCRBASE,
        DCRBASE_MAX                = DCRBASE_MIN + 0x7f,
        
        DCRBASE_DMA                = 0xd00,
        DCRBASE_DMA_MIN            = DCRBASE_DMA,
        DCRBASE_DMA_MAX            = DCRBASE_DMA_MIN + 0xff,

    };
    
    enum Dcr
    {
        DCR_TS_CTRL                     = 0x00, // Control Settings (R/W)
        TS_CTRL_RESET                   = 0x00, // CTRL: Reset Register
        TS_CTRL_NODES                   = 0x01, // CTRL: Machine Nodes (Coordinate means number of nodes -1)
        TS_CTRL_COORDS_NODE             = 0x02, // CTRL: Node Coordinates
        TS_CTRL_COORDS_PLUS             = 0x03, // CTRL: Plus-Neighbor's Coordinates
        TS_CTRL_COORDS_MINUS            = 0x04, // CTRL: Minus-Neighbor's Coordinates
        TS_CTRL_HINT_PLUS               = 0x05, // CTRL: Hint Bit Cutoff Plus
        TS_CTRL_HINT_MINUS              = 0x06, // CTRL: Hint Bit Cutoff Minus
        TS_CTRL_VCTHBD                  = 0x07, // CTRL: Receiver VC Threshold for Bypass Disable
        TS_CTRL_RETRANS_TOX             = 0x08, // CTRL: Retransmission Time Outs X+ and X-
        TS_CTRL_RETRANS_TOY             = 0x09, // CTRL: Retransmission Time Outs Y+ and Y-
        TS_CTRL_RETRANS_TOZ             = 0x0A, // CTRL: Retransmission Time Outs Z+ and Z
        TS_CTRL_STOPS_RAND              = 0x0B, // CTRL: Sender Stops and Random Number Generators
        TS_CTRL_CLK_RATIO               = 0x0C, // CTRL: Clock_ts to Clock_x2 Ratio
        TS_CTRL_WM_G0                   = 0x0D, // CTRL: WaterMarks for Fifo Group 0 (units=32B, 0=Empty, 0x20=Full)
        TS_CTRL_WM_G1                   = 0x0E, // CTRL: WaterMarks for Fifo Group 1 (units=32B, 0=Empty, 0x20=Full)
        TS_CTRL_SRAM_WD_0_31            = 0x0F, // CTRL: SRAM Backdoor Write-Data bits 0  - 31
        TS_CTRL_SRAM_WD_32_63           = 0x10, // CTRL: SRAM Backdoor Write-Data bits 32 - 63
        TS_CTRL_SRAM_WD_CTRL            = 0x11, // CTRL: SRAM Backdoor Write-Data Control
        TS_CTRL_LINKS                   = 0x12, // CTRL: Link Control (LoopBack and Receiver/Retransmisstion Stops
        TS_CTRL_FIFO                    = 0x13, // CTRL: FIFO Read Control

        TS_THR                          = 0x16,  // Threshold Settings (R/W)
        TS_THR_RECV_RETRY               = 0x16,  // THR: Receiver Retry Interrupt Threshold
        TS_THR_SRAM_ECC                 = 0x17,  // THR: Sender SBE SRAM 0,1 Interrupt Threshold
        TS_THR_SEND_RETRY               = 0x18,  // THR: Sender 0-5 Resend Attempts
        TS_THR_RAND_ERR                 = 0x19,  // THR: Random Number Generator Non-Fatal Error
        TS_THR_RECV_PIPE                = 0x1a,  // THR: Any Receiver PipeLine non-CRC Error Threshold
        TS_THR_VCFIFO_OVERFLOW          = 0x1b,  // THR: Chunks dropped in a VC Fifo because no space. (Header ECC Escape Error)

        TS_CLR                          = 0x1E,  // Clear Controls (W)
        
        TS_NREM                         = 0x20,  // Non-Recoverable Error Masks (R/W)
        TS_NREM_RXP                     = 0x20,  // NREM: X+ Receiver Error Mask
        TS_NREM_RXM                     = 0x21,  // NREM: X- Receiver Error Mask
        TS_NREM_RYP                     = 0x22,  // NREM: Y+ Receiver Error Mask
        TS_NREM_RYM                     = 0x23,  // NREM: Y- Receiver Error Mask
        TS_NREM_RZP                     = 0x24,  // NREM: Z+ Receiver Error Mask
        TS_NREM_RZM                     = 0x25,  // NREM: Z- Receiver Error Mask
        TS_NREM_SHDR_CHK                = 0x26,  // NREM: Sender Header Check Error  Mask
        TS_NREM_SPAR_SRAM               = 0x27,  // NREM: Sender Parity and SRAM ECC Double-Bit Mask
        TS_NREM_INJ_FIFO_G0             = 0x28,  // NREM: Injection Fifos Group 0 Mask
        TS_NREM_INJ_FIFO_G1             = 0x29,  // NREM: Injection Fifos Group 1 Mask
        TS_NREM_INJ_MEM                 = 0x2A,  // NREM: Injection Memory and Latch Error Mask
        TS_NREM_REC_MEM                 = 0x2B,  // NREM: Reception Memory and Latch Error Mask
        TS_NREM_DCR                     = 0x2C,  // NREM: DCR Error Mask

        TS_REM                          = 0x30,  // Recoverable Error Masks (RC/W)
        TS_REM_MASKS                    = 0x30,  // REM: Recoverable Error Masks
        TS_REM_Resvd                    = 0x03,  // REM: Reserved
        
        TS_STAT                         = 0x32,  // Status Info (R)
        TS_STAT_REC_CRC_XP              = 0x32,  // STAT: Receiver Link CRC X+
        TS_STAT_REC_CRC_XM              = 0x33,  // STAT: Receiver Link CRC X-
        TS_STAT_REC_CRC_YP              = 0x34,  // STAT: Receiver Link CRC Y+
        TS_STAT_REC_CRC_YM              = 0x35,  // STAT: Receiver Link CRC Y-
        TS_STAT_REC_CRC_ZP              = 0x36,  // STAT: Receiver Link CRC Z+
        TS_STAT_REC_CRC_ZM              = 0x37,  // STAT: Receiver Link CRC Z-
        TS_STAT_REC_FULL_XP             = 0x38,  // STAT: Receiver VC Fullness X+
        
        TS_NRE                          = 0x5C,  // Non-Recoverable Errors (RO, needs TS reset to clear!)
        TS_NRE_RXP                      = 0x5c,  // NRE: X+ Receiver Errors
        TS_NRE_RXM                      = 0x5d,  // NRE: X- Receiver Errors
        TS_NRE_RYP                      = 0x5e,  // NRE: Y+ Receiver Errors
        TS_NRE_RYM                      = 0x5f,  // NRE: Y- Receiver Errors
        TS_NRE_RZP                      = 0x60,  // NRE: Z+ Receiver Errors
        TS_NRE_RZM                      = 0x61,  // NRE: Z- Receiver Errors
        TS_NRE_SHDR_CHK                 = 0x62,  // NRE: Sender Header Check Error
        TS_NRE_SPAR_SRAM                = 0x63,  // NRE: Sender Parity and SRAM ECC Double-Bit
        TS_NRE_INJ_FIFO_G0              = 0x64,  // NRE: Injection Fifos Group 0
        TS_NRE_INJ_FIFO_G1              = 0x65,  // NRE: Injection Fifos Group 1
        TS_NRE_INJ_MEM                  = 0x66,  // NRE: Injection Memory and Latch Errors
        TS_NRE_REC_MEM                  = 0x67,  // NRE: Reception Memory and Latch Errors
        TS_NRE_DCR                      = 0x68,  // NRE: DCR Errors
        
        TS_RE                           = 0x6C,  // Recoverable Errors (RC/W)
        TS_RE_INPUT_XP                  = 0x6d,  // RE: Receiver Input Pipe Errors X+
        TS_RE_INPUT_XM                  = 0x6e,  // RE: Receiver Input Pipe Errors X-
        TS_RE_INPUT_YP                  = 0x6f,  // RE: Receiver Input Pipe Errors Y+
        TS_RE_INPUT_YM                  = 0x70,  // RE: Receiver Input Pipe Errors Y-
        TS_RE_INPUT_ZP                  = 0x71,  // RE: Receiver Input Pipe Errors Z+
        TS_RE_INPUT_ZM                  = 0x72,  // RE: Receiver Input Pipe Errors Z-
        TS_RE_SEND_SRAM_SBE             = 0x73,  // RE: Sender Single Bit Errors in SRAM 0,1
        TS_RE_VCFIFO_SRAM_SBE           = 0x74,  // RE: Receiver VC Fifo SRAM Single Bit Errors
        TS_RE_SEND_RETRY_XP             = 0x75,  // RE: Sender Retry Errors X+
        TS_RE_SEND_RETRY_XM             = 0x76,  // RE: Sender Retry Errors X-
        TS_RE_SEND_RETRY_YP             = 0x77,  // RE: Sender Retry Errors Y+
        TS_RE_SEND_RETRY_YM             = 0x78,  // RE: Sender Retry Errors Y-
        TS_RE_SEND_RETRY_ZP             = 0x79,  // RE: Sender Retry Errors Z+
        TS_RE_SEND_RETRY_ZM             = 0x7a,  // RE: Sender Retry Errors Z-
        TS_RE_RAND                      = 0x7b,  // RE: Corrected Random Number Generator Errors
        TS_RE_INJ_SRAM_SBE              = 0x7c,  // RE: Injection Single Bit Errors in all Fifo SRAMs
        TS_RE_REC_SRAM_SBE              = 0x7d,  // RE: Reception Single Bit Errors in all Fifo SRAMs
        TS_RE_BADHDR_FULLFIFO           = 0x7e,  // RE: Counts Bad Packet Header Check or Attempt to Write to Full Fifo
        TS_RE_VCFIFO_OVERFLOW           = 0x7f,  // RE: Counts dropped chunks due to attempt to write to full fifo (header CRC escape)


    };
    
    enum Irq
    {
        irq_fatal_err             = 0x20,   // torus fatal error 
        irq_cnt_thr_min           = 0x21,   // torus count thresh
        irq_cnt_thr_max           = 0x34,   // 
        irq_dma_cnt_thr_min       = 0x36,   // dma count thresh 
        irq_dma_cnt_thr_max       = 0x3e,   
        irq_dma_fatal_err         = 0x3f,   // dma fatal error 31
        irq_inj_fifo_wm_min       = 0x40,   // inj fifo watermark 0-7
        irq_inj_fifo_wm_max       = 0x47,
        irq_rcv_fifo_wm_min       = 0x48,   // inj fifo watermark 0-7
        irq_rcv_fifo_wm_max       = 0x55,   // invalid read group 0/1 22/23
        irq_rcv_fifo_inv_rd_min   = 0x56,
        irq_rcv_fifo_inv_rd_max   = 0x57,
        irq_inj_fifo_thr_min      = 0x58,   // inj fifo thresh 24-27
        irq_inj_fifo_thr_max      = 0x5b,
        irq_rcv_fifo_thr_min      = 0x5c,   // rcv fifo thresh 28-31
        irq_rcv_fifo_thr_max      = 0x5f,
        irq_inj_ctr_zero_min      = 0x60,   // inj counter zero 32-35
        irq_inj_ctr_zero_max      = 0x63,
        irq_rcv_ctr_zero_min      = 0x64,   // rcv counter zero 36-39
        irq_rcv_ctr_zero_max      = 0x67,
        irq_inj_ctr_err_min       = 0x68,   // inj counter zero 32-35
        irq_inj_ctr_err_max       = 0x6b,
        irq_rcv_ctr_err_min       = 0x6c,   // rcv counter zero 44-46
        irq_rcv_ctr_err_max       = 0x6e,
        irq_dma_write_err_min     = 0x6f,   // DMA write error 47-52
        irq_dma_write_err_max     = 0x74,
        irq_link_check_pkt        = 0x75,   // link check pkt 53
        irq_dd2_err_min           = 0x76,   
        irq_dd2_err_max           = 0x79,   

        irq_min                   = 0x20,
        irq_max                   = 0x79,

    };

    static bool is_irq(const word_t irq) 
        { return irq >= irq_min && irq <= irq_max; }

    /*
     * Structure layout must correspond exactly to the order of DCRs
     * See: arch/include/bpcore/bgp_torus_dcr.h
     */
    class dmadcr_t
    {
    public:
        union {
            struct {
                word_t  reset;
                word_t  base_control;
                struct {
                    word_t  min, max;
                }       inj_valid_addr[8];
                word_t  inj_range_tlb;
                word_t  rec_range_tlb;
                struct {
                    word_t min, max;
                }       rec_valid_addr[8];
                word_t  inj_fifo_thresh_torus[2];
                word_t  inj_fifo_thresh_local;
                word_t  rec_fifo_thresh_torus[4];
                word_t  rec_fifo_thresh_local;
                word_t  inj_fifo_enable[4];
                word_t  rec_fifo_enable;
                word_t  rec_fifo_enable_header;
                word_t  inj_fifo_prio[4];
                word_t  inj_fifo_thresh;
                word_t  inj_serv_quanta;
                word_t  rec_fifo_type;
                word_t  rec_fifo_type_header;
                word_t  rec_fifo_thresh[2];
                word_t  inj_fifo_map_torus[32];
                word_t  inj_fifo_map_local[4];
                word_t  rec_fifo_map[8];
                word_t  rec_fifo_clear[4];
                word_t  rec_fifo_clear_header;
                word_t  inj_fifo_int_enable_group[4];
                word_t  rec_fifo_int_enable_type[4];
                word_t  rec_fifo_int_enable_header;
                word_t  inj_cntr_int_enable_group[4];
                word_t  rec_cntr_int_enable_group[4];
                word_t  fatal_error_enable[4];
                word_t  pad1[8];
                word_t  correctable_error[9];
                word_t  fatal_error[4];
                word_t  pad2[26];
                word_t  clear[2];
            };
            word_t  regs[];
        };
        
        dmadcr_t() : reset (0xffffffff),
                     base_control (0) {}
    };
    
    torus_t () 
        { 
            assert(sizeof(torus_packet_t::header_t) == 8);
            assert(sizeof(torus_packet_t::lnk_hdr_t) == 12); 
            assert(sizeof(torus_dmareg_t::dma_hw_header_t) == 16);
            assert(sizeof(torus_t::dma_sw_header_t) == 8);
        }


};
