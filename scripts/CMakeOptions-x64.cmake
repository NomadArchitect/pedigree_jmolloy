#
# Copyright (c) 2008 James Molloy, James Pritchett, Jörg Pfähler, Matthew Iselin
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

add_option(APIC      "Enable use of the advanced PIC?" OFF)
add_option(MULTIPROCESSOR "Enable use of multiple processors?" OFF)
add_option(ACPI      "Enable use of ACPI?" OFF)
add_option(SMBIOS    "Enable use of SMBIOS?" OFF)
add_option(OMIT_FRAME_POINTER "Compile without using RBP." OFF)

add_non_user_options(ADDITIONAL_CHECKS BITS_64 LITTLE_ENDIAN KERNEL_STANDALONE X86_COMMON X64)

set(PEDIGREE_CFLAGS "-fno-builtin -fno-stack-protector -nostdlib -m64 -mno-red-zone -mno-sse -mcmodel=kernel")
set(PEDIGREE_CXXFLAGS "-Weffc++ -Wall -Wold-style-cast -Wno-long-long -fno-builtin -fno-exceptions -fno-rtti -fno-stack-protector -nostdlib -m64 -mno-red-zone -mno-sse -mcmodel=kernel")
if(OMIT_FRAME_POINTER)
  set(PEDIGREE_CFLAGS "${PEDIGREE_CFLAGS} -fomit-frame-pointer")
  set(PEDIGREE_CXXFLAGS "${PEDIGREE_CXXFLAGS} -fomit-frame-pointer")
endif(OMIT_FRAME_POINTER)
set(PEDIGREE_ASFLAGS "-felf64")
set(LINKER_SCRIPT "${PEDIGREE_SOURCE_DIR}/src/system/kernel/core/processor/x64/kernel.ld -nostdlib -nostdinc")
