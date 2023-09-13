//
// Created by tqcq on 2023/9/7.
//

#include "socket_dispatcher.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/physical_socket_server.h"

namespace tqcq {
SocketDispatcher::SocketDispatcher(PhysicalSocketServer *ss)
    : PhysicalSocket(ss)
#if defined(U_USE_EPOLL)
      ,
      saved_enabled_events_(-1)
#endif
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

        struct linger linger_opt {
                .l_onoff = 1, .l_linger = 30
        };

        socklen_t linger_opt_len = sizeof(linger_opt);

        if (setsockopt(s_, SOL_SOCKET, SO_LINGER, &linger_opt, linger_opt_len)
            == -1) {
                U_ERROR("setsockopt SO_LINGER failed: {}, {}", errno,
                        strerror(errno));
                return false;
        }

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

#if defined(U_USE_EPOLL)
        StartBatchedEventUpdates();
#endif

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
#if defined(U_USE_EPOLL)
        FinishBatchedEventUpdates();
#endif
}

int
SocketDispatcher::Close()
{
        if (s_ == INVALID_SOCKET) {
                return 0;
        }
#if defined(U_USE_EPOLL)
        if (saved_enabled_events_ != -1) {
                saved_enabled_events_ = 0;
        }
#endif
        ss_->Remove(this);
        return PhysicalSocket::Close();
}

#if defined(U_USE_EPOLL)
inline static int
GetEpollEvents(uint32_t ff)
{
        int events = 0;
        if (ff & (DE_READ | DE_ACCEPT)) {
                events |= EPOLLIN;
        }

        if (ff & (DE_WRITE | DE_CONNECT)) {
                events |= EPOLLOUT;
        }

        return events;
}

void
SocketDispatcher::StartBatchedEventUpdates()
{
        U_ASSERT(saved_enabled_events_ == -1,
                 "StartBatchedEventUpdates: saved_enabled_events_ is not -1");
        saved_enabled_events_ = enabled_events();
}

void
SocketDispatcher::FinishBatchedEventUpdates()
{
        uint8_t old_events = static_cast<uint8_t>(saved_enabled_events_);
        saved_enabled_events_ = -1;
        MayBeUpdateDispatcher(old_events);
}

void
SocketDispatcher::SetEnabledEvents(uint8_t events)
{
        uint8_t old_events = enabled_events();
        PhysicalSocket::SetEnabledEvents(events);
        MayBeUpdateDispatcher(old_events);
}

void
SocketDispatcher::EnableEvents(uint8_t events)
{
        uint8_t old_events = enabled_events();
        PhysicalSocket::EnableEvents(events);
        MayBeUpdateDispatcher(old_events);
}

void
SocketDispatcher::DisableEvents(uint8_t events)
{
        uint8_t old_events = enabled_events();
        PhysicalSocket::DisableEvents(events);
        MayBeUpdateDispatcher(old_events);
}

void
SocketDispatcher::MayBeUpdateDispatcher(uint8_t old_events)
{
        bool event_updated =
                GetEpollEvents(enabled_events()) != GetEpollEvents(old_events);
        if (event_updated && saved_enabled_events_ == -1) {
                ss_->Update(this);
        }
}
#endif
}// namespace tqcq
