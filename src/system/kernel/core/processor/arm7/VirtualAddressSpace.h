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

#ifndef KERNEL_PROCESSOR_ARM7_VIRTUALADDRESSSPACE_H
#define KERNEL_PROCESSOR_ARM7_VIRTUALADDRESSSPACE_H

#include <processor/types.h>
#include <processor/VirtualAddressSpace.h>

/** @addtogroup kernelprocessorArm7
 * @{ */

/** The Arm7VirtualAddressSpace implements the VirtualAddressSpace class for the mip32
    processor, which means it encompasses paging (KUSEG) and KSEG0, KSEG1, KSEG2.*/
class Arm7VirtualAddressSpace : public VirtualAddressSpace
{
  /** Processor::switchAddressSpace() needs access to m_PhysicalPageDirectory */
  friend class Processor;
  friend VirtualAddressSpace &VirtualAddressSpace::getKernelAddressSpace();
  public:
    //
    // VirtualAddressSpace Interface
    //
    virtual bool isAddressValid(void *virtualAddress);
    virtual bool isMapped(void *virtualAddress);

    virtual bool map(physical_uintptr_t physicalAddress,
                     void *virtualAddress,
                     size_t flags);
    virtual void getMapping(void *virtualAddress,
                            physical_uintptr_t &physicalAddress,
                            size_t &flags);
    virtual void setFlags(void *virtualAddress, size_t newFlags);
    virtual void unmap(void *virtualAddress);
    virtual void *allocateStack();
    virtual void freeStack(void *pStack);

    //
    // Needed for the PhysicalMemoryManager
    //
    /** Map the page table or the page frame if none is currently present
     *\note This should only be used from the PhysicalMemoryManager
     *\param[in] physicalAddress the physical page that should be used as page table or
     *                           page frame
     *\param[in] virtualAddress the virtual address that should be checked for the existance
     *                          of a page table and page frame
     *\param[in] flags the flags used for the mapping
     *\return true, if a page table/frame is already mapped for that address, false if the
     *        physicalAddress has been used as a page table or as a page frame. */
    bool mapPageStructures(physical_uintptr_t physicalAddress,
                           void *virtualAddress,
                           size_t flags);
    

    /** Clone this VirtualAddressSpace. That means that we copy-on-write-map the application
     *  image. */
    virtual VirtualAddressSpace *clone() {return 0;};

    /** Undo a clone() - this happens when an application is Exec()'d - we destroy all mappings
        not in the kernel address space so the space is 'clean'.*/
    virtual void revertToKernelAddressSpace() {};

  protected:
    /** The destructor does nothing */
    virtual ~Arm7VirtualAddressSpace();

  private:
    
    /** Section B3.3 in the ARM Architecture Reference Manual (ARM7) */
    
    /// \todo Document descriptor bits
    /// First level descriptor - roughly equivalent to a page directory entry
    /// on x86
    struct FirstLevelDescriptor
    {
        /// Type of this descriptor:
        /// 0 = fault
        /// 1 = page table
        /// 2 = section or supersection
        /// 3 = reserved
        uint32_t type : 2;
        
        union {
            struct {
                uint32_t ignore : 30;
            } PACKED fault;
            struct {
                uint32_t sbz1 : 1;
                uint32_t ns : 1;
                uint32_t sbz2 : 1;
                uint32_t domain : 4;
                uint32_t imp : 1;
                uint32_t baseaddr : 22;
            } PACKED pageTable;
            struct {
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t xn : 1;
                uint32_t domain : 4; /// extended base address for supersection
                uint32_t imp : 1;
                uint32_t ap : 1;
                uint32_t tex : 3;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t type : 1; /// = 0 for section, 1 for supersection
                uint32_t ns : 1;
                uint32_t base : 12;
            } PACKED section;
            
            uint32_t entry;
        } descriptor;
    } PACKED;
    
    /// Second level descriptor - roughly equivalent to a page table entry
    /// on x86
    struct SecondLevelDescriptor
    {
        /// Type of this descriptor:
        /// 0 = fault
        /// 1 = large page
        /// >2 = small page (NX at bit 0)
        uint32_t type : 2;
        
        union
        {
            struct {
                uint32_t ignore : 30;
            } PACKED fault;
            struct {
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t ap1 : 2;
                uint32_t sbz : 3;
                uint32_t ap2 : 1;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t tex : 3;
                uint32_t xn : 1;
                uint32_t base : 16;
            } PACKED largepage;
            struct {
                uint32_t b : 1;
                uint32_t c : 1;
                uint32_t ap1 : 2;
                uint32_t sbz : 3;
                uint32_t ap2 : 1;
                uint32_t s : 1;
                uint32_t nG : 1;
                uint32_t base : 20;
            } PACKED smallpage;
            
            uint32_t data;
        } descriptor;
    } PACKED;

    /** The constructor for already present paging structures
     *\param[in] Heap virtual address of the beginning of the heap
     *\param[in] PhysicalPageDirectory physical address of the page directory
     *\param[in] VirtualPageDirectory virtual address of the page directory
     *\param[in] VirtualPageTables virtual address of the page tables */
    Arm7VirtualAddressSpace(void *Heap,
                           physical_uintptr_t PhysicalPageDirectory,
                           void *VirtualPageDirectory,
                           void *VirtualPageTables);

    /** The default constructor
     *\note NOT implemented */
    Arm7VirtualAddressSpace();
    /** The copy-constructor
     *\note NOT implemented */
    Arm7VirtualAddressSpace(const Arm7VirtualAddressSpace &);
    /** The copy-constructor
     *\note Not implemented */
    Arm7VirtualAddressSpace &operator = (const Arm7VirtualAddressSpace &);

    /** Initialises the kernel address space, called by Processor. */
    bool initialise();

    /** Get the page table entry, if it exists and check whether a page is mapped or marked as
     *  swapped out.
     *\param[in] virtualAddress the virtual address
     *\param[out] pageTableEntry pointer to the page table entry
     *\return true, if the page table is present and the page mapped or marked swapped out, false
     *        otherwise */
    bool getPageTableEntry(void *virtualAddress,
                           uint32_t *&pageTableEntry);
    /** Convert the processor independant flags to the processor's representation of the flags
     *\param[in] flags the processor independant flag representation
     *\return the proessor specific flag representation */
    uint32_t toFlags(size_t flags);
    /** Convert processor's representation of the flags to the processor independant representation
     *\param[in] Flags the processor specific flag representation
     *\return the proessor independant flag representation */
    size_t fromFlags(uint32_t Flags);

    /** Physical address of the page directory */
    physical_uintptr_t m_PhysicalPageDirectory;
    /** Virtual address of the page directory */
    void *m_VirtualPageDirectory;
    /** Virtual address of the page tables */
    void *m_VirtualPageTables;

    /** The kernel virtual address space */
    static Arm7VirtualAddressSpace m_KernelSpace;
};

/** @} */

#endif