//
// Created by tqcq on 2023/9/6.
//

#include "u-toolbox/concurrency/condition_variable.h"

namespace tqcq {

ConditionVariable::ConditionVariable() { pthread_cond_init(&cond_, nullptr); }

ConditionVariable::~ConditionVariable() { pthread_cond_destroy(&cond_); }

void
ConditionVariable::NotifyAll()
{
        pthread_cond_signal(&cond_);
}

void
ConditionVariable::NotifyOne()
{
        pthread_cond_broadcast(&cond_);
}

}// namespace tqcq