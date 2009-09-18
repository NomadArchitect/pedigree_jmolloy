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
#include <machine/DeviceHashTree.h>
#include <machine/Device.h>
#include <Log.h>

#include <utilities/sha1/sha1.h>

DeviceHashTree DeviceHashTree::m_Instance;

DeviceHashTree::DeviceHashTree() : m_bInitialised(false), m_DeviceTree()
{
}

DeviceHashTree::~DeviceHashTree()
{
}

void DeviceHashTree::fill(Device *root)
{
    static SHA1 mySha1;
    for(unsigned int i = 0; i < root->getNumChildren(); i++)
    {
        Device *pChild = root->getChild(i);
        if(pChild->getType() != Device::Root)
        {
            // Grab the device information
            String name, dump;
            pChild->getName(name);
            pChild->dump(dump);
            uint32_t a, b, c;
            uint32_t bus = pChild->getPciBusPosition();
            uint32_t dev = pChild->getPciDevicePosition();
            uint32_t func = pChild->getPciFunctionNumber();
            uint16_t devId = pChild->getPciDeviceId();

            // Build the string to be hashed
            NormalStaticString theString;
            theString.clear();
            theString += name;
            theString += "-";
            theString += dump;
            theString += "-";
            theString.append(bus);
            theString += ".";
            theString.append(dev);
            theString += ".";
            theString.append(func);

            // Hash the string
            mySha1.Reset();
            mySha1.Input(static_cast<const char*>(theString), theString.length());
            unsigned int digest[5];
            mySha1.Result(digest);

            // Only use 4 bytes of the hash
            NOTICE("Device hash for `" << dump << "' is: " << digest[0] << ".");
            m_DeviceTree.insert(digest[0], pChild);
        }

        if(pChild->getNumChildren())
            fill(pChild);
    }

    m_bInitialised = true;
}

Device *DeviceHashTree::getDevice(uint32_t hash)
{
    if(!m_bInitialised)
        return 0;
    else
        return m_DeviceTree.lookup(hash);
}

Device *DeviceHashTree::getDevice(String hash)
{
    if(!m_bInitialised)
        return 0;
    else
    {
        /// \todo String -> int
        return m_DeviceTree.lookup(0);
    }
}
