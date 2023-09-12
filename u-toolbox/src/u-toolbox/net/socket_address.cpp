//
// Created by tqcq on 2023/9/11.
//

#include "socket_address.h"
#include "ip_address.h"
#include "sockets/physical_socket.h"
#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/log/u_log.h"
#include <sstream>

namespace tqcq {
SocketAddress::SocketAddress() { Clear(); }

SocketAddress::SocketAddress(uint32_t ip_as_host_order_integer, int port)
{
        SetIP(IPAddress(ip_as_host_order_integer));
        SetPort(port);
}

SocketAddress::SocketAddress(const IPAddress &ip, int port)
{
        SetIP(ip);
        SetPort(port);
}

SocketAddress::SocketAddress(const SocketAddress &addr)
{
        this->operator=(addr);
}

void
SocketAddress::Clear()
{
        hostname_.clear();
        ip_ = IPAddress();
        port_ = 0;
        scoped_id_ = 0;
        literal_ = false;
}

bool
SocketAddress::IsNil() const
{
        return hostname_.empty() && IPIsUnspec(ip_) && 0 == port_;
}

bool
SocketAddress::IsComplete() const
{
        return (!IPIsAny(ip_)) && (0 != port_);
}

SocketAddress &
SocketAddress::operator=(const SocketAddress &addr)
{
        hostname_ = addr.hostname_;
        ip_ = addr.ip_;
        port_ = addr.port_;
        scoped_id_ = addr.scoped_id_;
        return *this;
}

void
SocketAddress::SetIP(const std::string &ip)
{
        hostname_ = ip;
        literal_ = IPFromString(ip, &ip_);
        if (!literal_) {
                ip_ = IPAddress();
        }
}

void
SocketAddress::SetIP(uint32_t ip_as_host_order_integer)
{
        ip_ = IPAddress(ip_as_host_order_integer);
}

void
SocketAddress::SetIP(const IPAddress &ip)
{
        ip_ = ip;
}

void
SocketAddress::SetPort(int port)
{
        port_ = static_cast<uint16_t>(port);
}

void
SocketAddress::SetScopeID(int id)
{
        scoped_id_ = id;
}

uint32_t
SocketAddress::ip() const
{
        return ip_.v4AddressAsHostOrderInteger();
}

int
SocketAddress::family() const
{
        return ip_.family();
}

const IPAddress &
SocketAddress::ipaddr() const
{
        return ip_;
}

uint16_t
SocketAddress::port() const
{
        return port_;
}

std::string
SocketAddress::HostAsURIString() const
{
        if (!hostname_.empty()) {
                return hostname_;
        }

        if (ip_.family() == AF_INET6) {
                return "[" + ip_.ToString() + "]";
        } else {
                return ip_.ToString();
        }
}

std::string
SocketAddress::ToString() const
{
        std::stringstream ss;
        ss << HostAsURIString() << ":" << port();
        return ss.str();
}

bool
SocketAddress::FromString(const std::string &str)
{
        if (str.at(0) == '[') {
                // ipv6
                size_t close_bracket_pos = str.find(']');
                if (close_bracket_pos != std::string::npos) {
                        size_t pos = str.find(':', close_bracket_pos);
                        if (pos != std::string::npos) {
                                SetPort(strtoul(std::string(str.substr(pos + 1))
                                                        .c_str(),
                                                nullptr, 10));
                                SetIP(str.substr(1, close_bracket_pos - 1));
                        } else {
                                return false;
                        }
                }
        } else {
                size_t pos = str.find(':');
                if (pos == std::string::npos) {
                        return false;
                }
                SetPort(strtoul(std::string(str.substr(pos + 1)).c_str(),
                                nullptr, 10));
                SetIP(str.substr(0, pos));
        }

        return true;
}

bool
SocketAddress::IsAnyIP() const
{
        return IPIsAny(ip_);
}

bool
SocketAddress::IsLoopBackIP() const
{
        return IPIsLoopback(ip_)
                || (IPIsAny(ip_)
                    && 0 == strcmp(hostname_.c_str(), "localhost"));
}

bool
SocketAddress::IsPrivateIP() const
{
        return IPIsPrivate(ip_);
}

bool
SocketAddress::operator==(const SocketAddress &addr) const
{
        return EqualIPs(addr) && EqualPorts(addr);
}

bool
SocketAddress::operator<(const SocketAddress &addr) const
{
        if (ip_ != addr.ip_) {
                return ip_ < addr.ip_;
        }

        if ((IPIsAny(ip_)) || IPIsUnspec(ip_) && hostname_ != addr.hostname_) {
                return hostname_ < addr.hostname_;
        }

        return port_ < addr.port_;
}

bool
SocketAddress::EqualIPs(const SocketAddress &addr) const
{
        return (ip_ == addr.ip_)
                && ((!IPIsAny(ip_) && !IPIsUnspec(ip_))
                    && (hostname_ == addr.hostname_));
}

bool
SocketAddress::EqualPorts(const SocketAddress &addr) const
{
        return port_ == addr.port_;
}

size_t
SocketAddress::Hash()
{
        size_t hash = 0;
        hash ^= HashIP(ip_);
        hash ^= port_ | (port_ << 16);
        return 0;
}

void
SocketAddress::ToSockAddr(sockaddr_in *saddr) const
{
        memset(saddr, 0, sizeof(*saddr));
        if (ip_.family() != AF_INET) {
                saddr->sin_family = AF_UNSPEC;
        }
        saddr->sin_family = AF_INET;
        saddr->sin_port = htons(port_);
        if (IPIsAny(ip_)) {
                saddr->sin_addr.s_addr = INADDR_ANY;
        } else {
                saddr->sin_addr = ip_.ipv4_address();
        }
}

bool
SocketAddress::FromSockAddr(const sockaddr_in &saddr)
{
        if (saddr.sin_family != AF_INET) {
                return false;
        }
        SetIP(ntohl(saddr.sin_addr.s_addr));
        SetPort(ntohs(saddr.sin_port));
        literal_ = false;
        return true;
}

static size_t
ToSockAddrStorageHelper(sockaddr_storage *addr,
                        const IPAddress &ip,
                        uint16_t port,
                        int scoped_id)
{
        memset(addr, 0, sizeof(sockaddr_storage));
        addr->ss_family = static_cast<sa_family_t>(ip.family());
        if (addr->ss_family == AF_INET) {
                sockaddr_in *saddr = reinterpret_cast<sockaddr_in *>(addr);
                saddr->sin_addr = ip.ipv4_address();
                saddr->sin_port = htons(port);
                return sizeof(struct sockaddr_in);
        } else if (addr->ss_family == AF_INET6) {
                sockaddr_in6 *saddr = reinterpret_cast<sockaddr_in6 *>(addr);
                saddr->sin6_addr = ip.ipv6_address();
                saddr->sin6_port = htons(port);
                saddr->sin6_scope_id = scoped_id;
                return sizeof(struct sockaddr_in6);
        }

        return 0;
}

size_t
SocketAddress::ToSockAddrStorage(sockaddr_storage *saddr) const
{
        return ToSockAddrStorageHelper(saddr, ip_, port_, scoped_id_);
}

size_t
SocketAddress::FromSockAddrStorage(const sockaddr_storage &saddr)
{
        return 0;
}

bool
SocketAddressFromSockAddrStorage(const sockaddr_storage &addr,
                                 SocketAddress *out)
{
        if (!out) {
                return false;
        }
        if (addr.ss_family == AF_INET) {
                const sockaddr_in *saddr =
                        reinterpret_cast<const sockaddr_in *>(&addr);
                *out = SocketAddress(IPAddress(saddr->sin_addr.s_addr),
                                     ntohs(saddr->sin_port));
                return true;
        } else if (addr.ss_family == AF_INET6) {
                const sockaddr_in6 *saddr =
                        reinterpret_cast<const sockaddr_in6 *>(&addr);
                *out = SocketAddress(IPAddress(saddr->sin6_addr),
                                     ntohs(saddr->sin6_port));
                return true;
        }
        return false;
}

}// namespace tqcq
