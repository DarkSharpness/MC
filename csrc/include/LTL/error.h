#pragma once
#include <stdexcept>

namespace dark {

struct LTLException : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

template <typename Cond>
inline auto docheck(Cond &&cond, const char *msg) -> void {
    if (!cond) [[unlikely]]
        throw LTLException(msg);
}

} // namespace dark
