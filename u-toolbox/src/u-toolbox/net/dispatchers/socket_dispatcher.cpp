//
// Created by tqcq on 2023/9/7.
//

#include "socket_dispatcher.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/physical_socket_server.h"

namespace tqcq {
SocketDispatcher::SocketDispatcher(PhysicalSocketServer *ss)
    : PhysicalSocket(ss)
{}

SocketDispatcher::SocketDispatcher(SOCKET s, PhysicalSocketServer *ss)
    : PhysicalSocket(ss, s)
{}

SocketDispatcher::~SocketDispatcher() { Close(); }

bool
SocketDispatcher::Initialize()
{
        U_ASSERT(s_ != INVALID_SOCKET, "Initialize: s_ is invalid");
        if (s_ == INVALID_SOCKET) {
                return false;
        }

        // 服务端 socket 设置为 非阻塞模式
        fcntl(s_, F_SETFL, fcntl(s_, F_GETFL, 0) | O_NONBLOCK);

        ss_->Add(this);
        return true;
}

bool
SocketDispatcher::Create(int type)
{
        return Create(AF_INET, type);
}

bool
SocketDispatcher::Create(int family, int type)
{
        if (!PhysicalSocket::Create(family, type)) {
                return false;
        }

        if (!Initialize()) {
                return false;
        }

        return true;
}

int
SocketDispatcher::GetDescriptor() const
{
        return s_;
}

bool
SocketDispatcher::IsDescriptorClosed() const
{
        if (udp_) {
                return s_ == INVALID_SOCKET;
        }

        char ch;
        ssize_t res;
        do {
                res = ::recv(s_, &ch, 1, MSG_PEEK);
        } while (res < 0 && errno == EINTR);

        if (res > 0) {
                // 数据可用
                return false;
        } else if (res == 0) {
                // 连接关闭，EOF
                return true;
        } else {
                switch (errno) {
                case EBADF:
                        return true;
                case ECONNRESET:
                        return true;
                case EPIPE:
                        return true;
                case EWOULDBLOCK:
                        return false;
                default:
                        // EEK error: {}, {}", errno,
                        //       strerror(errno));
                        return false;
                }
        }
}

uint32_t
SocketDispatcher::GetRequestedEvents()
{
        return enabled_events();
}

void
SocketDispatcher::OnEvent(uint32_t ff, int err)
{
        if ((ff & DE_CONNECT) != 0) {
                state_ = CS_CONNECTED;
        }

        if ((ff & DE_CLOSE) != 0) {
                state_ = CS_CLOSED;
        }

        if ((ff & DE_CONNECT) != 0) {
                DisableEvents(DE_CONNECT);
                SignalConnectEvent(this);
        }

        if ((ff & DE_ACCEPT) != 0) {
                DisableEvents(DE_ACCEPT);
                SignalReadEvent(this);
        }

        if ((ff & DE_READ) != 0) {
                DisableEvents(DE_READ);
                SignalReadEvent(this);
        }

        if ((ff & DE_WRITE) != 0) {
                DisableEvents(DE_WRITE);
                SignalWriteEvent(this);
        }
        if ((ff & DE_CLOSE) != 0) {
                DisableEvents(DE_CLOSE);
                SignalCloseEvent(this, err);
        }
}

int
SocketDispatcher::Close()
{
        if (s_ == INVALID_SOCKET) {
                return 0;
        }
        ss_->Remove(this);
        return PhysicalSocket::Close();
}
}// namespace tqcq
