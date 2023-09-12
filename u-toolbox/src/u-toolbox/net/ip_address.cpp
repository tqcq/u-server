//
// Created by tqcq on 2023/9/4.
//

#include "ip_address.h"
#include "u-toolbox/log/u_log.h"
#include <arpa/inet.h>
#include <netdb.h>

namespace tqcq {

static const in6_addr kV4MappedPrefix = {
        {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, 0, 0, 0, 0}}};

IPAddress::IPAddress(const std::string &ip)
{
        bool success = IPFromString(ip, this);
        U_ASSERT(success, "IPFromString failed");
}

bool
IPAddress::operator==(const IPAddress &other) const
{
        if (family_ != other.family_) {
                return false;
        }
        if (family_ == AF_INET) {
                return !memcmp(&u_.ipv4, &other.u_.ipv4, sizeof(u_.ipv4));
        }
        if (family_ == AF_INET6) {
                return !memcmp(&u_.ipv6, &other.u_.ipv6, sizeof(u_.ipv6));
        }

        return family_ == AF_UNSPEC;
}

bool
IPAddress::operator!=(const IPAddress &other) const
{
        return !((*this) == other);
}

in_addr
IPAddress::ipv4_address() const
{
        return u_.ipv4;
}

in6_addr
IPAddress::ipv6_address() const
{
        return u_.ipv6;
}

size_t
IPAddress::Size() const
{
        switch (family_) {
        case AF_INET:
                return sizeof(in_addr);
        case AF_INET6:
                return sizeof(in6_addr);
        }
        return 0;
}

std::string
IPAddress::ToString() const
{
        if (family_ != AF_INET && family_ != AF_INET6) {
                return {};
        }

        char buf[INET6_ADDRSTRLEN] = {0};
        const void *src = &u_.ipv4;
        if (family_ == AF_INET6) {
                src = &u_.ipv6;
        }
        // 将 src 指向的地址转换为字符串形式的 IP 地址，存放在 buf 中
        if (!inet_ntop(family_, src, buf, sizeof(buf))) {
                return {};
        }

        return {buf};
}

IPAddress
IPAddress::AsIPv6Address() const
{
        if (family_ == AF_INET6) {
                return *this;
        }
        // convert ipv4 to ipv6
        in6_addr v6addr = kV4MappedPrefix;
        ::memcpy(&v6addr.s6_addr[12], &u_.ipv4, sizeof(u_.ipv4));
        return IPAddress(v6addr);
}

uint32_t
IPAddress::v4AddressAsHostOrderInteger() const
{
        if (family_ == AF_INET) {
                return ntohl(u_.ipv4.s_addr);
        } else {
                return 0;
        }
}

int
IPAddress::overhead() const
{
        switch (family_) {
        case AF_INET:
                return 20;
        case AF_INET6:
                return 40;
        default:
                return 0;
        }
}

bool
IPAddress::IsNil() const
{
        return IPIsUnspec(*this);
}

bool
IPAddress::operator<(const IPAddress &other) const
{
        if (family_ != other.family_) {
                if (family_ == AF_UNSPEC) {
                        return true;
                }
                if (family_ == AF_INET && other.family_ == AF_INET6) {
                        return true;
                }
                return false;
        }

        switch (family_) {
        case AF_INET: {
                return v4AddressAsHostOrderInteger()
                        < other.v4AddressAsHostOrderInteger();
        }
        case AF_INET6: {
                return memcmp(&u_.ipv6, &other.u_.ipv6, 16) < 0;
        }
        }

        return false;
}

bool
IPFromAddrInfo(struct addrinfo *info, IPAddress *out)
{
        if (!info || !info->ai_addr) {
                return false;
        }

        U_CHECK(!out);

        if (info->ai_addr->sa_family == AF_INET) {
                sockaddr_in *addr =
                        reinterpret_cast<sockaddr_in *>(info->ai_addr);
                *out = IPAddress(addr->sin_addr);
                return true;
        } else if (info->ai_addr->sa_family == AF_INET6) {
                sockaddr_in6 *addr =
                        reinterpret_cast<sockaddr_in6 *>(info->ai_addr);
                *out = IPAddress(addr->sin6_addr);
                return true;
        }

        return false;
}

bool
IPFromString(const std::string &str, IPAddress *out)
{
        if (!out) {
                return false;
        }
        // test convert to ipv4
        in_addr addr;
        if (::inet_pton(AF_INET, str.c_str(), &addr) == 1) {
                *out = IPAddress(addr);
                return true;
        }
        in6_addr addr6;
        if (::inet_pton(AF_INET6, str.c_str(), &addr6) == 1) {
                *out = IPAddress(addr6);
                return true;
        }
        *out = IPAddress();
        return false;
}

bool
IPIsUnspec(const IPAddress &ip)
{
        return ip.family() == AF_UNSPEC;
}

bool
IPIsAny(const IPAddress &ip)
{
        switch (ip.family()) {
        case AF_INET:
                return ip == IPAddress(INADDR_ANY);
        case AF_INET6:
                /** ip6addr_any 或者 IPv4 的 0.0.0.0 */
                return ip == IPAddress(in6addr_any)
                        || ip == IPAddress(kV4MappedPrefix);
        case AF_UNSPEC:
                return false;
        }
        return false;
}

static bool
IPIsLoopbackV4(const IPAddress &ip)
{
        uint32_t ip_in_host_order = ip.v4AddressAsHostOrderInteger();
        return ((ip_in_host_order) >> 24) == 127;
}

static bool
IPIsLoopbackV6(const IPAddress &ip)
{
        return ip == IPAddress(in6addr_loopback);
}

bool
IPIsLoopback(const IPAddress &ip)
{
        switch (ip.family()) {
        case AF_INET:
                return IPIsLoopbackV4(ip);
        case AF_INET6:
                return IPIsLoopbackV6(ip);
        }
        return false;
}

bool
IPIsLinkLocal(const IPAddress &ip)
{
        return false;
}

static bool
IPIsPrivateNetworkV4(const IPAddress &ip)
{
        uint32_t ip_in_host_order = ip.v4AddressAsHostOrderInteger();
        // 10.xxx.xxx.xxx
        // 172.16.000.000- 172.31.255.255
        // 192.168.xxx.xxx
        return ((ip_in_host_order >> 24) == 10)
                || ((ip_in_host_order >> 20) == ((172 << 4) | 1))
                || ((ip_in_host_order >> 16) == ((192 << 8) | 168));
}

static bool
IPIsPrivateNetworkV6(const IPAddress &ip)
{
        static const in6_addr kPrivateNetworkPrefix = {{{0xFD}}};

        in6_addr addr = ip.ipv6_address();
        return ::memcmp(&addr, &kPrivateNetworkPrefix, 1) == 0;
}

bool
IPIsPrivate(const IPAddress &ip)
{
        switch (ip.family()) {
        case AF_INET:
                return IPIsPrivateNetworkV4(ip);
        case AF_INET6:
                return IPIsPrivateNetworkV6(ip);
        }

        return false;
}

size_t
HashIP(const IPAddress &ip)
{
        switch (ip.family()) {
        case AF_INET: {
                return ip.ipv4_address().s_addr;
        }
        case AF_INET6: {
                in6_addr v6addr = ip.ipv6_address();
                const uint32_t *v6_as_ints =
                        reinterpret_cast<const uint32_t *>(&v6addr.s6_addr);
                return v6_as_ints[0] ^ v6_as_ints[1] ^ v6_as_ints[2]
                        ^ v6_as_ints[3];
        }
        }
        return 0;
}
}// namespace tqcq
