/*
    Copyright (C) 2013 by Matthieu Kraus

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __INETWORK_SOCKET_H__
#define __INETWORK_SOCKET_H__

#include <csutil/scf.h>
#include <csutil/scf_implementation.h>
#include <iutil/array.h>

namespace CS
{
namespace Network
{
namespace Socket
{

enum Family
{
  CS_SOCKET_FAMILY_IP4,
  CS_SOCKET_FAMILY_IP6
};

enum Protocol
{
  CS_SOCKET_PROTOCOL_TCP,
  CS_SOCKET_PROTOCOL_UDP
};

struct iAddress : public virtual iBase
{
  SCF_INTERFACE(iAddress, 0, 1, 0);

  virtual char const *GetAddress() const = 0;

  virtual int GetPort() const = 0;

  virtual Family GetFamily() const = 0;
};

struct iSocket : public virtual iBase
{
  SCF_INTERFACE(iSocket, 0, 1, 0);

  virtual bool Bind(iAddress const *address) = 0;

  virtual bool Listen(int queueSize) = 0;

  virtual csPtr<iSocket> Accept(csRef<iAddress> *client = nullptr) = 0;

  virtual bool Connect(iAddress const *client) = 0;

  virtual size_t Receive(char *buffer, size_t size, csRef<iAddress> *client = nullptr) = 0;

  virtual size_t Send(char const *buffer, size_t size, iAddress *client = nullptr) = 0;

  virtual char const *GetLastError() const = 0;

  virtual bool IsReady() const = 0;

  virtual bool IsConnected() const = 0;

  virtual Family GetFamily() const = 0;

  virtual Protocol GetProtocol() const = 0;
};

struct iSocketArray : public iArrayChangeAll<iSocket *>
{
  SCF_IARRAYCHANGEALL_INTERFACE(iSocketArray);
};

struct iSocketManager : public virtual iBase
{
  SCF_INTERFACE(iSocketManager, 0, 1, 0);

  virtual csPtr<iSocket> CreateSocket(Family family, Protocol protocol) const = 0;

  virtual csPtr<iAddress> Resolve(char const *host, char const *service, Family family, Protocol protocol) const = 0;

  virtual void Select(iSocketArray *read, iSocketArray *write) const = 0;

  virtual size_t GetSelectLimit() const = 0;
};

} // Socket
} // Network
} // CS

#endif // __INETWORK_SOCKET_H__
