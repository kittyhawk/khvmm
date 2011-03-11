/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vtest.h
 * Description:     Virtual BlueGene Test Interface
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"
#include "vmbox.h"

class vtest_t
{
    private:
        struct {
            vmbox_t *to_core;
            vmbox_t *to_host;
        } mbox[4];

        enum Value
        {
            GLOB_ATT_ALERT_OUT      = 1ul << 7, // Message to Host from Core
            GLOB_ATT_ALERT_IN       = 1ul << 3, // Interrupt to BIC
        };

    public:
        // See: arch/include/bpcore/bgp_test_int.h
        enum Dcr
        {
            DCR_GLOB_ATT_WRITE_SET  = 0x17,
            DCR_GLOB_ATT_WRITE_CLR  = 0x18,
        };

        vtest_t();

        void dcr_read   (Dcr, word_t *);
        void dcr_write  (Dcr, word_t);
};
