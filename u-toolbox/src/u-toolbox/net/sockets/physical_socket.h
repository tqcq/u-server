//
// Created by tqcq on 2023/9/7.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKETS_PHYSICAL_SOCKET_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKETS_PHYSICAL_SOCKET_H_

#include "u-toolbox/concurrency/mutex.h"
#include "u-toolbox/concurrency/thread_annotations.h"
#include "u-toolbox/net/socket.h"
#include "u-toolbox/third_party/sigslot/sigslot.h"
#include <pcap/socket.h>

namespace tqcq {

class SocketServer;
class PhysicalSocketServer;

class PhysicalSocket : public Socket, public sigslot::has_slots<> {
public:
        PhysicalSocket(PhysicalSocketServer *ss, SOCKET s = INVALID_SOCKET);
        ~PhysicalSocket() override;

        virtual bool Create(int family, int type);

        int Bind(const SocketAddress &addr) override;
        int Connect(const SocketAddress &addr) override;
        int Send(const void *pv, size_t cb) override;
        int SendTo(const void *pv, size_t cb,
                   const SocketAddress &addr) override;
        int Recv(void *pv, size_t cb, int64_t *timestamp) override;
        int RecvFrom(void *pv, size_t cb, SocketAddress *paddr,
                     int64_t *timestamp) override;
        int Listen(int backlog) override;
        Socket *Accept(SocketAddress *paddr) override;
        int Close() override;
        int GetErrror() const override;
        void SetError(int error) override;
        ConnState GetState() const override;
        int GetOption(Option opt, int *value) override;
        int SetOption(Option opt, int value) override;

        SocketServer *socket_server();

protected:
        int DoConnect(const SocketAddress &connect_addr);
        virtual SOCKET DoAccept(SOCKET socket, sockaddr *addr,
                                socklen_t *addrlen);
        virtual int DoSend(SOCKET socket, const char *buf, int len, int flags);
        virtual int DoSendTo(SOCKET socket, const char *buf, int len, int flags,
                             const struct sockaddr *dest_addr,
                             socklen_t addrlen);

        PhysicalSocketServer *ss_;
        SOCKET s_;
        bool udp_;
        int family_ = 0;
        mutable Mutex mutex_;
        int error_ U_GUARDED_BY(mutex_);
        ConnState state_;

private:
        uint8_t enabled_events_ = 0;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKETS_PHYSICAL_SOCKET_H_
