//
// Created by tqcq on 2023/9/6.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_NULL_SOCKET_SERVER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_NULL_SOCKET_SERVER_H_

#include "socket_server.h"
#include "u-toolbox/concurrency/event.h"

namespace tqcq {
class NullSocketServer : public SocketServer {
public:
        NullSocketServer();
        ~NullSocketServer() override;

        bool Wait(int64_t max_wait_duration, bool process_io) override;
        void WakeUp() override;

        Socket *CreateSocket(int family, int type) override;

private:
        Event event_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_NULL_SOCKET_SERVER_H_
