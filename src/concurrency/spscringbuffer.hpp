#pragma once

#include <atomic>
#include <new>
#include <cstring>
#include <array>

namespace netbook::concurrency {

// The effective size of the buffer is N-1.
template<typename T, std::size_t N>
class SPSCRingBuffer {
    // By enforcing this we don't have to do any integer division which is faster.
    static_assert(N > 0 && (N & (N - 1)) == 0, "N must be a power of 2");

    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> head{0};
    alignas(std::hardware_destructive_interference_size) std::atomic<size_t> tail{0};
    std::array<T, N> buffer;

public:
    // Returns true if successful.
    bool pop(T& item) noexcept {
        const std::size_t current_tail = tail.load(std::memory_order_relaxed);
        const std::size_t current_head = head.load(std::memory_order_acquire);

        if (current_tail == current_head) {
            return false;
        }

        item = buffer[current_tail];
        tail.store((current_tail + 1) & (N - 1), std::memory_order_release);

        return true;
    }

    // Returns the number of items popped. maximum must be less than the size of
    // the buffer or the behaviour is undefined.
    unsigned int pop_many(T* items, const unsigned int maximum) noexcept {
        static_assert(std::is_trivially_copyable_v<T>, "pop_many requires trivially copyable T");

        const std::size_t current_tail = tail.load(std::memory_order_relaxed);
        const std::size_t current_head = head.load(std::memory_order_acquire);
        std::size_t number_to_pop;

        if (current_tail < current_head) {
            number_to_pop = current_head - current_tail;
        } else if (current_tail > current_head) {
            number_to_pop = (N - current_tail) + current_head;
        } else {
            // current_tail == current_head.
            return 0;
        }

        if (number_to_pop > maximum) {
            number_to_pop = maximum;
        }

        // Before we wrap this back to the start of the queue.
        const std::size_t next_tail_raw = current_tail + number_to_pop;

        if (next_tail_raw < N) {
            // No wrap-around, we can just write everything in one go.
            std::memcpy(items, &buffer[current_tail], number_to_pop * sizeof(T));
        } else {
            // We have to do two copies.
            const std::size_t first_copy_size = N - current_tail;
            const std::size_t second_copy_size = number_to_pop - first_copy_size;

            std::memcpy(items, &buffer[current_tail], first_copy_size * sizeof(T));
            std::memcpy(&items[first_copy_size], &buffer[0], second_copy_size * sizeof(T));
        }

        tail.store(next_tail_raw & (N - 1), std::memory_order_release);

        return number_to_pop;
    }

    // Returns true if successful.
    bool push(const T item) noexcept {
        const std::size_t current_tail = tail.load(std::memory_order_acquire);
        const std::size_t current_head = head.load(std::memory_order_relaxed);
        const std::size_t next_head = (current_head + 1) & (N - 1);

        if (next_head == current_tail) {
            return false;
        }

        buffer[current_head] = item;
        head.store(next_head, std::memory_order_release);

        return true;
    }
};

}