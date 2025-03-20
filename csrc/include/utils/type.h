#pragma once
#include <concepts>
#include <cstddef>

namespace dark {

template <std::integral _Int, typename... _Tags>
struct tagged_enum {
    enum type : _Int {};
};

template <std::integral _Int, typename... _Tags>
using tagged_int = typename tagged_enum<_Int, _Tags...>::type;

template <typename... _Tags>
using tagged_size_t = tagged_int<std::size_t, _Tags...>;

} // namespace dark
