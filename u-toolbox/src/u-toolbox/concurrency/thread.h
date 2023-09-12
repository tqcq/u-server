//
// Created by tqcq on 2023/9/3.
//

#ifndef HTTP_SERVER_SRC_CONCORRENCE_THREAD_H_
#define HTTP_SERVER_SRC_CONCORRENCE_THREAD_H_

#include "lock_guard.h"
#include "mutex.h"
#include "u-toolbox/api/task_queue_base.h"
#include "u-toolbox/base/non_copyable.h"
#include "u-toolbox/concurrency/thread_annotations.h"
#include "u-toolbox/net/socket_server.h"
#include <atomic>
#include <mutex>
#include <pthread.h>
#include <queue>
#include <string>

namespace tqcq {
class Thread : NonCopyable, public TaskQueueBase {
public:
        static const int kForever = -1;

        ~Thread() override;
        explicit Thread(SocketServer *ss);
        explicit Thread(std::unique_ptr<SocketServer> ss);
        Thread(SocketServer *ss, bool do_init);
        Thread(std::unique_ptr<SocketServer> ss, bool do_init);

        static std::unique_ptr<Thread> CreateWithSocketServer();
        static std::unique_ptr<Thread> Create();

        void Delete() override;

        /**
         * @brief 处理I/O 和 分发消息，直到发生以下情况
         * 1. cms milliseconds 被耗尽 (return true)
         * 2. Stop() 被调用 (return false)
         */
        bool ProcessMessages(int cms);

        // 判断这个 Thread 对象是否是当前正在运行的线程
        bool IsCurrent() const;

        bool Start();
        void Stop();
        virtual void Restart();
        virtual void Run();
        virtual void Quit();
        virtual bool IsQuitting();
        void Join();
        void SetName(const std::string &name);

protected:
        struct DelayedMessage {
                bool operator<(const DelayedMessage &rhs) const
                {
                        return (rhs.run_time_ms < run_time_ms)
                                || ((rhs.run_time_ms == run_time_ms)
                                    && (rhs.message_number < message_number));
                }

                int64_t delay_ms;
                int64_t run_time_ms;
                uint32_t message_number;
                mutable std::function<void()> functor;
        };

        void PostTaskImpl(std::function<void()> task,
                          const PostTaskTraits &traits) override;

        void PostDelayedTaskImpl(std::function<void()> task,
                                 int delay_ms,
                                 const PostDelayedTaskTraits &traits) override;

        void Dispatch(std::function<void()> task);
        bool IsRunning();
        void WakeUpSocketServer();
        void DoInit();

        void DoDestroy();

private:
        static void *PreRun(void *pv);

        /**
         * @brief 等待IO事件，直到发生以下情况
         * 1. 一个任务被分发 (return it)
         * 2. cms 超时 (return empty task)
         * 3. Stop() 被调用 (return empty task)
         * @param cms
         * @return
         */
        std::function<void()> Get(int cms_wait);
        /**
         * @brief
         * @return kForever if no delayed messages,
         *         otherwise the delay until the next delayed message
         */
        int64_t ProcessDelayedMessage(int64_t cms_now);

        static const int kSlowDispatchLoggingThreshold = 50;// 50 ms
        std::queue<std::function<void()>> messages_ U_GUARDED_BY(mutex_);
        std::priority_queue<DelayedMessage>
                delayed_messages_ U_GUARDED_BY(mutex_);
        uint32_t delayed_next_num_ U_GUARDED_BY(mutex_);
        mutable Mutex mutex_;

        bool initialized_;
        bool destroyed_;
        std::atomic<int> stop_;

        SocketServer *ss_;
        std::unique_ptr<SocketServer> own_ss_;

        std::string name_;
        pthread_t thread_;

        std::unique_ptr<TaskQueueBase::CurrentTaskQueueSetter>
                task_queue_registration_;

        int dispatch_warning_ms_ U_GUARDED_BY(this) =
                kSlowDispatchLoggingThreshold;
};

}// namespace tqcq

#endif//HTTP_SERVER_SRC_CONCORRENCE_THREAD_H_
