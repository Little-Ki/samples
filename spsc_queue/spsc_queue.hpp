#pragma once

#include <memory>
#include <vector>
#include <atomic>

template <typename T, size_t size = 1024>
class SpscQueue
{

public:
    SpscQueue()
    {
        m_slot.resize(m_size);
    }

    SpscQueue(const SpscQueue &_) = delete;

    bool empty()
    {
        return m_head.load(std::memory_order_acquire) ==
               m_tail.load(std::memory_order_acquire);
    }

    void push(const T &val)
    {
        static_assert(std::is_nothrow_copy_constructible<T>::value, "T must be nothrow destructible");
        auto const _tail = m_tail.load(std::memory_order_relaxed);
        auto _next = (_tail + 1) % m_size;
        while (_next == m_head.load(std::memory_order_acquire))
            ;
        m_slot[_tail].value = val;
        m_tail.store(_next, std::memory_order_release);
    }

    void pop()
    {
        auto const _head = m_head.load(std::memory_order_relaxed);
        auto _next = (_head + 1) % m_size;
        if (_head != m_tail.load(std::memory_order_acquire))
        {
            m_head.store(_next, std::memory_order_release);
        }
    }

    T *head()
    {
        auto const _head = m_head.load(std::memory_order_relaxed);
        if (_head == m_tail.load(std::memory_order_acquire))
        {
            return nullptr;
        }
        return &m_slot[_head].value;
    }

private:
#ifdef __cpp_lib_hardware_interference_size
    static constexpr size_t __chache_line_size =
        std::hardware_destructive_interference_size;
#else
    static constexpr size_t __chache_line_size = 64;
#endif

    static constexpr size_t m_size = std::max(2ULL, size);

    struct alignas(__chache_line_size) _T
    {
        T value;
    };

    alignas(__chache_line_size) std::vector<_T> m_slot;
    alignas(__chache_line_size) std::atomic<size_t> m_head{0};
    alignas(__chache_line_size) std::atomic<size_t> m_tail{0};
};