//
// Created by tqcq on 2023/9/3.
//

#ifndef HTTP_SERVER_UTOOLBOX_BASE_NON_COPYABLE_H_
#define HTTP_SERVER_UTOOLBOX_BASE_NON_COPYABLE_H_

namespace tqcq {
class NonCopyable {

public:
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;

protected:
        NonCopyable() = default;
        ~NonCopyable() = default;
};

#define SET_NONOCOPYABLE(CLASS_NAME)                                           \
        CLASS_NAME(const CLASS_NAME &) = delete;                               \
        CLASS_NAME &operator=(const CLASS_NAME &) = delete;
}// namespace tqcq

#endif//HTTP_SERVER_UTOOLBOX_BASE_NON_COPYABLE_H_
