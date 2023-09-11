//
// Created by tqcq on 2023/9/7.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_

#include "u-toolbox/net/dispatcher.h"

namespace tqcq {
class SocketDispatcher : public Dispatcher {
public:
        ~SocketDispatcher() override = default;
        uint32_t GetRequestEvents() override;
        void OnEvent(uint32_t ff, int err) override;
        int GetDescriptor() const override;
        bool IsDescriptorClosed() const override;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_
