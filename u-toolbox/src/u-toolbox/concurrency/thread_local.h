//
// Created by tqcq on 2023/9/6.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_THREAD_LOCAL_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_THREAD_LOCAL_H_
#include "u-toolbox/base/non_copyable.h"
#include <pthread.h>

namespace tqcq {
template<typename T>
class ThreadLocal : NonCopyable {
public:
        inline ThreadLocal(T v)
        {
                pthread_key_create(&key_, nullptr);
                pthread_setspecific(key_, v);
        }

        inline ~ThreadLocal() { pthread_key_delete(key_); }

        inline operator T() const
        {
                return static_cast<T>(pthread_getspecific(key_));
        }

        inline ThreadLocal &operator=(T v) { pthread_setspecific(key_, v); }

private:
        pthread_key_t key_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_THREAD_LOCAL_H_
