/*
* Copyright (c) 2009 Kevin Wolf <kevin@tyndur.org>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITRTLSS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, RTLGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONRTLCTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include "CdiDisk.h"

#include <Log.h>
#include <ServiceManager.h>

#include <vfs/VFS.h>

// Prototypes in the extern "C" block to ensure that they are not mangled
extern "C" {
    void cdi_cpp_disk_register(void* void_pdev, struct cdi_storage_device* device);

    int cdi_storage_read(struct cdi_storage_device* device, uint64_t pos, size_t size, void* dest);
    int cdi_storage_write(struct cdi_storage_device* device, uint64_t pos, size_t size, void* src);
};

CdiDisk::CdiDisk(Disk* pDev, struct cdi_storage_device* device) :
    Disk(), m_Device(device)
{
    setSpecificType(String("CDI Disk"));

    // Chat to the partition service and let it pick up that we're around now
    ServiceFeatures *pFeatures = ServiceManager::instance().enumerateOperations(String("partition"));
    Service         *pService  = ServiceManager::instance().getService(String("partition"));
    NOTICE("Asking if the partition provider supports touch");
    if(pFeatures->provides(ServiceFeatures::touch))
    {
        NOTICE("It does, attempting to inform the partitioner of our presence...");
        if(pService)
        {
            if(pService->serve(ServiceFeatures::touch, reinterpret_cast<void*>(this), sizeof(CdiDisk)))
                NOTICE("Successful.");
            else
                ERROR("Failed.");
        }
        else
            ERROR("FileDisk: Couldn't tell the partition service about the new disk presence");
    }
    else
        ERROR("FileDisk: Partition service doesn't appear to support touch");
}

CdiDisk::~CdiDisk()
{
}

bool CdiDisk::initialise()
{
    return true;
}

// These are the functions that others call - they add a request to the parent controller's queue.
uint64_t CdiDisk::read(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
    /// \todo Sector cache...
    if(cdi_storage_read(m_Device, location, nBytes, reinterpret_cast<void*>(buffer)) != 0)
        return 0;
    else
        return nBytes;
}

uint64_t CdiDisk::write(uint64_t location, uint64_t nBytes, uintptr_t buffer)
{
    /// \todo Sector cache...
    if(cdi_storage_write(m_Device, location, nBytes, reinterpret_cast<void*>(buffer)) != 0)
        return 0;
    else
        return nBytes;
}

void cdi_cpp_disk_register(void* void_pdev, struct cdi_storage_device* device)
{
    Disk* pDev = reinterpret_cast<Disk*>(void_pdev);

    // Create a new CdiDisk node.
    /// \todo I'm assuming pDev is invalid at this stage. This isn't necessarily
    ///       correct, but it's a quick fix that should work for now. It will need
    ///       to be fixed later!
    CdiDisk *pCdiDisk = new CdiDisk(pDev, device);
    device->dev.pDev = reinterpret_cast<void*>(pCdiDisk);

    // Insert into the tree, properly
    pCdiDisk->setParent(&Device::root());
    Device::root().addChild(pCdiDisk);
}
