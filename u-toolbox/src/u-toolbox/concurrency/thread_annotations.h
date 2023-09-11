//
// Created by tqcq on 2023/9/3.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_THREAD_ANNOTATIONS_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_THREAD_ANNOTATIONS_H_

namespace tqcq {
#if defined(__clang__) && (!defined(SWIG))
#define U_THREAD_ANNOTATION_ATTRIBUTE__(x) __attribute__((x))
#else
#define U_THREAD_ANNOTATION_ATTRIBUTE__(x)// no-op
#endif

#define U_GUARDED_BY(x) U_THREAD_ANNOTATION_ATTRIBUTE__(guarded_by(x))
#define U_EXCLUSIVE_LOCKS_REQUIRED(...)                                        \
        U_THREAD_ANNOTATION_ATTRIBUTE__(exclusive_locks_required(__VA_ARGS__))
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_THREAD_ANNOTATIONS_H_
