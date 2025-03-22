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

// don't use this anywhere else
inline constexpr auto __bitsetN = 64;

// why: i found that 64 bits is enough for my use case
struct bitset : private std::bitset<__bitsetN> {
private:
    using Base = std::bitset<__bitsetN>;

    static auto m_check(std::size_t length) -> void {
        assume(length <= __bitsetN, "Length must be less than or equal to 64");
    }

    // private constructor for internal use
    explicit bitset(const Base &b, std::size_t n) : Base(b), m_length(n) {}

    friend struct iterator;

    struct iterator {
    public:
        friend auto operator==(const iterator &lhs, const iterator &rhs) -> bool {
            return lhs.m_index == rhs.m_index;
        }
        auto operator++() -> iterator & {
            m_index = m_bitset._Find_next(m_index);
            return *this;
        }
        auto operator++(int) -> iterator {
            auto result = *this;
            ++(*this);
            return result;
        }
        auto operator*() const -> std::size_t {
            return m_index;
        }

    private:
        friend struct bitset;
        iterator(const bitset &b, std::size_t i) : m_bitset(b), m_index(i) {}
        const bitset &m_bitset;
        std::size_t m_index;
    };

    auto as_bitset() const -> const Base & {
        return static_cast<const Base &>(*this);
    }

public:
    using Base::reference;

    explicit bitset() = default;
    explicit bitset(std::size_t n) noexcept : Base(0), m_length(n) {
        m_check(n);
    }

    bitset(const bitset &)                     = default;
    auto operator=(const bitset &) -> bitset & = default;

    auto set_at(std::size_t n, std::size_t shift, const bitset &rhs) -> void {
        m_length = n;
        assume(n >= rhs.m_length + shift);
        static_cast<Base &>(*this) = (rhs << shift);
    }

    auto expand(std::size_t n) const -> bitset {
        m_check(n);
        assume(n >= m_length);
        auto result = bitset{*this, n};
        return result;
    }

    auto subset(std::size_t n) const -> bitset {
        assume(n <= m_length);
        auto result = bitset{*this, n};
        result &= Base((1ULL << n) - 1);
        return result;
    }

    auto size() const -> std::size_t {
        return m_length;
    }

    using Base::reset;

    auto set_all() -> void {
        m_check(m_length);
        for (std::size_t i = 0; i < m_length; ++i)
            Base::operator[](i) = true;
    }

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
    auto hash() const -> std::uint64_t {
        static_assert(sizeof(Base) == sizeof(std::uint64_t));
        return Base::to_ullong();
    }

    auto begin() const -> iterator {
        return iterator{*this, this->_Find_first()};
    }

    auto end() const -> iterator {
        return iterator{*this, __bitsetN};
    }

    friend auto operator&(const bitset &lhs, const bitset &rhs) -> bitset {
        assume(lhs.m_length == rhs.m_length);
        return bitset{lhs.as_bitset() & rhs.as_bitset(), lhs.m_length};
    }

    friend auto operator==(const bitset &lhs, const bitset &rhs) -> bool {
        assume(lhs.m_length == rhs.m_length);
        return lhs.as_bitset() == rhs.as_bitset();
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
