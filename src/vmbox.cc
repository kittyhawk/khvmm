/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vmbox.cc
 * Description:     Virtual BlueGene SRAM Mailbox
 *
 * All rights reserved
 *
 ********************************************************************/

#include "vmbox.h"

void vmbox_t::alert()
{
    switch (cmd) {

        case TO_HOST_PRINT:
            for (unsigned i = 0; i < len; i++)
                putc (payload[i]);
            break;

        default:
            printf ("%s: unknown mbox alert cmd:%x len:%x\n", __PRETTY_FUNCTION__, cmd, len);
            return;
    }

    result = SUCCESS;

    cmd |= 0x8000;
}
