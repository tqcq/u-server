//
// Created by tqcq on 2023/9/3.
//

#include "task_queue_base.h"
#include "u-toolbox/log/u_log.h"
#include <pthread.h>

namespace tqcq {
pthread_key_t g_queue_ptr_tls = 0;

void
InitializeTls()
{
        U_CHECK(pthread_key_create(&g_queue_ptr_tls, nullptr) == 0);
}

pthread_key_t
GetQueuePtrTls()
{
        static pthread_once_t init_once = PTHREAD_ONCE_INIT;
        U_CHECK(pthread_once(&init_once, &InitializeTls) == 0);
        return g_queue_ptr_tls;
}

TaskQueueBase *
TaskQueueBase::Current()
{
        return static_cast<TaskQueueBase *>(
                pthread_getspecific(GetQueuePtrTls()));
}

TaskQueueBase::CurrentTaskQueueSetter::CurrentTaskQueueSetter(
        TaskQueueBase *task_queue)
    : previous_(TaskQueueBase::Current())
{
        pthread_setspecific(GetQueuePtrTls(), task_queue);
}

TaskQueueBase::CurrentTaskQueueSetter::~CurrentTaskQueueSetter()
{
        pthread_setspecific(GetQueuePtrTls(), previous_);
}
}// namespace tqcq
