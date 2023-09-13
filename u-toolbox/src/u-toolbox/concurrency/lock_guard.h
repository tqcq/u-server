//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_

#include "mutex.h"
#include "u-toolbox/base/non_copyable.h"

namespace tqcq {

template<typename T>
class is_lockable {
        template<typename>
        static std::false_type TestLock(...);

        template<typename>
        static std::false_type TestUnlock(...);

        template<typename U>
        static auto TestLock(int)
                -> decltype(std::declval<U>().Lock(), std::true_type());

        template<typename U>
        static auto TestUnlock(int)
                -> decltype(std::declval<U>().UnLock(), std::true_type());

public:
        static constexpr bool value =
                std::is_same<decltype(TestLock<T>(0)),
                             decltype(std::true_type())>::value
                && std::is_same<decltype(TestUnlock<T>(0)),
                                decltype(std::true_type())>::value;
};

template<typename MutexType>
class LockGuard : NonCopyable {
public:
        // static_assert(is_lockable<MutexType>::value,
        //              "L must be a lockable type");

        inline explicit LockGuard(MutexType &mutex) : mutex_(mutex)
        {
                mutex_.Lock();
        }

        inline ~LockGuard() { mutex_.Unlock(); }

private:
        MutexType &mutex_;
};
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_CONCURRENCY_LOCK_GUARD_H_
