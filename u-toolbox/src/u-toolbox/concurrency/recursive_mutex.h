//
// Created by tqcq on 2023/9/13.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_RECURSIVE_MUTEX_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_RECURSIVE_MUTEX_H_
#include <pthread.h>

namespace tqcq {
class RecursiveMutex {
public:
        RecursiveMutex();
        ~RecursiveMutex();
        void Lock();
        void Unlock();

private:
        pthread_mutex_t mutex_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_RECURSIVE_MUTEX_H_
