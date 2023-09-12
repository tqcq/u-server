//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_CONDITION_VARIABLE_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_CONDITION_VARIABLE_H_

#include "u-toolbox/base/non_copyable.h"
#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/concurrency/thread_annotations.h"
#include <list>
#include <pthread.h>

namespace tqcq {

class ConditionVariable : NonCopyable {
public:
        ConditionVariable();

        ConditionVariable(const ConditionVariable &) = delete;
        ~ConditionVariable();

        template<typename Predicate>
        void Wait(Mutex &lock, Predicate &&pred)
        {
                while (!pred()) { pthread_cond_wait(&cond_, &lock.mutex_); }
        }

        void NotifyAll();
        void NotifyOne();

private:
        pthread_cond_t cond_;
};

}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_CONDITION_VARIABLE_H_
