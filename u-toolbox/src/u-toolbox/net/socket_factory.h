//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_FACTORY_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_FACTORY_H_

#include "u-toolbox/net/socket.h"

namespace tqcq {
class SocketFactory {
public:
        virtual ~SocketFactory() = default;
        virtual Socket *CreateSocket(int family, int type) = 0;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_FACTORY_H_
