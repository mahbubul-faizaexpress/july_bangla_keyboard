#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace july {

// Fixed-capacity UTF-16 buffer for hot-path text. Never allocates; appends that would
// overflow are rejected as a whole and report false.
template <std::size_t Capacity>
class TextBuffer {
public:
    constexpr bool append(std::u16string_view text) noexcept {
        if (text.size() > Capacity - size_) return false;
        for (char16_t c : text) data_[size_++] = c;
        return true;
    }

    constexpr bool push(char16_t c) noexcept {
        if (size_ == Capacity) return false;
        data_[size_++] = c;
        return true;
    }

    constexpr void clear() noexcept { size_ = 0; }

    [[nodiscard]] constexpr std::u16string_view view() const noexcept { return {data_.data(), size_}; }
    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    std::array<char16_t, Capacity> data_{};
    std::size_t size_ = 0;
};

} // namespace july
