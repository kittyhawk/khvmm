/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       elf.h
 * Description:     ELF Loader
 *
 * All rights reserved
 *
 ********************************************************************/
#pragma once

#include "std.h"

class elf_hdr_t
{
    public:
        enum {
            EH_MAGIC        = 0x7f454c46
        };

        u32_t          ident[4];
        u16_t          type;
        u16_t          machine;
        u32_t          version;
        u32_t          entry;
        u32_t          ph_offset;
        u32_t          sh_offset;
        u32_t          flags;
        u16_t          eh_size;
        u16_t          ph_size;
        u16_t          ph_count;
        u16_t          sh_size;
        u16_t          sh_count;
        u16_t          strtab;

        static word_t load_file (word_t, word_t, word_t);
};

class phdr_t
{
    public:
        enum {
            PT_NULL         = 0,
            PT_LOAD         = 1,
            PT_DYNAMIC      = 2,
            PT_INTERP       = 3,
            PT_NOTE         = 4,
            PT_SHLIB        = 5,
            PT_PHDR         = 6
        };

        u32_t          type;
        u32_t          f_offset;
        u32_t          v_addr;
        u32_t          p_addr;
        u32_t          f_size;
        u32_t          m_size;
        u32_t          flags;
        u32_t          align;
};
