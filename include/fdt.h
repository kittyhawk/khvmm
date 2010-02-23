/*********************************************************************
 *                
 * Copyright (C) 2007-2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *                
 * File path:     fdt.h
 * Description:   FDT support
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 ********************************************************************/
#pragma once

#include "std.h"

class fdt_node_t
{
    private:
        u32_t tag;

    public:
        enum Type
        {
            HEAD    = 1,
            TAIL    = 2,
            PROP    = 3,
        };

        Type type() const { return Type (tag); }

};

class fdt_head_t : public fdt_node_t
{
    public:
        char name[];

        size_t size() const { return sizeof (*this) + (strlen (name) + 4) & ~3; }
};

class fdt_prop_t : public fdt_node_t
{
    private:
        u32_t len;
        u32_t offset;
        union {
            u8_t data8[];
            u32_t data[];
            char  str[];
        };

    public:
        size_t size() const { return sizeof (*this) + (len - 1 + 4) & ~3; }

        u32_t name() const              { return offset; }
        u32_t length() const            { return len; }
        u8_t  u8 (unsigned idx) const   { return data8[idx];}
        u32_t u32 (unsigned idx) const  { return data[idx]; }
        u64_t u64 (unsigned idx) const  { return static_cast<u64_t>(data[idx]) << 32 | data[idx + 1]; }
        char const *string() const      { return str; }
};

class fdt_t
{
    private:
        u32_t   magic;
        u32_t   size;
        u32_t   offset_dt_struct;           // offset to structure
        u32_t   offset_dt_strings;          // offset to strings
        u32_t   offset_mem_reserve_map;     // offset to memory map
        u32_t   version;
        u32_t   last_compatible_version;
        u32_t   boot_cpuid_phys;
        u32_t   dt_string_size;
        u32_t   dt_struct_size;

        fdt_head_t *path_lookup (char const *&);

    public:
        static fdt_t *fdt;

        static void *operator new (size_t, word_t addr)
        {
            return reinterpret_cast<void *>(addr);
        }

        bool is_valid() const   { return magic == 0xd00dfeed; }

        fdt_head_t *root_node()
        {
            return reinterpret_cast<fdt_head_t *>
                  (reinterpret_cast<word_t>(this) + offset_dt_struct);
        }

        template <typename T>
        fdt_node_t *get_next_node (T *ptr)
        {
            return reinterpret_cast<fdt_node_t *>
                  (reinterpret_cast<word_t>(ptr) + ptr->size());
        }

        char *get_name (fdt_prop_t *prop)
	    {
            return ((char*) this) + offset_dt_strings + prop->name();
        }

        fdt_node_t *find (fdt_node_t *, fdt_node_t::Type, char const *);
        fdt_head_t *find_head_node (char const *);
        fdt_prop_t *find_prop_node (char const *);

        void dump();
};
