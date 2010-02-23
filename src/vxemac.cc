/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vxemac.cc
 * Description:     Virtual BlueGene Ethernet Media Access Controller
 *
 * All rights reserved
 *
 ********************************************************************/

#include "vxemac.h"

word_t vxemac_t::gpr_load (Register, word_t *)
{
    return 0;
}

word_t vxemac_t::gpr_store (Register, word_t *)
{
    return 0;
}
