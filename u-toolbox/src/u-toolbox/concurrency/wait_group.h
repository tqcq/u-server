//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_WAIT_GROUP_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_WAIT_GROUP_H_

#include "condition_variable.h"
#include "lock_guard.h"
#include "mutex.h"
#include "u-toolbox/log/u_log.h"
#include <atomic>
#include <condition_variable>
#include <memory>

namespace tqcq {
class WaitGroup {
public:
        inline WaitGroup(uint32_t initialCount = 0);
        inline void Add(uint32_t count = 1) const;
        inline bool Done() const;
        inline void Wait() const;

private:
        struct Data {
                std::atomic<uint32_t> count = {0};
                Mutex mutex;
                ConditionVariable cv;
        };

        const std::shared_ptr<Data> data_;
};

WaitGroup::WaitGroup(uint32_t initialCount) { data_->count = initialCount; }

void
WaitGroup::Add(uint32_t count) const
{
        U_ASSERT(count > 0, "WaitGroup count must be greater than 0");
        data_->count += count;
}

bool
WaitGroup::Done() const
{
        U_ASSERT(data_->count > 0, "WaitGroup count must be greater than 0");
        auto count = --data_->count;
        if (count == 0) {
                LockGuard lock(data_->mutex);
                data_->cv.NotifyAll();
                return true;
        }
}

void
WaitGroup::Wait() const
{
        LockGuard lock(data_->mutex);
        data_->cv.Wait(data_->mutex, [this] { return data_->count == 0; });
}

}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_WAIT_GROUP_H_
