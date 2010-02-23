/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vtest.cc
 * Description:     Virtual BlueGene Test Interface
 *
 * All rights reserved
 *
 ********************************************************************/

#include "config.h"
#include "vtest.h"

vtest_t::vtest_t()
{
    // Get a pointer to the the mailbox table in SRAM backing store 
    vmbox_table_t *table = new (SRAM_MAP + SRAM_MBOX_TABLE_OFFS) vmbox_table_t;

    for (unsigned i = 0; i < sizeof (mbox) / sizeof (*mbox); i++) {

        // Keep pointers to the mailboxes in SRAM backing store
        mbox[i].to_core = i ? 0 :
                          new (SRAM_MAP + SRAM_MBOX_TO_CORE_OFFS) vmbox_t;
        mbox[i].to_host = new (SRAM_MAP + SRAM_MBOX_TO_HOST_OFFS) vmbox_t;

        // Create mailbox descriptors in the mailbox table
        table->mbox[i].to_core = vmbox_desc_t (i ? 0 : SRAM_MBOX_TO_CORE_OFFS,
                                               i ? 0 : SRAM_MBOX_TO_CORE_SIZE);
        table->mbox[i].to_host = vmbox_desc_t (SRAM_MBOX_TO_HOST_OFFS +
                                               SRAM_MBOX_TO_HOST_SIZE * i,
                                               SRAM_MBOX_TO_HOST_SIZE);
    }
}

void vtest_t::dcr_read (Dcr dcr, word_t *gpr)
{
    printf ("%s: dcr=%#lx gpr=%p\n", __PRETTY_FUNCTION__, static_cast<word_t>(dcr), gpr);
}

void vtest_t::dcr_write (Dcr dcr, word_t val)
{
    switch (dcr) {

        case DCR_GLOB_ATT_WRITE_SET:
            for (unsigned i = 0; i < sizeof (mbox) / sizeof (*mbox); i++)
                if (val & GLOB_ATT_ALERT_OUT >> i)
                    if (mbox[i].to_host)
                        mbox[i].to_host->alert();
            return;

        case DCR_GLOB_ATT_WRITE_CLR:
            return;
    }

    printf ("%s: dcr=%#lx val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(dcr), val);
}
