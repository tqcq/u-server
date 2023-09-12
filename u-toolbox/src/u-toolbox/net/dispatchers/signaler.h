//
// Created by tqcq on 2023/9/12.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SIGNALER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SIGNALER_H_
#include "u-toolbox/net/dispatcher.h"
#include "u-toolbox/net/physical_socket_server.h"
#include "u-toolbox/net/sockets/physical_socket.h"

namespace tqcq {
// 独立出来一个 Signaler 用于唤醒阻塞的 PhysicalSocketServer
class Signaler : public Dispatcher {
public:
        Signaler(PhysicalSocketServer *ss, bool &flag_to_clear);
        ~Signaler() override;
        virtual void Signal();
        uint32_t GetRequestedEvents() override;
        void OnEvent(uint32_t ff, int err) override;
        int GetDescriptor() const override;
        //  保持和 PhysicalSocketServer 生命周期一致，始终返回 true
        bool IsDescriptorClosed() const override;

private:
        PhysicalSocketServer *ss_;
        const std::array<int, 2> afd_;
        bool f_signaled_ U_GUARDED_BY(mutex_);
        Mutex mutex_;
        bool &flag_to_clear_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SIGNALER_H_
