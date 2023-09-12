//
// Created by tqcq on 2023/9/3.
//

#include "thread.h"
#include "u-toolbox/api/task_queue_base.h"
#include "u-toolbox/log/u_log.h"
#include "u-toolbox/net/null_socket_server.h"
#include "u-toolbox/system/time_utils.h"

namespace tqcq {
pthread_key_t current;

Thread::Thread(SocketServer *ss) : Thread(ss, true) {}

Thread::Thread(std::unique_ptr<SocketServer> ss) : Thread(std::move(ss), true)
{}

Thread::Thread(SocketServer *ss, bool do_init)
    : ss_(ss),
      delayed_next_num_(1),
      initialized_(false),
      destroyed_(false),
      stop_(0)
{
        U_ASSERT(ss != nullptr, "SocketServer is nullptr");
        ss_->SetMessageQueue(this);
        if (do_init) {
                DoInit();
        }
}

Thread::Thread(std::unique_ptr<SocketServer> ss, bool do_init)
    : Thread(ss.get(), do_init)
{
        own_ss_ = std::move(ss);
}

Thread::~Thread()
{
        Stop();
        DoDestroy();
}

std::unique_ptr<Thread>
Thread::CreateWithSocketServer()
{
        return std::unique_ptr<Thread>();
}

std::unique_ptr<Thread>
Thread::Create()
{
        return std::unique_ptr<Thread>(new Thread(
                std::unique_ptr<SocketServer>(new NullSocketServer())));
}

bool
Thread::ProcessMessages(int cms)
{
        U_CHECK(cms == kForever || cms == 0);
        int64_t ms_end = (kForever == cms) ? 0 : TimeAfter(cms);

        int cms_next = cms;
        while (true) {
                std::function<void()> task = Get(cms_next);
                if (!task) {
                        U_INFO("Get() returned nullptr");
                        return !IsQuitting();
                }
                Dispatch(std::move(task));

                if (cms != kForever) {
                        cms_next = static_cast<int>(TimeUntil(ms_end));
                        if (cms_next < 0) {
                                return true;
                        }
                }
        }

        return false;
}

bool
Thread::IsCurrent() const
{
        return TaskQueueBase::IsCurrent();
}

void
Thread::Join()
{
        if (!IsRunning()) {
                return;
        }

        pthread_join(thread_, nullptr);
        thread_ = 0;
}

void
Thread::SetName(const std::string &name)
{
        name_ = name;
}

void
Thread::PostTaskImpl(std::function<void()> task,
                     const TaskQueueBase::PostTaskTraits &traits)
{
        if (IsQuitting()) {
                return;
        }

        {
                LockGuard<Mutex> lock(mutex_);
                messages_.push(std::move(task));
        }

        WakeUpSocketServer();
}

void
Thread::PostDelayedTaskImpl(std::function<void()> task,
                            int delay_ms,
                            const TaskQueueBase::PostDelayedTaskTraits &traits)
{
        if (IsQuitting()) {
                return;
        }

        int64_t run_time_ms = TimeAfter(delay_ms);
        {
                LockGuard<Mutex> lock(mutex_);
                delayed_messages_.push({.delay_ms = delay_ms,
                                        .run_time_ms = run_time_ms,
                                        .message_number = delayed_next_num_,
                                        .functor = std::move(task)});
                ++delayed_next_num_;
                U_ASSERT(delayed_next_num_ != 0, "delayed_next_num_ overflow");
        }
        WakeUpSocketServer();
}

bool
Thread::Start()
{
        if (IsRunning()) {
                return false;
        }

        pthread_attr_t attr;
        pthread_attr_init(&attr);

        int error_code = pthread_create(&thread_, &attr, &Thread::PreRun, this);
        if (error_code != 0) {
                U_ERROR("Unable to create pthread, error {}", error_code);
                return false;
        }
        U_CHECK(thread_);

        return true;
}

void
Thread::Stop()
{
        Thread::Quit();
        Join();
}

void
Thread::Delete()
{
        Stop();
        delete this;
}

void
Thread::Dispatch(std::function<void()> task)
{
        // U_ASSERT(Thread::Current() == this, "Not running on this thread");

        // start_time: 任务开始执行的时间
        // end_time: 任务执行结束的时间
        // diff: 任务执行的时间

        int64_t start_time = TimeMillis();
        std::move(task)();
        int64_t end_time = TimeMillis();
        int64_t diff = TimeDiff(end_time, start_time);

        if (diff >= dispatch_warning_ms_) {
                U_WARN("Dispatch took {} ms, task: {}", diff, name_);
                // 修正慢任务警告阈值
                dispatch_warning_ms_ = static_cast<int>(diff) + 1;
        }
}

bool
Thread::IsRunning()
{
        return thread_ != 0;
}

void *
Thread::PreRun(void *pv)
{
        Thread *thread = static_cast<Thread *>(pv);
        thread->task_queue_registration_ =
                std::make_unique<TaskQueueBase::CurrentTaskQueueSetter>(thread);
        thread->Run();
        return nullptr;
}

void
Thread::Run()
{
        ProcessMessages(kForever);
}

bool
Thread::IsQuitting()
{
        return stop_.load(std::memory_order_acquire) != 0;
}

void
Thread::Quit()
{
        return stop_.store(1, std::memory_order_release);
        WakeUpSocketServer();
}

void
Thread::WakeUpSocketServer()
{
        ss_->WakeUp();
}

std::function<void()>
Thread::Get(int cms_wait)
{
        int64_t cms_total = cms_wait;
        int64_t cms_elapsed = 0;
        // 当前系统时间
        int64_t ms_start = TimeMillis();
        int64_t ms_current = ms_start;

        while (true) {
                // 计算下一次事件来临的时间
                int64_t cms_delay_next = kForever;
                {
                        LockGuard<Mutex> lock(mutex_);
                        cms_delay_next = ProcessDelayedMessage(ms_current);

                        if (!messages_.empty()) {
                                std::function<void()> task =
                                        std::move(messages_.front());
                                messages_.pop();
                                return task;
                        }
                }

                if (IsQuitting()) {
                        break;
                }

                int64_t cms_next;
                if (cms_wait == kForever) {
                        cms_next = cms_delay_next;
                } else {
                        cms_next =
                                std::max<int64_t>(0, cms_total - cms_elapsed);
                        // 还有其他延迟执行的消息
                        if ((cms_delay_next != kForever)
                            && (cms_delay_next < cms_next)) {
                                cms_next = cms_delay_next;
                        }
                }

                if (!ss_->Wait(cms_next == kForever ? SocketServer::kForever
                                                    : cms_next,
                               /* process_io= */ true)) {
                        return nullptr;
                }

                ms_current = TimeMillis();
                cms_elapsed = TimeDiff(ms_current, ms_start);

                // 如果有超时设定，并且超时了
                if (cms_wait != kForever && cms_elapsed >= cms_total) {
                        return nullptr;
                }
        }

        return nullptr;
}

int64_t
Thread::ProcessDelayedMessage(int64_t cms_now)
{
        while (!delayed_messages_.empty()) {
                if (cms_now < delayed_messages_.top().run_time_ms) {
                        return TimeDiff(delayed_messages_.top().run_time_ms,
                                        cms_now);
                }

                messages_.push(std::move(delayed_messages_.top().functor));
                delayed_messages_.pop();
        }

        return kForever;
}

void
Thread::Restart()
{
        stop_.store(0, std::memory_order_release);
}

void
Thread::DoDestroy()
{
        if (ss_) {
                ss_->SetMessageQueue(nullptr);
        }
        messages_ = {};
        delayed_messages_ = {};
}

void
Thread::DoInit()
{
        if (initialized_) {
                return;
        }

        initialized_ = true;
}

}// namespace tqcq
