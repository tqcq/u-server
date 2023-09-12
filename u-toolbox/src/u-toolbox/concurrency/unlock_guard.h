//
// Created by tqcq on 2023/9/13.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_UNLOCK_GUARD_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_UNLOCK_GUARD_H_

#include "mutex.h"

namespace tqcq {

class UnlockGuard {
public:
        explicit UnlockGuard(Mutex &mutex) : mutex_(mutex) { mutex_.Unlock(); }

        ~UnlockGuard() { mutex_.Lock(); }

        // inner
        friend class ConditionVariable;

private:
        Mutex &mutex_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_UNLOCK_GUARD_H_
