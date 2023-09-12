//
// Created by tqcq on 2023/9/11.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_ADDRESS_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_ADDRESS_H_
#include "ip_address.h"
#include <netdb.h>
#include <stdint.h>

namespace tqcq {
class SocketAddress {
public:
        SocketAddress();
        SocketAddress(uint32_t ip_as_host_order_integer, int port);
        SocketAddress(const IPAddress &ip, int port);
        SocketAddress(const SocketAddress &addr);

        void Clear();
        bool IsNil() const;
        bool IsComplete() const;
        SocketAddress &operator=(const SocketAddress &addr);
        // ipv4: 1.1.1.1, ipv6: ::xxxx
        void SetIP(const std::string &ip);
        void SetIP(uint32_t ip_as_host_order_integer);
        void SetIP(const IPAddress &ip);
        void SetPort(int port);
        void SetScopeID(int id);

        const std::string &hostname() const { return hostname_; }

        uint32_t ip() const;
        int family() const;
        const IPAddress &ipaddr() const;
        uint16_t port() const;

        std::string HostAsURIString() const;
        std::string ToString() const;
        // address:port and [address]:port
        bool FromString(const std::string &str);

        bool IsAnyIP() const;
        bool IsLoopBackIP() const;
        bool IsPrivateIP() const;
        bool operator==(const SocketAddress &addr) const;

        inline bool operator!=(const SocketAddress &addr) const
        {
                return !this->operator==(addr);
        }

        bool operator<(const SocketAddress &addr) const;
        bool EqualIPs(const SocketAddress &addr) const;
        bool EqualPorts(const SocketAddress &addr) const;
        size_t Hash();

        void ToSockAddr(sockaddr_in *saddr) const;

        bool FromSockAddr(const sockaddr_in &saddr);

        size_t ToSockAddrStorage(sockaddr_storage *saddr) const;
        size_t FromSockAddrStorage(const sockaddr_storage &saddr);

private:
        std::string hostname_;
        IPAddress ip_;
        uint16_t port_;

        // for IPv6
        int scoped_id_;
        bool literal_;
};

bool SocketAddressFromSockAddrStorage(const sockaddr_storage &saddr,
                                      SocketAddress *out);
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_ADDRESS_H_
