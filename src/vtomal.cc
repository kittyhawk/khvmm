/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       vtomal.cc
 * Description:     Virtual BlueGene TCP/IP Offload Memory Access Layer
 *
 * All rights reserved
 *
 ********************************************************************/

#include "vtomal.h"

word_t vtomal_t::gpr_load (Register, word_t *)
{
    return 0;
}

word_t vtomal_t::gpr_store (Register, word_t *)
{
    return 0;
}
