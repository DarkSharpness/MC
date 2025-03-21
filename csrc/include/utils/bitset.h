#pragma once
#include "error.h"
#include <bitset>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string>
#include <vector>

namespace dark {

struct dynamic_bitset {
public:
    using word_t = std::uint64_t;

    inline static constexpr auto kMask = ~static_cast<word_t>(0);

    dynamic_bitset() = default;
    dynamic_bitset(std::size_t length) : m_length(length), m_data(s_required(length), 0) {}
    dynamic_bitset(std::size_t length, bool value) :
        m_length(length), m_data(s_required(length), value ? kMask : 0) {}

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
        assume(i < m_length);
        const auto [div, mod] = s_split(i);
        return (m_data[div] & (1ULL << mod)) != 0;
    }

    auto set(std::size_t i, bool value = true) -> void {
        assume(i < m_length);
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

    auto to_string() const -> std::string {
        std::string result;
        for (std::size_t i = 0; i < m_length; ++i)
            result.push_back(test(i) ? '1' : '0');
        return result;
    }

    // using default operator== for comparison
    friend auto operator==(const dynamic_bitset &lhs, const dynamic_bitset &rhs) -> bool = default;

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
    std::size_t m_length;
    std::vector<word_t> m_data;
};

// why: i found that 64 bits is enough for my use case
struct bitset : private std::bitset<64> {
private:
    using Base = std::bitset<64>;

public:
    using Base::reference;

    bitset() = default;
    bitset(std::size_t length) : Base(0), m_length(length) {
        assume(length <= 64);
    }

    bitset(const bitset &)                     = default;
    auto operator=(const bitset &) -> bitset & = default;

    auto resize(std::size_t length) -> void {
        assume(length <= 64);
        m_length = length;
    }

    auto size() const -> std::size_t {
        return m_length;
    }

    using Base::reset;

    auto operator[](std::size_t i) const -> bool {
        assume(i < m_length, "Subscript out of range");
        return Base::operator[](i);
    }

    auto operator[](std::size_t i) -> reference {
        assume(i < m_length, "Subscript out of range");
        return Base::operator[](i);
    }

    auto to_string() const -> std::string {
        auto result = std::string(m_length, '0');
        for (std::size_t i = 0; i < m_length; ++i)
            if (test(i))
                result[i] = '1';
        return result;
    }

    // using default operator== for comparison
    friend auto operator==(const bitset &lhs, const bitset &rhs) -> bool = default;

    auto hash() const -> std::uint64_t {
        static_assert(sizeof(Base) == sizeof(std::uint64_t));
        return Base::to_ullong();
    }

private:
    std::size_t m_length;
};

} // namespace dark

namespace std {

template <>
struct hash<dark::bitset> {
    auto operator()(const dark::bitset &bitset) const -> std::size_t {
        return std::hash<std::uint64_t>{}(bitset.hash());
    }
};

} // namespace std
