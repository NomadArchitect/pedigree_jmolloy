/*
 * Copyright (c) 2008 James Molloy, J�rg Pf�hler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Module.h>
#include <Log.h>

extern "C" {
    void init_floppy(int argc, char *argv[]);
};

/// Wrapper for the actual driver's code
void pedigree_init()
{
    init_floppy(0, 0);
}

void pedigree_destroy()
{
    ERROR("No MODULE_EXIT function call that we can use");
}

MODULE_NAME("floppy");
MODULE_ENTRY(&pedigree_init);
MODULE_EXIT(&pedigree_destroy);
MODULE_DEPENDS("cdi");
