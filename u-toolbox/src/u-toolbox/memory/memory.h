//
// Created by tqcq on 2023/9/5.
//

#ifndef HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_MEMORY_MEMORY_H_
#define HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_MEMORY_MEMORY_H_

#include "u-toolbox/concurrency/lock_guard.h"
#include "u-toolbox/concurrency/mutex.h"
#include "u-toolbox/concurrency/thread_annotations.h"
#include "u-toolbox/log/u_log.h"
#include <array>
#include <stdint.h>

namespace tqcq {
template<typename T>
struct StlAllocator;

size_t PageSize();

template<typename T>
inline T
AlignUp(T val, T alignment)
{
        static_assert(std::is_integral<T>::value, "T must be integral type");
        return alignment * ((val + alignment - 1) / alignment);
}

template<size_t SIZE, size_t ALIGNMENT>
struct AlignedStorage {
        // 必须时2的幂次方，否则会失效
        static_assert((ALIGNMENT & (ALIGNMENT - 1)) == 0,
                      "ALIGNMENT must be power of 2");

        struct alignas(ALIGNMENT) Type {
                uint8_t data[SIZE];
        };
};

// 封装内存块，方便操作
struct Allocation {
        enum class Usage : uint8_t {
                Undefined = 0,
                Stack,
                Create,// 创建堆数据
                Vector,
                List,
                Stl,
                Count,// Usage的类型数量,申请数组时可用
                      // Max = Count,
        };

        struct Request {
                size_t size;            // 要申请的字节数
                size_t alignment;       // 最小对齐
                bool use_guards = false;// 是否启用申请保护
                Usage usage = Usage::Undefined;
        };

        void *ptr = nullptr;// 指向已经申请的内存
        Request request;
};

class Allocator {
public:
        static Allocator *Default;

        struct Deleter {
                inline Deleter();
                inline Deleter(Allocator *allocator, size_t count);

                template<typename T>
                inline void operator()(T *object);

                Allocator *allocator = nullptr;
                size_t count = 0;
        };

        template<typename T>
        using UniquePtr = std::unique_ptr<T, Deleter>;

        virtual ~Allocator() = default;

        /**
         * @brief Create -> Allocate
         * @return
         */
        virtual Allocation Allocate(const Allocation::Request &) = 0;

        /**
         * @brief Destory -> Free
         */
        virtual void Free(const Allocation &) = 0;

        template<typename T, typename... Args>
        inline T *Create(Args &&...args);

        template<typename T>
        inline void Destroy(T *object);

        template<typename T, typename... Args>
        inline UniquePtr<T> MakeUnique(Args &&...args);

        template<typename T, typename... Args>
        inline UniquePtr<T> MakeUniqueN(size_t count, Args &&...args);

        template<typename T, typename... Args>
        inline std::shared_ptr<T> MakeShared(Args &&...args);

protected:
        Allocator() = default;
};

Allocator::Deleter::Deleter() : allocator(nullptr) {}

Allocator::Deleter::Deleter(tqcq::Allocator *allocator_, size_t count_)
    : allocator(allocator_),
      count(count_)
{}

template<typename T>
inline void
Allocator::Deleter::operator()(T *object)
{
        object->~T();

        Allocation allocation;
        allocation.ptr = object;
        allocation.request.size = sizeof(T) * count;
        allocation.request.alignment = alignof(T);
        allocation.request.usage = Allocation::Usage::Create;
        allocator->Free(allocation);
}

template<typename T, typename... Args>
inline T *
Allocator::Create(Args &&...args)
{
        Allocation::Request request;
        request.size = sizeof(T);
        request.alignment = alignof(T);
        request.usage = Allocation::Usage::Create;

        auto alloc = Allocate(request);
        new (alloc.ptr) T(std::forward<Args>(args)...);
        return reinterpret_cast<T *>(alloc.ptr);
}

// same Deleter
template<typename T>
inline void
Allocator::Destroy(T *object)
{
        object->~T();

        Allocation alloc;
        alloc.ptr = object;
        alloc.request.size = sizeof(T);
        alloc.request.alignment = alignof(T);
        alloc.request.usage = Allocation::Usage::Create;
        Free(alloc);
}

template<typename T, typename... Args>
Allocator::UniquePtr<T>
Allocator::MakeUnique(Args &&...args)
{
        return MakeUniqueN<T>(1, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
Allocator::UniquePtr<T>
Allocator::MakeUniqueN(size_t count, Args &&...args)
{
        if (count == 0) {
                return nullptr;
        }

        Allocation::Request request;
        request.size = sizeof(T) * count;
        request.alignment = alignof(T);
        request.usage = Allocation::Usage::Create;

        auto alloc = Allocate(request);
        new (alloc.ptr) T(std::forward<Args>(args)...);
        return UniquePtr<T>(reinterpret_cast<T *>(alloc.ptr),
                            Deleter{this, count});
}

template<typename T, typename... Args>
std::shared_ptr<T>
Allocator::MakeShared(Args &&...args)
{
        Allocation::Request request;
        request.size = sizeof(T);
        request.alignment = alignof(T);
        request.usage = Allocation::Usage::Create;

        auto alloc = Allocate(request);
        new (alloc.ptr) T(std::forward<Args>(args)...);
        return std::shared_ptr<T>(reinterpret_cast<T *>(alloc.ptr),
                                  Deleter{this, 1});
}

// 包装Allocator，提供记录功能
// 统计 Usage {Stack, Create, Vector, List, Stl} 各种类型的申请次数和申请的字节数
class TrackedAllocator : public Allocator {
public:
        struct UsageStats {
                size_t count = 0;
                size_t bytes = 0;
        };

        struct Stats {
                inline size_t NumAllocations() const;
                inline size_t BytesAllocated() const;
                std::array<UsageStats, size_t(Allocation::Usage::Count)>
                        by_usage;
        };

        inline TrackedAllocator(Allocator *allocator);
        inline Stats stats();
        inline Allocation Allocate(const Allocation::Request &) override;
        inline void Free(const Allocation &) override;

private:
        Allocator *const allocator_;
        Stats stats_ U_GUARDED_BY(mutex_);
        Mutex mutex_;
};

size_t
TrackedAllocator::Stats::NumAllocations() const
{
        size_t out = 0;
        for (auto &stats : by_usage) { out += stats.count; }
        return out;
}

size_t
TrackedAllocator::Stats::BytesAllocated() const
{
        size_t out = 0;
        for (auto &stats : by_usage) { out += stats.bytes; }
        return out;
}

TrackedAllocator::TrackedAllocator(tqcq::Allocator *allocator)
    : allocator_(allocator)
{}

TrackedAllocator::Stats
TrackedAllocator::stats()
{
        LockGuard<Mutex> lock(mutex_);
        return stats_;
}

Allocation
TrackedAllocator::Allocate(const Allocation::Request &request)
{
        {
                LockGuard<Mutex> lock(mutex_);
                auto &usage_stats = stats_.by_usage[int(request.usage)];
                ++usage_stats.count;
                usage_stats.bytes += request.size;
        }
        return allocator_->Allocate(request);
}

void
TrackedAllocator::Free(const Allocation &allocation)
{
        LockGuard<Mutex> lock(mutex_);
        auto &usage_stats = stats_.by_usage[int(allocation.request.usage)];
        U_ASSERT(usage_stats.count > 0,
                 "TrackedAllocator detected abnormal free()");
        U_ASSERT(usage_stats.bytes >= allocation.request.size,
                 "TrackedAllocator detected abnormal free()");
        --usage_stats.count;
        usage_stats.bytes -= allocation.request.size;
        return allocator_->Free(allocation);
}

template<typename T>
struct StlAllocator {
        using value_type = T;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using size_type = size_t;
        using difference_type = size_t;

        template<typename U>
        struct rebind {
                typedef StlAllocator<U> other;
        };

        inline StlAllocator(Allocator *allocator);

        template<typename U>
        inline StlAllocator(const StlAllocator<U> &other);

        inline pointer address(reference x) const;
        inline const_pointer address(const_reference x) const;

        inline T *allocate(std::size_t n);
        inline void deallocate(T *p, std::size_t n);

        inline size_type max_size() const;

        inline void construct(pointer p, const_reference val);
        template<typename U, typename... Args>
        inline void construct(U *p, Args &&...args);

        inline void destroy(pointer p);
        template<typename U>
        inline void destroy(U *p);

private:
        // 申请 n 个 T 类型的内存
        inline Allocation::Request BuildRequest(size_t n) const;
        template<typename U>
        friend struct StlAllocator;
        Allocator *allocator_;
};

template<typename T>
StlAllocator<T>::StlAllocator(Allocator *allocator) : allocator_(allocator)
{}

template<typename T>
template<typename U>
StlAllocator<T>::StlAllocator(const StlAllocator<U> &other)
{
        allocator_ = other.allocator_;
}

template<typename T>
typename StlAllocator<T>::pointer
StlAllocator<T>::address(reference x) const
{
        return &x;
}

template<typename T>
typename StlAllocator<T>::const_pointer
StlAllocator<T>::address(const_reference x) const
{
        return &x;
}

template<typename T>
T *
StlAllocator<T>::allocate(std::size_t n)
{
        auto alloc = allocator_->Allocate(BuildRequest(n));
        return reinterpret_cast<T *>(alloc.ptr);
}

template<typename T>
void
StlAllocator<T>::deallocate(T *p, std::size_t n)
{
        Allocation alloc;
        alloc.ptr = p;
        alloc.request = BuildRequest(n);
        allocator_->Free(alloc);
}

template<typename T>
typename StlAllocator<T>::size_type
StlAllocator<T>::max_size() const
{
        // 申请的内存不能超过size_type的最大值
        return std::numeric_limits<size_type>::max() / sizeof(T);
}

template<typename T>
void
StlAllocator<T>::construct(pointer p, const_reference val)
{
        new (p) T(val);
}

template<typename T>
template<typename U, typename... Args>
void
StlAllocator<T>::construct(U *p, Args &&...args)
{
        ::new ((void *) p) U(std::forward<Args>(args)...);
}

template<typename T>
template<typename U>
void
StlAllocator<T>::destroy(U *p)
{
        p->~U();
}

template<typename T>
Allocation::Request
StlAllocator<T>::BuildRequest(size_t n) const
{
        Allocation::Request request;
        request.size = sizeof(T) * n;
        request.alignment = alignof(T);
        request.usage = Allocation::Usage::Stl;
        return request;
}

}// namespace tqcq

#endif//HTTP_SERVER_U_TOOLBOX_SRC_U_TOOLBOX_MEMORY_MEMORY_H_
