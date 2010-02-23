/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       personality.cc
 * Description:     Personality Page
 *
 * All rights reserved
 *
 ********************************************************************/

#include "fdt.h"
#include "personality.h"

bgp_personality_kernel_t::bgp_personality_kernel_t (unsigned freq)
                        : mhz (freq / 1000000),
                          node (EN_COLLECTIVE | EN_TORUS)
{
    fdt_prop_t *prop = fdt_t::fdt->find_prop_node ("/u-boot-env/bgp_uci");
    const char *bgp_uci_str = prop->string();
    uci = strtoul(bgp_uci_str, NULL, 16);
}

bgp_personality_ddr_t::bgp_personality_ddr_t (size_t ram)
                     : sizemb (ram >> 20)
{}

bgp_personality_network_t::bgp_personality_network_t()
                         : psetsize (32),
                           ionoderank (32)
{
    fdt_prop_t *prop;
    
    prop = fdt_t::fdt->find_prop_node ("/plb/tree/nodeid");
    if (prop && prop->length() == sizeof (u32_t))
        rank = prop->u32 (0);
    
    prop = fdt_t::fdt->find_prop_node("/plb/torus/coordinates");
    if (prop && prop->length() == 3 * sizeof (u32_t))
    {
        xcoord = prop->u32 (0);
        ycoord = prop->u32 (1);
        zcoord = prop->u32 (2);
    }
    
    prop = fdt_t::fdt->find_prop_node("/plb/torus/dimension");
    if (prop && prop->length() == 3 * sizeof (u32_t))
    {
        xnodes = prop->u32 (0);
        ynodes = prop->u32 (1);
        znodes = prop->u32 (2);
    }
}

bgp_personality_t::bgp_personality_t (unsigned freq, size_t ram)
                 : kernel (freq),
                   ddr (ram)
{}
