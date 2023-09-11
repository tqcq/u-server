//
// Created by tqcq on 2023/9/3.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_API_TASK_QUEUE_BASE_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_API_TASK_QUEUE_BASE_H_

#include "u-toolbox/base/non_copyable.h"
#include <functional>

namespace tqcq {
class TaskQueueBase {
public:
        enum class DelayPrecision {
                kLow,
                kHigh,
        };

        virtual void Delete() = 0;

        void PostTask(std::function<void()> task)
        {
                PostTaskImpl(std::move(task), PostTaskTraits{});
        }

        void PostDelayedTask(std::function<void()> task, int delay_ms)
        {
                struct PostDelayedTaskTraits traits;
                traits.high_precision = false;

                PostDelayedTaskImpl(std::move(task), delay_ms, traits);
        }

        void PostDelayedHighPrecisionTask(std::function<void()> task,
                                          int delay_ms)

        {
                struct PostDelayedTaskTraits traits;
                traits.high_precision = true;
                PostDelayedTaskImpl(std::move(task), delay_ms, traits);
        }

        void PostDelayedTaskWithPrecision(DelayPrecision precision,
                                          std::function<void()> task,
                                          int delay_ms)
        {
                switch (precision) {
                case DelayPrecision::kLow:
                        PostDelayedTask(std::move(task), delay_ms);
                        break;
                case DelayPrecision::kHigh:
                        PostDelayedHighPrecisionTask(std::move(task), delay_ms);
                        break;
                }
        }

        static TaskQueueBase *Current();

        bool IsCurrent() const { return Current() == this; }

protected:
        struct PostTaskTraits {};

        struct PostDelayedTaskTraits {
                bool high_precision = false;
        };

        virtual void PostTaskImpl(std::function<void()> task,
                                  const PostTaskTraits &traits) = 0;
        virtual void
        PostDelayedTaskImpl(std::function<void()> task, int delay_ms,
                            const PostDelayedTaskTraits &traits) = 0;
        virtual ~TaskQueueBase() = default;

        class CurrentTaskQueueSetter : NonCopyable {
        public:
                explicit CurrentTaskQueueSetter(TaskQueueBase *task_queue);
                ~CurrentTaskQueueSetter();

        private:
                TaskQueueBase *const previous_;
        };
};

struct TaskQueueDeleter {
        void operator()(TaskQueueBase *task_queue) const
        {
                task_queue->Delete();
        }
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_API_TASK_QUEUE_BASE_H_
