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

#include "PhysicalMemoryManager.h"

MipsCommonPhysicalMemoryManager MipsCommonPhysicalMemoryManager::m_Instance;

PhysicalMemoryManager &PhysicalMemoryManager::instance()
{
  return MipsCommonPhysicalMemoryManager::instance();
}

physical_uintptr_t MipsCommonPhysicalMemoryManager::allocatePage()
{
  return (m_NextPage += 0x1000);
}
void MipsCommonPhysicalMemoryManager::freePage(physical_uintptr_t page)
{
  // TODO
}
bool MipsCommonPhysicalMemoryManager::allocateRegion(MemoryRegion &Region,
                                                     size_t cPages,
                                                     size_t pageConstraints,
                                                     size_t Flags,
                                                     physical_uintptr_t start)
{
  // TODO
  return false;
}

MipsCommonPhysicalMemoryManager::MipsCommonPhysicalMemoryManager() :
  m_NextPage(0x00400000) // Start 4MB in.
{
}
MipsCommonPhysicalMemoryManager::~MipsCommonPhysicalMemoryManager()
{
}
