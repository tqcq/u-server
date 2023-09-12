//
// Created by tqcq on 2023/9/6.
//

#include "event.h"
#include "lock_guard.h"
#include <ios>

namespace tqcq {
Event::Event() : Event(false, false) {}

Event::Event(bool manual_reset, bool initially_signaled)
    : manual_reset_(manual_reset),
      event_status_(initially_signaled)
{
        pthread_cond_init(&cond_, nullptr);
}

Event::~Event() { pthread_cond_destroy(&cond_); }

void
Event::Set()
{
        LockGuard<Mutex> lock(mutex_);
        event_status_ = true;
        pthread_cond_broadcast(&cond_);
}

void
Event::Reset()
{
        LockGuard<Mutex> lock(mutex_);
        event_status_ = false;
}

bool
Event::Wait()
{
        LockGuard<Mutex> lock(mutex_);
        while (!event_status_) { pthread_cond_wait(&cond_, mutex_.mutex()); }
        if (!manual_reset_) {
                event_status_ = false;
        }
        return true;
}

bool
Event::Wait(int64_t give_up_after)
{
        LockGuard<Mutex> lock(mutex_);
        struct timespec ts;
        ts.tv_sec = give_up_after / 1000;
        ts.tv_nsec = (give_up_after % 1000) * 1000000;
        while (!event_status_) {
                if (pthread_cond_timedwait(&cond_, mutex_.mutex(), &ts) != 0) {
                        // 超时
                        return false;
                }
        }
        if (!manual_reset_) {
                event_status_ = false;
        }
        return true;
}
}// namespace tqcq
