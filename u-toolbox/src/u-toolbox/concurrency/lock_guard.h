//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_

#include "mutex.h"
#include "u-toolbox/base/non_copyable.h"
#include <mutex>

namespace tqcq {
class LockGuard : NonCopyable {
public:
        explicit LockGuard(Mutex &mutex) : mutex_(mutex) { mutex_.Lock(); }

        ~LockGuard() { mutex_.Unlock(); }

        // inner
        friend class ConditionVariable;

private:
        pthread_mutex_t *mutex() const { return mutex_.mutex(); }

        Mutex &mutex_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_
