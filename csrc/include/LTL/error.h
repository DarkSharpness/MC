#pragma once
#include <format>
#include <stdexcept>

namespace dark {

struct LTLException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

template <typename Cond, typename... Args>
inline auto docheck(Cond &&cond, std::format_string<Args...> fmt, Args &&...args) -> void {
    if (!cond) [[unlikely]]
        throw LTLException(std::format(fmt, std::forward<Args>(args)...));
}

} // namespace dark
