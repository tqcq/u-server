//
// Created by tqcq on 2023/9/4.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_FINALLY_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_FINALLY_H_

#include <memory>

namespace tqcq {
class Finally {
public:
        virtual ~Finally() = default;
};

template<typename F>
class FinallyImpl : public Finally {
public:
        inline FinallyImpl(const F &func) : func_(func) {}

        inline FinallyImpl(F &&func) : func_(std::move(func)) {}

        inline FinallyImpl(FinallyImpl<F> &&other)
            : func_(std::move(other.func_))
        {
                other.valid = false;
        }

        inline ~FinallyImpl()
        {
                if (valid) {
                        func_();
                }
        }

private:
        // const 对象不能进行释放操作
        FinallyImpl(const FinallyImpl<F> &other) = delete;
        // 禁止移动赋值函数，避免意外的改变生命周期
        FinallyImpl<F> &operator=(const FinallyImpl<F> &other) = delete;
        FinallyImpl<F> &operator=(FinallyImpl<F> &&other) = delete;
        F func_;
        bool valid = true;
};

template<typename F>
inline FinallyImpl<F>
MakeFinally(F &&f)
{
        return FinallyImpl<F>(std::forward<F>(f));
}

template<typename F>
inline std::shared_ptr<Finally>
MakeSharedFinally(F &&f)
{
        return std::make_shared<FinallyImpl<F>>(std::forward<F>(f));
}

}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_BASE_FINALLY_H_
