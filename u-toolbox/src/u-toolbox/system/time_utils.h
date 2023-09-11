//
// Created by tqcq on 2023/9/6.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_SYSTEM_TIME_UTILS_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_SYSTEM_TIME_UTILS_H_

#include <chrono>
#include <stdint.h>

namespace tqcq {

int64_t SystemTimeNanos();

int64_t SystemTimeMillis();

int64_t TimeMillis();

int64_t TimeMicros();

int64_t TimeNanos();

/*
 * @brief 返回当前时间加上 elapsed 的时间，单位毫秒
 */
int64_t TimeAfter(int64_t elapsed);

int64_t TimeDiff(int64_t later, int64_t earlier);

inline int64_t
TimeSince(int64_t earlier)
{
        return TimeMillis() - earlier;
}

inline int64_t
TimeUntil(int64_t later)
{
        return later - TimeMillis();
}
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_SYSTEM_TIME_UTILS_H_
