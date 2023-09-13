//
// Created by tqcq on 2023/9/3.
//

#ifndef HTTP_SERVER_U_TOOLBOX_LOG_U_LOG_H_
#define HTTP_SERVER_U_TOOLBOX_LOG_U_LOG_H_

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

namespace tqcq {
class ULog {
#define U_TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define U_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define U_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define U_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define U_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define U_FATAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

#define U_CHECK(condition)                                                     \
        (condition) ? static_cast<void>(0)                                     \
                    : U_FATAL("Check failed: " #condition ". ")

#define U_ASSERT(condition, error_msg, ...)                                    \
        U_ASSERT_IMPL(condition, error_msg, ##__VA_ARGS__)

#define U_ASSERT_IMPL(condition, error_msg, ...)                               \
        do {                                                                   \
                if (!(condition)) {                                            \
                        U_FATAL("assert failed. [" #condition "] {}",          \
                                error_msg, ##__VA_ARGS__);                     \
                }                                                              \
        } while (0)
};

class InitLogFormat {
public:
        InitLogFormat()
        {
                spdlog::set_pattern("%Y-%m-%d %H:%M:%S.%e %t %l: %v");
        }
};

static InitLogFormat init_log_format;
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_LOG_U_LOG_H_
