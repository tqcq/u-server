//
// Created by tqcq on 2023/9/6.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_EVENT_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_EVENT_H_

#include "condition_variable.h"
#include "mutex.h"
#include "u-toolbox/base/non_copyable.h"

namespace tqcq {
class Event : NonCopyable {
public:
        Event();
        Event(bool manual_reset, bool initially_signaled);
        ~Event();

        void Set();
        void Reset();

        //TODO maybe support timeout?
        bool Wait();
        bool Wait(int64_t give_up_after);

private:
        Mutex mutex_;
        pthread_cond_t cond_;
        // ConditionVariable cv_;
        bool manual_reset_;
        bool event_status_ U_GUARDED_BY(mutex_);
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_EVENT_H_
