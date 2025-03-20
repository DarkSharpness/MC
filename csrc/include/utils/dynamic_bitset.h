#pragma once
#include "error.h"
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <vector>

namespace dark {

struct dynamic_bitset {
public:
    using word_t = std::uint64_t;

    inline static constexpr auto kMask = ~static_cast<word_t>(0);

    dynamic_bitset() = default;
    dynamic_bitset(std::size_t length) : m_data(s_required(length), 0), m_length(length) {}
    dynamic_bitset(std::size_t length, bool value) :
        m_data(s_required(length), value ? kMask : 0), m_length(length) {}

    dynamic_bitset(const dynamic_bitset &)                     = default;
    dynamic_bitset(dynamic_bitset &&)                          = default;
    auto operator=(const dynamic_bitset &) -> dynamic_bitset & = default;
    auto operator=(dynamic_bitset &&) -> dynamic_bitset &      = default;

    template <std::ranges::sized_range _Range>
        requires std::unsigned_integral<std::ranges::range_value_t<_Range>>
    auto set_indices(const _Range &indices) -> void {
        for (const auto i : indices)
            set(i, true);
    }

    auto resize(std::size_t length) -> void {
        m_data.resize(s_required(length));
        m_length = length;
    }

    auto size() const -> std::size_t {
        return m_length;
    }

    auto test(std::size_t i) const -> bool {
        const auto [div, mod] = s_split(i);
        return (m_data[div] & (1ULL << mod)) != 0;
    }

    auto set(std::size_t i, bool value = true) -> void {
        const auto [di, mod] = s_split(i);
        if (value) {
            m_data[di] |= (1ULL << mod);
        } else {
            m_data[di] &= ~(1ULL << mod);
        }
    }

    auto reset() -> void {
        for (auto &d : m_data)
            d = 0;
    }

    auto operator[](std::size_t i) const -> bool {
        return test(i);
    }

    auto operator|=(const dynamic_bitset &rhs) -> dynamic_bitset & {
        assume(m_length == rhs.m_length);
        for (std::size_t i = 0; i < m_data.size(); ++i)
            m_data[i] |= rhs.m_data[i];
        return *this;
    }

    auto operator&=(const dynamic_bitset &rhs) -> dynamic_bitset & {
        assume(m_length == rhs.m_length);
        for (std::size_t i = 0; i < m_data.size(); ++i)
            m_data[i] &= rhs.m_data[i];
        return *this;
    }

    auto operator^=(const dynamic_bitset &rhs) -> dynamic_bitset & {
        assume(m_length == rhs.m_length);
        for (std::size_t i = 0; i < m_data.size(); ++i)
            m_data[i] ^= rhs.m_data[i];
        return *this;
    }

private:
    struct Pair {
        std::size_t div;
        std::size_t mod;
    };
    static auto s_required(std::size_t n) -> std::size_t {
        return (n + 63) / 64;
    }
    static auto s_split(std::size_t n) -> Pair {
        return Pair{n / 64, n % 64};
    }
    std::vector<word_t> m_data;
    std::size_t m_length;
};

} // namespace dark