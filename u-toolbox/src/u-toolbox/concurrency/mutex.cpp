//
// Created by tqcq on 2023/9/4.
//

#include "mutex.h"
#include "u-toolbox/log/u_log.h"
#include <pthread.h>

namespace tqcq {
void
Mutex::Lock()
{
        U_CHECK(pthread_mutex_lock(&mutex_) == 0);
}

void
Mutex::Unlock()
{
        U_CHECK(pthread_mutex_unlock(&mutex_) == 0);
}

Mutex::Mutex() { U_CHECK(pthread_mutex_init(&mutex_, nullptr) == 0); }

Mutex::~Mutex() { U_CHECK(pthread_mutex_destroy(&mutex_) == 0); }

pthread_mutex_t *
Mutex::mutex() const
{
        return &mutex_;
}
}// namespace tqcq
