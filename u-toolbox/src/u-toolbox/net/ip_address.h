//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_IP_ADDRESS_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_IP_ADDRESS_H_

#include <netdb.h>
#include <string.h>
#include <string>

namespace tqcq {
class IPAddress {
public:
        IPAddress() : family_(AF_UNSPEC) { ::bzero(&u_, sizeof(u_)); }

        explicit IPAddress(const in_addr &ipv4) : family_(AF_INET)
        {
                bzero(&u_, sizeof(u_));
                u_.ipv4 = ipv4;
        }

        explicit IPAddress(const in6_addr &ipv6) : family_(AF_INET6)
        {
                // bzero(&u_, sizeof(u_));
                u_.ipv6 = ipv6;
        }

        explicit IPAddress(uint32_t ip_in_host_byte_order) : family_(AF_INET)
        {
                bzero(&u_, sizeof(u_));
                u_.ipv4.s_addr = htonl(ip_in_host_byte_order);
        }

        IPAddress(const IPAddress &other) : family_(other.family_)
        {
                ::memcpy(&u_, &other.u_, sizeof(u_));
        }

        virtual ~IPAddress() {}

        const IPAddress &operator=(const IPAddress &other)
        {
                family_ = other.family_;
                ::memcpy(&u_, &other.u_, sizeof(u_));
                return *this;
        }

        /**
         * @brief 判断两个 IPAddress 是否相等，有两种情况
         * 1. family 类型相同、地址相同
         * 2。family == AF_UNSPEC 类型，和任何类型相等
         */
        bool operator==(const IPAddress &other) const;
        bool operator!=(const IPAddress &other) const;

        int family() const { return family_; }

        in_addr ipv4_address() const;
        in6_addr ipv6_address() const;
        // 当前类型地址所占用的字节数
        size_t Size() const;
        // inet_ntop 函数的实现
        std::string ToString();

        IPAddress AsIPv6Address() const;
        uint32_t v4AddressAsHostOrderInteger() const;

        // 头部负载
        int overhead() const;
        // 判断是不是 unspecified 地址
        bool IsNil() const;

private:
        int family_;

        union {
                in_addr ipv4;
                in6_addr ipv6;
        } u_;
};

bool IPFromAddrInfo(struct addrinfo *info, IPAddress *out);
bool IPFromString(const std::string &str, IPAddress *out);
bool IPIsUnspec(const IPAddress &ip);
bool IPIsAny(const IPAddress &ip);
bool IPIsLoopback(const IPAddress &ip);
bool IPIsLinkLocal(const IPAddress &ip);
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_IP_ADDRESS_H_
