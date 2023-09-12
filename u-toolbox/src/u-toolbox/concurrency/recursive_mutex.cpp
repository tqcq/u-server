//
// Created by tqcq on 2023/9/13.
//

#include "recursive_mutex.h"

namespace tqcq {
RecursiveMutex::RecursiveMutex()
{
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mutex_, &attr);
}

RecursiveMutex::~RecursiveMutex() { pthread_mutex_destroy(&mutex_); }

void
RecursiveMutex::Lock()
{
        pthread_mutex_lock(&mutex_);
}

void
RecursiveMutex::Unlock()
{
        pthread_mutex_unlock(&mutex_);
}
}// namespace tqcq
