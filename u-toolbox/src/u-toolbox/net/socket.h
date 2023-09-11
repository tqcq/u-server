//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_H_

#include "u-toolbox/base/non_copyable.h"
#include "u-toolbox/third_party/sigslot/sigslot.h"
#ifndef SOCKET
typedef int SOCKET;
#endif

namespace tqcq {

class SocketAddress;

class Socket : NonCopyable {
public:
        virtual ~Socket() = default;

        virtual int Bind(const SocketAddress &addr) = 0;
        virtual int Connect(const SocketAddress &addr) = 0;
        virtual int Send(const void *pv, size_t cb) = 0;
        virtual int SendTo(const void *pv, size_t cb,
                           const SocketAddress &addr) = 0;
        virtual int Recv(void *pv, size_t cb, int64_t *timestamp) = 0;
        virtual int RecvFrom(void *pv, size_t cb, SocketAddress *paddr,
                             int64_t *timestamp) = 0;
        virtual int Listen(int backlog) = 0;
        virtual Socket *Accept(SocketAddress *paddr) = 0;
        virtual int Close() = 0;
        virtual int GetErrror() const = 0;
        virtual void SetError(int error) = 0;
        inline bool IsBlocking() const;

        enum ConnState {
                CS_CLOSED,
                CS_CONNECTING,
                CS_CONNECTED,
        };

        virtual ConnState GetState() const = 0;

        enum Option {
                OPT_DONTFRAGMENT,
                OPT_RCVBUF,
                OPT_SNDBUF,
                OPT_NODELAY,
                OPT_IPV6_V6ONLY,
                OPT_DSCP,
                OPT_RTP_SENDTIME_EXTN_ID,
        };

        virtual int GetOption(Option opt, int *value) = 0;
        virtual int SetOption(Option opt, int value) = 0;

        // ready to read
        sigslot::signal1<Socket *, sigslot::multi_threaded_local>
                SignalReadEvent;
        // ready to write
        sigslot::signal1<Socket *, sigslot::multi_threaded_local>
                SignalWriteEvent;
        // connected
        sigslot::signal1<Socket *> SignalConnectEvent;
        // closed
        sigslot::signal2<Socket *, int> SignalCloseEvent;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_SOCKET_H_
