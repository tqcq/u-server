//
// Created by tqcq on 2023/9/7.
//

#include "physical_socket.h"
#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/dispatcher.h"
#include "u-toolbox/net/physical_socket_server.h"
#include "u-toolbox/net/socket.h"
#include "u-toolbox/net/socket_address.h"
#include <netdb.h>
#include <netinet/tcp.h>
#include <sys/fcntl.h>
#include <sys/socket.h>

#define LAST_SYSTEM_ERROR (errno)

namespace tqcq {

int64_t
GetSocketRecvTimestamp(int socket)
{
#if __linux__
        /*
        struct timeval tv_ioctl;
        int ret = ioctl(socket, SIOCGSTAMP, &tv_ioctl);
        if (ret) {
                return -1;
        }
         */
        return -1;
#else
        return -1;
#endif
}

PhysicalSocket::PhysicalSocket(PhysicalSocketServer *ss, int s)
    : ss_(ss),
      s_(s),
      error_(0),
      state_((s == INVALID_SOCKET) ? CS_CLOSED : CS_CONNECTED)
{
        SetEnabledEvents(DE_READ | DE_WRITE);

        int type = SOCK_STREAM;
        socklen_t len = sizeof(type);
        const int res = getsockopt(s_, SOL_SOCKET, SO_TYPE, &type, &len);
        udp_ = (SOCK_DGRAM == type);
}

PhysicalSocket::~PhysicalSocket() { Close(); }

bool
PhysicalSocket::Create(int family, int type)
{
        Close();
        s_ = ::socket(family, type, 0);
        udp_ = (SOCK_DGRAM == type);
        family_ = family;
        UpdateLastError();
        if (udp_) {
                SetEnabledEvents(DE_READ | DE_WRITE);
        }
        return s_ != INVALID_SOCKET;
}

int
PhysicalSocket::Bind(const SocketAddress &bind_addr)
{
        SocketAddress copied_bind_addr = bind_addr;
        if (!bind_addr.IsAnyIP()) {
                if (bind_addr.IsLoopBackIP()) {
                        U_INFO("Bind soket to loop address");
                } else {
                        return -1;
                }
        }

        sockaddr_storage addr_storage;
        size_t len = copied_bind_addr.ToSockAddrStorage(&addr_storage);
        sockaddr *addr = reinterpret_cast<sockaddr *>(&addr_storage);
        int err = ::bind(s_, addr, static_cast<socklen_t>(len));
        UpdateLastError();
        return err;
}

int
PhysicalSocket::Connect(const SocketAddress &addr)
{
        if (state_ != CS_CLOSED) {
                SetError(EALREADY);
                return SOCKET_ERROR;
        }

        return DoConnect(addr);
}

int
PhysicalSocket::Send(const void *pv, size_t cb)
{
        // 只有Linux 和 Android 需要使用 MSG_NOSIGNAL
        int sent = DoSend(s_, reinterpret_cast<const char *>(pv),
                          static_cast<int>(cb), MSG_NOSIGNAL);
        UpdateLastError();
        MaybeRemapSendError();
        U_ASSERT(sent <= static_cast<int>(cb), "sent > buffer size");

        // buffer 尚有数据，或者收到EAGAIN 等重试错误，继续监听可写事件
        if ((sent > 0 && sent < static_cast<int>(cb))
            && (sent < 0 && IsBlockingError(GetError()))) {
                EnableEvents(DE_WRITE);
        }

        return sent;
}

int
PhysicalSocket::SendTo(const void *pv, size_t cb, const SocketAddress &addr)
{
        sockaddr_storage saddr;
        size_t len = addr.ToSockAddrStorage(&saddr);
        int sent = DoSendTo(s_, reinterpret_cast<const char *>(pv),
                            static_cast<int>(cb), 0,
                            reinterpret_cast<const sockaddr *>(&saddr),
                            static_cast<socklen_t>(len));
        UpdateLastError();
        MaybeRemapSendError();
        U_ASSERT(sent <= static_cast<int>(cb), "sent > buffer size");
        if ((sent > 0 && sent < static_cast<int>(cb))
            && (sent < 0 && IsBlockingError(GetError()))) {
                EnableEvents(DE_WRITE);
        }

        return sent;
}

int
PhysicalSocket::Recv(void *buffer, size_t length, int64_t *timestamp)
{
        int received = DoReadFromSocket(buffer, length, nullptr, timestamp);
        if ((received == 0) && (length != 0)) {
                EnableEvents(DE_READ);
                SetError(EWOULDBLOCK);
                return SOCKET_ERROR;
        }
        UpdateLastError();
        int error = GetError();
        bool success = (received >= 0) || IsBlockingError(error);
        if (udp_ || success) {
                EnableEvents(DE_READ);
        }
        if (!success) {
                U_ERROR("recv error: {}", error);
        }

        return received;
}

int
PhysicalSocket::RecvFrom(void *buffer,
                         size_t length,
                         SocketAddress *out_addr,
                         int64_t *timestamp)
{
        int received = DoReadFromSocket(buffer, length, out_addr, timestamp);
        UpdateLastError();
        int error = GetError();
        bool success = (received >= 0) || IsBlockingError(error);
        if (udp_ || success) {
                EnableEvents(DE_READ);
        }
        if (!success) {
                U_ERROR("recvfrom error: {}", error);
        }

        return received;
}

int
PhysicalSocket::Listen(int backlog)
{
        int err = ::listen(s_, backlog);
        UpdateLastError();
        if (err == 0) {
                state_ = CS_CONNECTING;
                EnableEvents(DE_ACCEPT);
        }
        return err;
}

Socket *
PhysicalSocket::Accept(SocketAddress *out_addr)
{
        EnableEvents(DE_ACCEPT);
        sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);
        sockaddr *addr = reinterpret_cast<sockaddr *>(&addr_storage);
        SOCKET s = DoAccept(s_, addr, &addr_len);
        UpdateLastError();
        if (s == INVALID_SOCKET) {
                return nullptr;
        }
        if (out_addr != nullptr) {
                SocketAddressFromSockAddrStorage(addr_storage, out_addr);
        }

        return ss_->WrapSocket(s);
}

int
PhysicalSocket::Close()
{
        if (s_ == INVALID_SOCKET) {
                return 0;
        }
        int err = ::close(s_);
        UpdateLastError();
        s_ = INVALID_SOCKET;
        state_ = CS_CLOSED;
        SetEnabledEvents(0);
        return err;
}

int
PhysicalSocket::Shutdown(int flags)
{
        return shutdown(s_, flags);
}

int
PhysicalSocket::GetError() const
{
        LockGuard<Mutex> lock(mutex_);
        return error_;
}

void
PhysicalSocket::SetError(int error)
{
        LockGuard<Mutex> lock(mutex_);
        error_ = error;
}

Socket::ConnState
PhysicalSocket::GetState() const
{
        return Socket::CS_CONNECTING;
}

int
PhysicalSocket::GetOption(Socket::Option opt, int *value)
{
        int slevel;
        int sopt;
        if (TranslateOption(opt, &slevel, &sopt) == -1) {
                return -1;
        }

        socklen_t option = sizeof(*value);
        int ret = ::getsockopt(s_, slevel, sopt, value, &option);
        return ret;
}

int
PhysicalSocket::SetOption(Socket::Option opt, int value)
{
        int slevel;
        int sopt;
        if (TranslateOption(opt, &slevel, &sopt) == -1) {
                return -1;
        }

        int ret = ::setsockopt(s_, slevel, sopt, &value, sizeof(value));
        if (ret != 0) {
                UpdateLastError();
        }

        return ret;
}

SocketServer *
PhysicalSocket::socket_server()
{
        return nullptr;
}

int
PhysicalSocket::DoConnect(const SocketAddress &connect_addr)
{
        if ((s_ == INVALID_SOCKET)
            && !Create(connect_addr.family(), SOCK_STREAM)) {
                return SOCKET_ERROR;
        }

        sockaddr_storage addr_storage;
        size_t len = connect_addr.ToSockAddrStorage(&addr_storage);
        sockaddr *addr = reinterpret_cast<sockaddr *>(&addr_storage);
        int err = ::connect(s_, addr, static_cast<socklen_t>(len));
        UpdateLastError();
        uint8_t events = DE_READ | DE_WRITE;
        if (err == 0) {
                state_ = CS_CONNECTED;
        } else if (IsBlockingError((GetError()))) {
                state_ = CS_CONNECTING;
                events |= DE_CONNECT;
        } else {
                return SOCKET_ERROR;
        }

        EnableEvents(events);
        return 0;
}

SOCKET
PhysicalSocket::DoAccept(int socket, sockaddr *addr, socklen_t *addrlen)
{
        return ::accept(socket, addr, addrlen);
}

int
PhysicalSocket::DoSend(int socket, const char *buf, int len, int flags)
{
        return ::send(socket, buf, len, flags);
}

int
PhysicalSocket::DoSendTo(int socket,
                         const char *buf,
                         int len,
                         int flags,
                         const struct sockaddr *dest_addr,
                         socklen_t addrlen)
{
        return ::sendto(socket, buf, len, flags, dest_addr, addrlen);
}

int
PhysicalSocket::DoReadFromSocket(void *buffer,
                                 size_t length,
                                 SocketAddress *out_addr,
                                 int64_t *timestamp)
{
        sockaddr_storage addr_storage;
        socklen_t addr_len = sizeof(addr_storage);
        sockaddr *addr = reinterpret_cast<sockaddr *>(&addr_storage);

        int received = 0;
        if (out_addr) {
                received = ::recvfrom(s_, static_cast<char *>(buffer),
                                      static_cast<int>(length), 0, addr,
                                      &addr_len);
                SocketAddressFromSockAddrStorage(addr_storage, out_addr);
        } else {
                received = ::recv(s_, static_cast<char *>(buffer),
                                  static_cast<int>(length), 0);
        }

        if (timestamp) {
                *timestamp = GetSocketRecvTimestamp(s_);
        }

        return received;
}

void
PhysicalSocket::UpdateLastError()
{
        SetError(LAST_SYSTEM_ERROR);
}

void
PhysicalSocket::MaybeRemapSendError()
{
#if defined(__APPLE__)
        // 网络接口的 output queue 满了,
        // 应该停止发送，稍后重试
        if (GetError() == ENOBUFS) {
                SetError(EWOULDBLOCK);
        }
#endif
}

void
PhysicalSocket::SetEnabledEvents(uint8_t events)
{
        enabled_events_ = events;
}

void
PhysicalSocket::EnableEvents(uint8_t events)
{
        enabled_events_ |= events;
}

void
PhysicalSocket::DisableEvents(uint8_t events)
{
        enabled_events_ &= ~events;
}

int
PhysicalSocket::TranslateOption(Socket::Option opt, int *slevel, int *sopt)
{
        switch (opt) {
        case OPT_DONTFRAGMENT:
#if defined(__linux__)
                *slevel = IPPROTO_IP;
                *sopt = IP_MTU_DISCOVER;
#else
                return -1;
#endif
                break;
        case OPT_RCVBUF:
                *slevel = SOL_SOCKET;
                *sopt = SO_RCVBUF;
                break;
        case OPT_SNDBUF:
                *slevel = SOL_SOCKET;
                *sopt = SO_SNDBUF;
                break;
        case OPT_NODELAY:
                *slevel = SOL_SOCKET;
                *sopt = TCP_NODELAY;
                break;
        case OPT_REUSEADDR:
                *slevel = SOL_SOCKET;
                *sopt = SO_REUSEADDR;
        case OPT_REUSEPORT:
                *slevel = SOL_SOCKET;
                *sopt = SO_REUSEPORT;
        case OPT_DSCP:
                if (family_ == AF_INET6) {
                        *slevel = IPPROTO_IPV6;
                        *sopt = IPV6_TCLASS;
                } else {
                        *slevel = IPPROTO_IP;
                        *sopt = IP_TOS;
                }
                break;
        case OPT_RTP_SENDTIME_EXTN_ID:
                return -1;
        default:
                return -1;
        }
        return 0;
}
}// namespace tqcq
