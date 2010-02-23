/*********************************************************************
 *
 * Copyright (C) 2007-2008, Volkmar Uhlig, IBM Corporation
 * Copyright (C) 2008, Udo Steinberg, IBM Corporation
 *
 * File path:     fdt.cc
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

#include "fdt.h"

fdt_t *fdt_t::fdt;

void fdt_t::dump()
{
    if (!is_valid())
        return;

    unsigned level = 0;

    fdt_node_t *node = root_node();

    do switch (node->type()) {

        case fdt_node_t::HEAD: {
            fdt_head_t *head = static_cast<fdt_head_t *>(node);
            printf ("%*s%s {\n", level * 2, " ", level ? head->name : "/");
            level++;
            node = get_next_node (head);
            break;
        }

        case fdt_node_t::PROP: {
            fdt_prop_t *prop = static_cast<fdt_prop_t *>(node);
            printf ("%*s%s\n", level * 2, " ", get_name (prop));
            node = get_next_node (prop);
            break;
        }

        case fdt_node_t::TAIL:
            level--;
            printf ("%*s}\n", level * 2, " ");
            node++;
            break;

        default:
            printf ("unknown node type %u\n", node->type());
            return;

    } while (level);
}

fdt_node_t *fdt_t::find (fdt_node_t *node, fdt_node_t::Type t, char const *name)
{
    unsigned level = 0;

    do switch (node->type()) {

        case fdt_node_t::HEAD:
            if (level == 1 && node->type() == t && !strcmp (name, static_cast<fdt_head_t *>(node)->name))
                return node;
            level++;
            node = get_next_node (static_cast<fdt_head_t *>(node));
            break;

        case fdt_node_t::PROP:
            if (level == 1 && node->type() == t && !strcmp (name, get_name (static_cast<fdt_prop_t *>(node))))
                return node;
            node = get_next_node (static_cast<fdt_prop_t *>(node));
            break;

        case fdt_node_t::TAIL:
            level--;
            node++;
            break;

        default:
            printf ("unknown node type %u\n", node->type());
            return 0;

    } while (level);

    return 0;
}

fdt_head_t *fdt_t::path_lookup (char const *&path)
{
    while (*path == '/')
        path++;

    fdt_head_t *node = root_node();

    for (char *ptr; (ptr = strchr (path, '/')) && node; path = ptr + 1) {
        *ptr = 0;
	    node = static_cast<fdt_head_t *>(find (node, fdt_node_t::HEAD, path));
	    *ptr = '/';
    }

    return node;
}

fdt_head_t *fdt_t::find_head_node (char const *path)
{
    fdt_head_t *node = path_lookup (path);

    return node ? static_cast<fdt_head_t *>(find (node, fdt_node_t::HEAD, path)) : 0;
}

fdt_prop_t *fdt_t::find_prop_node (char const *path)
{
    fdt_head_t *node = path_lookup (path);

    return node ? static_cast<fdt_prop_t *>(find (node, fdt_node_t::PROP, path)) : 0;
}
