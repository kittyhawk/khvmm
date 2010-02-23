/*********************************************************************
 *
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:       elf.cc
 * Description:     ELF Loader
 *
 * All rights reserved
 *
 ********************************************************************/

#include "elf.h"

word_t elf_hdr_t::load_file (word_t addr, word_t ram_offset, word_t sram_offset)
{
    elf_hdr_t *eh = reinterpret_cast<elf_hdr_t *>(addr);

    if (*eh->ident != elf_hdr_t::EH_MAGIC)
        return 0;

    phdr_t *ph = reinterpret_cast<phdr_t *>(addr + eh->ph_offset);

    for (unsigned i = 0; i < eh->ph_count; i++, ph++) {

        if (ph->type != phdr_t::PT_LOAD)
            continue;

        // XXX
        word_t offset = ph->v_addr > 0xc0000000 ? sram_offset : ram_offset;

        printf ("%#010x-%#010x (%#010lx-%#010lx) %11u Bytes\n",
                ph->p_addr, ph->p_addr + ph->m_size,
                offset + ph->v_addr, offset + ph->v_addr + ph->m_size,
                ph->m_size);

        memcpy (reinterpret_cast<void *>(offset + ph->p_addr), reinterpret_cast<void *>(addr + ph->f_offset), ph->f_size);
        memset (reinterpret_cast<void *>(offset + ph->p_addr + ph->f_size), 0, ph->m_size - ph->f_size);
    }

    return eh->entry;
}
