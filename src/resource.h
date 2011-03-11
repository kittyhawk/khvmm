/*********************************************************************
 *
 * Copyright (C) 2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:     resource.h
 * Description:
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class resource_t
{
public:
    paddr_t hpa_base;   // real physical address
    word_t  gpa_base;   // guest physical address
    word_t  map_base;   // local mapped address
    size_t  size;       // region size

    resource_t() {};
    resource_t (paddr_t hpa, word_t gpa, size_t s, word_t map = 0) : hpa_base (hpa), gpa_base (gpa), map_base (map), size (s) {}

    bool gpa_to_map (paddr_t gpa, word_t &map)
        {
            if (gpa >= gpa_base && gpa < gpa_base + size) {
                map = gpa - gpa_base + map_base;
                return true;
            }

            return false;
        }

    bool map_to_gpa (word_t map, paddr_t &gpa)
        {
            if (map >= map_base && map < map_base + size)
            {
                gpa = map - map_base + gpa_base;
                return true;
            }
            return false;
        }

    bool gpa_to_hpa (paddr_t gpa, paddr_t &hpa)
        {
            if (gpa >= gpa_base && gpa < gpa_base + size) {
                hpa = gpa - gpa_base + hpa_base;
                return true;
            }

            return false;
        }

    bool hpa_to_gpa (paddr_t hpa, paddr_t &gpa)
        {
            if (hpa >= hpa_base && hpa < hpa_base + size)
            {
                gpa = hpa - hpa_base + gpa_base;
                return true;
            }
            return false;
        }

    bool relocatable (paddr_t hpa, paddr_t &gpa)
        {
            // Resource must be relocated
            assert(hpa_base != gpa_base);

            // Check resource bounds
            if (hpa >= hpa_base && hpa < hpa_base + size) {
                gpa = hpa - hpa_base + gpa_base;
                return true;
            }

            return false;
        }
   
   
};
