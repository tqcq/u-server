//
// Created by tqcq on 2023/9/6.
//

#include "time_utils.h"

namespace tqcq {
int64_t
SystemTimeNanos()
{
        int64_t ticks;
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        ticks = 1000000000LL * ts.tv_sec + ts.tv_nsec;
        return ticks;
}

int64_t
SystemTimeMillis()
{
        return SystemTimeNanos() / 1000000;
}

int64_t
TimeMillis()
{
        return TimeNanos() / 1000000LL;
}

int64_t
TimeMicros()
{
        return TimeNanos() / 1000LL;
}

int64_t
TimeNanos()
{
        return SystemTimeNanos();
}

int64_t
TimeAfter(int64_t elapsed)
{
        return TimeMillis() + elapsed;
}

int64_t
TimeDiff(int64_t later, int64_t earlier)
{
        return later - earlier;
}
}// namespace tqcq
