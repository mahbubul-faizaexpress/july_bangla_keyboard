#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <type_traits>

namespace july {

// Fixed-capacity ring buffer. Pushing into a full buffer overwrites the oldest element.
// No heap allocation; intended for per-thread engine state only (not thread-safe).
template <typename T, std::size_t Capacity>
class RingBuffer {
    static_assert(Capacity > 0);
    static_assert(std::is_trivially_copyable_v<T>);

public:
    constexpr void push(const T& value) noexcept {
        items_[(head_ + size_) % Capacity] = value;
        if (size_ < Capacity) {
            ++size_;
        } else {
            head_ = (head_ + 1) % Capacity;
        }
    }

    // Removes and returns the newest element.
    constexpr std::optional<T> popBack() noexcept {
        if (size_ == 0) return std::nullopt;
        --size_;
        return items_[(head_ + size_) % Capacity];
    }

    // index 0 = oldest element.
    [[nodiscard]] constexpr const T& operator[](std::size_t index) const noexcept {
        return items_[(head_ + index) % Capacity];
    }

    [[nodiscard]] constexpr const T* back() const noexcept {
        return size_ == 0 ? nullptr : &items_[(head_ + size_ - 1) % Capacity];
    }

    constexpr void clear() noexcept { head_ = 0; size_ = 0; }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    std::array<T, Capacity> items_{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

} // namespace july
