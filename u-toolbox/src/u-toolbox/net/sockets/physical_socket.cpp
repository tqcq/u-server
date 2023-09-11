//
// Created by tqcq on 2023/9/7.
//

#include "physical_socket.h"
#include "u-toolbox/net/physical_socket_server.h"

namespace tqcq {
int
PhysicalSocket::Bind(const SocketAddress &addr)
{
        return 0;
}

int
PhysicalSocket::Connect(const SocketAddress &addr)
{
        return 0;
}

int
PhysicalSocket::Send(const void *pv, size_t cb)
{
        return 0;
}

int
PhysicalSocket::SendTo(const void *pv, size_t cb, const SocketAddress &addr)
{
        return 0;
}

int
PhysicalSocket::Recv(void *pv, size_t cb, int64_t *timestamp)
{
        return 0;
}

int
PhysicalSocket::RecvFrom(void *pv, size_t cb, SocketAddress *paddr,
                         int64_t *timestamp)
{
        return 0;
}

int
PhysicalSocket::Listen(int backlog)
{
        return 0;
}

Socket *
PhysicalSocket::Accept(SocketAddress *paddr)
{
        return nullptr;
}

int
PhysicalSocket::Close()
{
        return 0;
}

SocketServer *
PhysicalSocket::socket_server()
{
        return ss_;
}

int
PhysicalSocket::GetErrror() const
{
        return 0;
}

void
PhysicalSocket::SetError(int error)
{}

Socket::ConnState
PhysicalSocket::GetState() const
{
        return CS_CLOSED;
}

int
PhysicalSocket::GetOption(Socket::Option opt, int *value)
{
        return 0;
}

int
PhysicalSocket::SetOption(Socket::Option opt, int value)
{
        return 0;
}
}// namespace tqcq
