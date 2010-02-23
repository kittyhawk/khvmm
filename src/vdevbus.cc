/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vdevbus.cc
 * Description:     Virtual BlueGene Ethernet Devbus
 *
 * All rights reserved
 *
 ********************************************************************/

#include "vdevbus.h"

word_t vdevbus_t::gpr_load (Register reg, word_t *gpr)
{
    printf ("%s: reg=%lx gpr=%p\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), gpr);

    *gpr = 0;       // for now

    return 0;
}

word_t vdevbus_t::gpr_store (Register reg, word_t *gpr)
{
    printf ("%s: reg=%lx val=%#lx\n", __PRETTY_FUNCTION__, static_cast<word_t>(reg), *gpr);

    return 0;
}
