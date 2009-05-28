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

#include <syscallError.h>
#include <processor/types.h>
#include <processor/Processor.h>
#include <process/Process.h>
#include <utilities/Tree.h>
#include <vfs/File.h>
#include <vfs/VFS.h>
#include <machine/Network.h>
#include <network/NetManager.h>
#include <network/NetworkStack.h>
#include <network/RoutingTable.h>
#include <network/Dns.h>
#include <network/Tcp.h>

#include <Subsystem.h>
#include <PosixSubsystem.h>

#include "file-syscalls.h"
#include "net-syscalls.h"

#define SYS_SOCK_CONSTANTS_ONLY
#include "include/sys/socket.h"
#include "include/netinet/in.h"

typedef size_t size_t;
typedef uint32_t sa_family_t;
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct sockaddr
{
  sa_family_t sa_family;
  char sa_data[14];
};

struct in_addr
{
  in_addr_t s_addr;
};


struct sockaddr_in
{
  sa_family_t sin_family;
  in_port_t sin_port;
  struct in_addr sin_addr;
};

int posix_socket(int domain, int type, int protocol)
{
  NOTICE("socket(" << domain << ", " << type << ", " << protocol << ")");

  // Lookup this process.
  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  size_t fd = pSubsystem->getFd();

  File* file = 0;
  bool valid = true;
  if(domain == AF_INET)
  {
    if((type == SOCK_DGRAM && protocol == 0) || (type == SOCK_DGRAM && protocol == IPPROTO_UDP))
      file = NetManager::instance().newEndpoint(NETMAN_TYPE_UDP, protocol);
    else if((type == SOCK_STREAM && protocol == 0) || (type == SOCK_STREAM && protocol == IPPROTO_TCP))
      file = NetManager::instance().newEndpoint(NETMAN_TYPE_TCP, protocol);
    else if(type == SOCK_RAW)
    {
      file = NetManager::instance().newEndpoint(NETMAN_TYPE_RAW, protocol);
    }
    else
    {
      valid = false;
    }
  }
  else if(domain == PF_SOCKET)
  {
    // raw wire-level sockets
    file = NetManager::instance().newEndpoint(NETMAN_TYPE_RAW, 0xff);
  }
  else
  {
    WARNING("domain = " << domain << " - not known!");
    valid = false;
  }

  if(!valid)
    return -1;

  FileDescriptor *f = new FileDescriptor;
  f->file = file;
  f->offset = 0;
  f->fd = fd;
  pSubsystem->addFileDescriptor(fd, f);

  return static_cast<int> (fd);
}

int posix_connect(int sock, struct sockaddr* address, size_t addrlen)
{
  NOTICE("posix_connect(" << sock << ", " << reinterpret_cast<uintptr_t>(address) << ", " << addrlen << ")");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  int endpointState = p->state();
  if(endpointState != 0xff)
  {
    if(endpointState < Tcp::CLOSED)
    {
      if(endpointState < Tcp::ESTABLISHED)
      {
        // EALREADY - connection attempt in progress
        SYSCALL_ERROR(Already);
        return -1;
      }
      else
      {
        // EISCONN - already connected
        SYSCALL_ERROR(IsConnected);
        return -1;
      }
    }
  }

  bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

  Endpoint::RemoteEndpoint remoteHost;

  /// \todo Other protocols
  bool success = false;
  if(s->getProtocol() == NETMAN_TYPE_TCP)
  {
    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);

    success = p->connect(remoteHost, blocking);

    if(!blocking)
    {
      SYSCALL_ERROR(InProgress);
      return -1;
    }
  }
  else if(s->getProtocol() == NETMAN_TYPE_UDP)
  {
    // connect on a UDP socket sets a remote address and port for send/recv
    // to send to multiple addresses and receive from multiple clients
    // sendto and recvfrom must be used

    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);

    p->setRemotePort(remoteHost.remotePort);
    p->setRemoteIp(remoteHost.ip);
    success = true;
  }
  else if(s->getProtocol() == NETMAN_TYPE_RAW)
    success = true; /// \todo If the interface is down, fail

  NOTICE("posix_connect returns");

  return success ? 0 : -1;
}

ssize_t posix_send(int sock, const void* buff, size_t bufflen, int flags)
{
  NOTICE("posix_send");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  bool success = false;
  if(s->getProtocol() == NETMAN_TYPE_TCP)
  {
    return p->send(bufflen, reinterpret_cast<uintptr_t>(buff));
  }
  else if(s->getProtocol() == NETMAN_TYPE_UDP)
  {
    // special handling - need to check for a remote host
    IpAddress remoteIp = p->getRemoteIp();
    if(remoteIp.getIp() != 0)
    {
      Endpoint::RemoteEndpoint remoteHost;
      remoteHost.remotePort = p->getRemotePort();
      remoteHost.ip = remoteIp;
      p->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false, RoutingTable::instance().DetermineRoute(remoteIp));
    }
  }

  return success ? bufflen : -1;
}

struct special_send_recv_data
{
  int sock;
  void* buff;
  unsigned long bufflen;
  int flags;
  struct sockaddr* remote_addr;
  unsigned long* addrlen;
} __attribute__((packed));

ssize_t posix_sendto(void* callInfo)
{
  NOTICE("posix_sendto");

  /// \todo Ugly - constructor would be nicer.
  special_send_recv_data* tmp = reinterpret_cast<special_send_recv_data*>(callInfo);
  int sock = tmp->sock;
  const void* buff = tmp->buff;
  size_t bufflen = tmp->bufflen;
  //int flags = tmp->flags;
  const sockaddr* address = tmp->remote_addr;
  //size_t* addrlen = tmp->addrlen;

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  if(s->getProtocol() == NETMAN_TYPE_TCP)
  {
    /// \todo I need to write a sendto for TcpManager and TcpEndpoint
    ///       which probably means UDP gets a free sendto as well.
    ///       Until then, this is NOT valid according to the standards.
    ERROR("TCP sendto called, but not implemented properly!");
    return p->send(bufflen, reinterpret_cast<uintptr_t>(buff));
  }
  else if(s->getProtocol() == NETMAN_TYPE_UDP || s->getProtocol() == NETMAN_TYPE_RAW)
  {
    Endpoint::RemoteEndpoint remoteHost;
    const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(address);
    remoteHost.remotePort = BIG_TO_HOST16(sin->sin_port);
    remoteHost.ip.setIp(sin->sin_addr.s_addr);
    return p->send(bufflen, reinterpret_cast<uintptr_t>(buff), remoteHost, false, RoutingTable::instance().DetermineRoute(remoteHost.ip));
  }

  return -1;
}

ssize_t posix_recv(int sock, void* buff, size_t bufflen, int flags)
{
  NOTICE("posix_recv");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

  int ret = -1;
  if(s->getProtocol() == NETMAN_TYPE_TCP)
  {
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, blocking, flags & MSG_PEEK);
  }
  else if(s->getProtocol() == NETMAN_TYPE_UDP)
  {
    /// \todo Actually, we only should read this data if it's from the IP specified
    ///       during connect - otherwise we fail (UDP should use sendto/recvfrom)
    ///       However, to do that we need to tell recv not to remove from the queue
    ///       and instead peek at the message (in other words, we need flags)
    Endpoint::RemoteEndpoint remoteHost;
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, &remoteHost);
  }

  return ret;
}

ssize_t posix_recvfrom(void* callInfo)
{
  NOTICE("posix_recvfrom");

  /// \todo Ugly - constructor would be nicer.
  special_send_recv_data* tmp = reinterpret_cast<special_send_recv_data*>(callInfo);
  int sock = tmp->sock;
  void* buff = tmp->buff;
  size_t bufflen = tmp->bufflen;
  int flags = tmp->flags;
  sockaddr* address = tmp->remote_addr;
  size_t* addrlen = tmp->addrlen;

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  bool blocking = !((f->flflags & O_NONBLOCK) == O_NONBLOCK);

  int ret = -1;
  if(s->getProtocol() == NETMAN_TYPE_TCP)
  {
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, true, flags & MSG_PEEK);

    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    sin->sin_port = HOST_TO_BIG16(p->getRemotePort());
    sin->sin_addr.s_addr = p->getRemoteIp().getIp();
    *addrlen = sizeof(struct sockaddr_in);
  }
  else if(s->getProtocol() == NETMAN_TYPE_UDP || s->getProtocol() == NETMAN_TYPE_RAW)
  {
    if(blocking && (s->getProtocol() != NETMAN_TYPE_RAW))
      p->dataReady(true);

    Endpoint::RemoteEndpoint remoteHost;
    ret = p->recv(reinterpret_cast<uintptr_t>(buff), bufflen, &remoteHost);

    struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
    sin->sin_port = HOST_TO_BIG16(remoteHost.remotePort);
    sin->sin_addr.s_addr = remoteHost.ip.getIp();
    *addrlen = sizeof(struct sockaddr_in);
  }

  return ret;
}

int posix_bind(int sock, const struct sockaddr *address, size_t addrlen)
{
  NOTICE("posix_bind");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();
  if(p)
  {
    int ret = -1;
    if(s->getProtocol() == NETMAN_TYPE_TCP || s->getProtocol() == NETMAN_TYPE_UDP)
    {
      const struct sockaddr_in* sin = reinterpret_cast<const struct sockaddr_in*>(address);

      p->setLocalPort(BIG_TO_HOST16(sin->sin_port));

      ret = 0;
    }

    return ret;
  }
  else
    return -1;
}

int posix_listen(int sock, int backlog)
{
  NOTICE("posix_listen");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s = static_cast<Socket *>(f->file);
  if(!s)
    return -1; /// \todo SYSCALL_ERROR of some sort

  Endpoint* p = s->getEndpoint();

  p->listen();

  return 0;
}

int posix_accept(int sock, struct sockaddr* address, size_t* addrlen)
{
  NOTICE("posix_accept");

  Process *pProcess = Processor::information().getCurrentThread()->getParent();
  PosixSubsystem *pSubsystem = reinterpret_cast<PosixSubsystem*>(pProcess->getSubsystem());
  if(!pSubsystem)
  {
      ERROR("No subsystem for one or both of the processes!");
      return -1;
  }

  FileDescriptor *f = pSubsystem->getFileDescriptor(sock);
  Socket *s1 = static_cast<Socket *>(f->file);
  if(!s1)
    return -1;

  Socket *s = static_cast<Socket *>(NetManager::instance().accept(s1));
  if(!s)
    return -1;

  // add into the descriptor table
  size_t fd = pSubsystem->getFd();

  Endpoint* e = s->getEndpoint(); // NetManager::instance().getEndpoint(f);

  if(address && addrlen)
  {
    if(s->getProtocol() == NETMAN_TYPE_TCP || s->getProtocol() == NETMAN_TYPE_UDP)
    {
      struct sockaddr_in* sin = reinterpret_cast<struct sockaddr_in*>(address);
      sin->sin_port = HOST_TO_BIG16(e->getRemotePort());
      sin->sin_addr.s_addr = e->getRemoteIp().getIp();

      *addrlen = sizeof(struct sockaddr_in);
    }
  }

  FileDescriptor *desc = new FileDescriptor;
  desc->file = s;
  desc->offset = 0;
  pSubsystem->addFileDescriptor(fd, desc);

  return static_cast<int> (fd);
}

int posix_gethostbyaddr(const void* addr, size_t len, int type, void* ent)
{
  NOTICE("gethostbyaddr");
  return -1;
}

int posix_gethostbyname(const char* name, void* hostinfo, int offset)
{
  NOTICE("gethostbyname");

  // sanity checks
  if(!hostinfo || !offset)
    return -1;

  // lookups can be done on the default interface
  Network* pCard = RoutingTable::instance().DefaultRoute();

  // do the lookup
  size_t num = 0;
  IpAddress* ips = Dns::instance().hostToIp(String(name), num, pCard);
  if(!num)
  {
    NOTICE("No IPs found");
    return -1;
  }

  // grab the passed hostent
  struct hostent
  {
    char* h_name;
    char** h_aliases;
    int h_addrtype;
    int h_length;
    char** h_addr_list;
    char* h_addr;
  } *entry = reinterpret_cast<struct hostent*>(hostinfo);
  uintptr_t userBlock = reinterpret_cast<uintptr_t>(hostinfo) + sizeof(struct hostent);
  uintptr_t endBlock = (userBlock + offset) - sizeof(struct hostent);

  memset(hostinfo, 0, offset);

  // copy the hostname
  char* hostName = reinterpret_cast<char*>(userBlock);
  strcpy(hostName, name);
  userBlock += strlen(name) + 1;

  /// \todo Aliases
  char** aliases = reinterpret_cast<char**>(0);

  // address list
  char** addrList = reinterpret_cast<char**>(userBlock);
  userBlock += sizeof(char*) * (num + 1); // null terminated list

  // make sure we don't overflow the buffer
  if(userBlock < endBlock)
  {
    // load each IP into the buffer
    for(size_t i = 0; i < num; i++)
    {
      uint32_t ip = ips[i].getIp();

      char tmp[] = {ip & 0xff, (ip & 0xff00) >> 8, (ip & 0xff0000) >> 16, (ip & 0xff000000) >> 24};
      char* ipBlock = reinterpret_cast<char*>(userBlock);
      memcpy(ipBlock, tmp, 4);

      addrList[i] = ipBlock;

      userBlock += 4;
      if(userBlock >= endBlock)
        break;
    }
  }

  // build the real hostent
  entry->h_name = hostName;
  entry->h_aliases = aliases;
  entry->h_addrtype = AF_INET;
  entry->h_length = 4;
  entry->h_addr_list = addrList;

  return 0;
}

