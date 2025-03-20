#pragma once

#include <memory>
namespace dark {

namespace pimpl {

template <typename T>
auto destructor(T *ptr) -> void;

template <typename T>
struct deleter {
    auto operator()(T *ptr) const -> void {
        destructor(ptr);
    }
};

} // namespace pimpl

template <typename T>
using pimpl_ptr = std::unique_ptr<T, pimpl::deleter<T>>;

} // namespace dark
