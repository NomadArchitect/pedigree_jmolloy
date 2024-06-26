/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
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

#ifndef KERNEL_PROCESSOR_X64_GDT_H
#define KERNEL_PROCESSOR_X64_GDT_H

#include <compiler.h>
#include <processor/types.h>

/** @addtogroup kernelprocessorx64
 * @{ */

/** The GDT manager on x64 processors */
class X64GdtManager
{
  public:
    /** Get the gdt manager instance
     *\return instance of the gdt manager */
    inline static X64GdtManager &instance()
      {return m_Instance;}

    /** Initialise the GDT
     *\param[in] processorCount the number of processors
     *\note This should only be called from initialiseProcessor2() */
    void initialise(size_t processorCount) INITIALISATION_ONLY;
    /** Initialises this processors GDTR
     *\note This should only be called from initialiseProcessor2()
     *\todo and some smp/acpi function */
    static void initialiseProcessor() INITIALISATION_ONLY;
    
    /** Sets up the TLS base for the current CPU to point to the given base. */
    void setTlsBase(uintptr_t base);

  private:
    /** The constructor */
    X64GdtManager() INITIALISATION_ONLY;
    /** Copy constructor
     *\note NOT implemented */
    X64GdtManager(const X64GdtManager &);
    /** Assignment operator
     *\note NOT implemented */
    X64GdtManager &operator = (const X64GdtManager &);
    /** The destructor */
    ~X64GdtManager();

    /** Set up a segment descriptor
     *\param[in] index the descriptor index
     *\param[in] base the base address
     *\param[in] limit the size of the segment (either in bytes or in 4kb units)
     *\param[in] flags the flags
     *\param[in] flags2 additional flags */
    void setSegmentDescriptor(size_t index,
                              uint64_t base,
                              uint32_t limit,
                              uint8_t flags,
                              uint8_t flags2);
    /** Set up a task-state-segment descriptor
     *\param[in] index the descriptor index
     *\param[in] base the base address */
    void setTssDescriptor(size_t index, uint64_t base) INITIALISATION_ONLY;

    /** Readies a TSS for use.
     *\param[in] pTss the TSS to ready. */
    void initialiseTss(struct X64TaskStateSegment *pTss) INITIALISATION_ONLY;
    void initialiseDoubleFaultTss(X64TaskStateSegment *pTss) INITIALISATION_ONLY;
    
    /** Load the new segment registers
     *\note This function is implemented in asm/gdt.s */
    static void loadSegmentRegisters();

    /** Long-mode segment descriptor structure */
    struct segment_descriptor
    {
      /** Bits 0-15 from the limit */
      uint16_t limit0;
      /** Bits 0-15 from the base address */
      uint16_t base0;
      /** Bits 16-23 from the base address */
      uint8_t base1;
      /** The flags */
      uint8_t flags;
      /** Additional flags and bits 16-19 from the limit */
      uint8_t flags_limit1;
      /** Bits 24-32 from the base address */
      uint8_t base2;
    }PACKED;

    /** Upper 8byte of the long-mode task-state-segment descriptor */
    struct tss_descriptor
    {
      /** Bits 32-63 of the base address */
      uint32_t base3;
      /** reserved */
      uint32_t res;
    };

    /** The Gdt */
    segment_descriptor *m_Gdt;
    /** The number of descriptors in the Gdt */
    size_t m_DescriptorCount;

    /** The instance of the gdt manager  */
    static X64GdtManager m_Instance;
};

/** @} */

#endif
