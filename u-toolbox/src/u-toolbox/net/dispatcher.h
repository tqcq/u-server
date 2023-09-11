//
// Created by tqcq on 2023/9/7.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHER_H_

#include <stdint.h>

namespace tqcq {
class Dispatcher {
public:
        virtual ~Dispatcher() = default;
        virtual uint32_t GetRequestEvents() = 0;
        virtual void OnEvent(uint32_t ff, int err) = 0;

        virtual int GetDescriptor() const = 0;
        virtual bool IsDescriptorClosed() const = 0;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_NET_DISPATCHER_H_
