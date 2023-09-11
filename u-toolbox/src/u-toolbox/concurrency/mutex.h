//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_MUTEX_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_MUTEX_H_

#include "u-toolbox/base/non_copyable.h"
#include <pthread.h>

namespace tqcq {

class Mutex : NonCopyable {
public:
        Mutex();
        ~Mutex();
        void Lock();
        void Unlock();

        // inner
        friend class LockGuard;
        friend class ConditionVariable;
        friend class Event;

private:
        pthread_mutex_t *mutex() const;
        mutable pthread_mutex_t mutex_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_MUTEX_H_
