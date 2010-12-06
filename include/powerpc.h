/*********************************************************************
 *                
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 *                
 * File path:     powerpc.h
 * Description:   
 *                
 * All rights reserved
 *                
 ********************************************************************/
#pragma once

#include "std.h"

/* TLB entry 0 */
#define PPC_TLB_VALID		(0x80000000 >> 22)
#define PPC_TLB_SPACE0		(0)
#define PPC_TLB_SPACE1		(0x80000000 >> 23)
#define PPC_TLB_SIZE(x)		(((x - 10)/2) << 4)
#define PPC_TLB_SIZE_1K		PPC_TLB_SIZE(10)
#define PPC_TLB_SIZE_4K		PPC_TLB_SIZE(12)
#define PPC_TLB_SIZE_16K	PPC_TLB_SIZE(14)
#define PPC_TLB_SIZE_64K	PPC_TLB_SIZE(16)
#define PPC_TLB_SIZE_256K	PPC_TLB_SIZE(18)
#define PPC_TLB_SIZE_1M		PPC_TLB_SIZE(20)
#define PPC_TLB_SIZE_16M	PPC_TLB_SIZE(24)
#define PPC_TLB_SIZE_256M	PPC_TLB_SIZE(28)
#define PPC_TLB_SIZE_1G		PPC_TLB_SIZE(30)

/* TLB entry 1 */
#define PPC_TLB_RPN(x)		((x) & 0xfffff300)
#define PPC_TLB_ERPN(x)		((x) & 0xf)

/* TLB entry 2 */
#define PPC_TLB_WL1		(0x80000000 >> 11)
#define PPC_TLB_IL1I		(0x80000000 >> 12)
#define PPC_TLB_IL1D		(0x80000000 >> 13)
#define PPC_TLB_IL2I		(0x80000000 >> 14)
#define PPC_TLB_IL2D		(0x80000000 >> 15)
#define PPC_TLB_U0		(0x80000000 >> 16)
#define PPC_TLB_U1		(0x80000000 >> 17)
#define PPC_TLB_U2		(0x80000000 >> 18)
#define PPC_TLB_U3		(0x80000000 >> 19)
#define PPC_TLB_WRITETHROUGH	(0x80000000 >> 20)
#define PPC_TLB_INHIBIT		(0x80000000 >> 21)
#define PPC_TLB_MEMCOHERENCY	(0x80000000 >> 22)
#define PPC_TLB_GUARDED		(0x80000000 >> 23)
#define PPC_TLB_ENDIAN		(0x80000000 >> 24)
#define PPC_TLB_UX		(0x80000000 >> 26)
#define PPC_TLB_UW		(0x80000000 >> 27)
#define PPC_TLB_UR		(0x80000000 >> 28)
#define PPC_TLB_SX		(0x80000000 >> 29)
#define PPC_TLB_SW		(0x80000000 >> 30)
#define PPC_TLB_SR		(0x80000000 >> 31)

#define PPC_MAX_TLB_ENTRIES	64

class ppc_instr_t
{
    private:
        union {
	        word_t raw;

        	struct {
	            word_t primary : 6;
	            word_t li : 26;
	        } iform;

	        struct {
	            word_t primary : 6;
	            word_t bo : 5;
	            word_t bi : 5;
	            word_t bd : 14;
	            word_t aa : 1;
	            word_t lk : 1;
	        } bform;

        	struct {
        	    word_t primary : 6;
        	    word_t     : 24;
        	    word_t one : 1;
        	    word_t     : 1;
	        } scform;

            struct {
	            word_t primary : 6;
	            word_t rt : 5;
	            word_t ra : 5;
	            word_t d : 16;
	        } dform;

            struct {
	            word_t primary : 6;
	            word_t rt : 5;
	            word_t ra : 5;
	            word_t rb : 5;
	            word_t xo : 10;
	            word_t rc : 1;
	        } xform;

            struct {
	            word_t primary : 6;
	            word_t bt : 5;
        	    word_t ba : 5;
        	    word_t bb : 5;
        	    word_t xo : 10;
        	    word_t lk : 1;
	        } xlform;

            struct {
	            word_t primary : 6;
	            word_t rt : 5;
	            word_t rf : 10;
	            word_t xo : 10;
	            word_t    : 1;
	        } xfxform;

            struct {
                word_t primary : 6;
                word_t rt : 5;
                word_t ra : 5;
                word_t rb : 5;
                word_t oe : 1;
                word_t xo : 9;
                word_t rc : 1;
            } xoform;

            struct {
                word_t primary : 6;
                word_t rs : 5;
                word_t ra : 5;
                word_t rb : 5;
                word_t mb : 5;
                word_t me : 5;
                word_t rc : 1;
            } mform;

        } __attribute((packed));

    public:
        ppc_instr_t() {}
        ppc_instr_t (word_t val) { raw = val; }

        unsigned primary() const    { return xform.primary; }
        unsigned secondary() const  { return xform.xo; }
        unsigned ra() const         { return xform.ra; }
        unsigned rb() const         { return xform.rb; }
        unsigned rt() const         { return xform.rt; }
        s16_t d() const             { return dform.d;  }
        unsigned rf() const         { return xform.rb << 5 | xform.ra; }
        word_t getRaw()				{ return raw; }
};


INLINE int lzw( word_t w ) __attribute__ ((const));
INLINE int lzw( word_t w )
{
    int zeros;
    asm volatile ("cntlzw %0, %1" : "=r" (zeros) : "r" (w) );
    return zeros;
}

INLINE void stwbrx( word_t val, word_t *addr)
{
    asm volatile( "stwbrx %0, 0, %1" 
                  : 
                  : "r" (val), "b" (addr)
                  : "memory" );
}
