/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vdma.h
 * Description:     Virtual BlueGene DMA
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class vdma_t
{
    public:
        enum Register
        {
            INJ_FIFO_START              = 0x0,
            INJ_FIFO_END                = 0x1f,

            INJ_FIFO_NOT_EMPTY          = 0x200,
            INJ_FIFO_AVAILABLE          = 0x208,
            INJ_FIFO_THRESHOLD          = 0x210,
            INJ_FIFO_THRESHOLD_CLR      = 0x218,
            INJ_FIFO_ACTIVATED          = 0x220,
            INJ_FIFO_ACTIVATE           = 0x224,
            INJ_FIFO_DEACTIVATE         = 0x228,

            INJ_CNTR_ENABLED            = 0x304,    // and 0x308
            INJ_CNTR_ENABLE             = 0x308,    // and 0x30c
            INJ_CNTR_DISABLE            = 0x310,    // and 0x314
            INJ_CNTR_ZERO               = 0x320,    // and 0x324
            INJ_CNTR_ZERO_CLR           = 0x328,    // and 0x32c
            INJ_CNTR_GROUP_STS          = 0x330,
        };

        vdma_t() : reset (0xffffffff),
                   base_control (0) {}

        void dcr_read   (word_t, word_t *);
        void dcr_write  (word_t, word_t);

        word_t gpr_load     (Register, word_t *);
        word_t gpr_store    (Register, word_t *);

    private:
        /*
         * Structure layout must correspond exactly to the order of DCRs
         * See: arch/include/bpcore/bgp_dma_dcr.h
         */
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
            word_t  dcr[];
        };
};
