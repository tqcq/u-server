//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_DEFER_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_DEFER_H_

#include "u-toolbox/base/finally.h"

namespace tqcq {
#define U_DEFER_CONCAT_IMPL(x, y) x##y
#define U_DEFER_CONCAT(x, y) U_DEFER_CONCAT_IMPL(x, y)

#define defer(x)                                                               \
        auto U_DEFER_CONCAT(defer, __LINE__) = tqcq::MakeFinally([&]() { x; })
}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_DEFER_H_
