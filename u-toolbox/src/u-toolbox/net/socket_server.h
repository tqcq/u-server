//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_SERVER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_SERVER_H_

#include "socket_factory.h"
#include <memory>

// #include "u-toolbox/concurrency/thread.h"

namespace tqcq {
class Thread;

class SocketServer : public SocketFactory {
public:
        static constexpr int64_t kForever = -1;
        static std::unique_ptr<SocketServer> CreateDefault();

        virtual void SetMessageQueue(Thread *queue) {}

        /**
         * @brief
         * @param max_wait_duration milliseconds (kForever is -1)
         * @return
         */
        virtual bool Wait(int64_t max_wait_duration, bool process_io) = 0;
        virtual void WakeUp() = 0;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_SOCKET_SERVER_H_
