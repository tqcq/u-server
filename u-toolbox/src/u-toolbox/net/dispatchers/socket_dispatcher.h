//
// Created by tqcq on 2023/9/7.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_

#include "u-toolbox/base/config.h"
#include "u-toolbox/net/dispatcher.h"
#include "u-toolbox/net/sockets/physical_socket.h"

namespace tqcq {
// Dispatcher 可以实现基于事件的响应，便于分类处理各种事件
class SocketDispatcher : public Dispatcher, public PhysicalSocket {
public:
        // 创建一个新的 Socket
        explicit SocketDispatcher(PhysicalSocketServer *ss);

        // 从存在的 socket_fd 创建一个 Socket
        SocketDispatcher(SOCKET s, PhysicalSocketServer *ss);
        ~SocketDispatcher() override;

        bool Initialize();

        virtual bool Create(int type);
        bool Create(int family, int type) override;

        int GetDescriptor() const override;
        bool IsDescriptorClosed() const override;

        uint32_t GetRequestedEvents() override;
        /**
         * @brief
         * @param ff DispatchEvent 按位或
         * @param err
         */
        void OnEvent(uint32_t ff, int err) override;

        int Close() override;

#if defined(U_USE_EPOLL)
protected:
        void StartBatchedEventUpdates();
        void FinishBatchedEventUpdates();

        void SetEnabledEvents(uint8_t events) override;
        void EnableEvents(uint8_t events) override;
        void DisableEvents(uint8_t events) override;
#endif

#if defined(U_USE_EPOLL)
private:
        void MayBeUpdateDispatcher(uint8_t old_events);
        int saved_enabled_events_ = -1;
#endif
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHERS_SOCKET_DISPATCHER_H_
