#pragma once
#include <iosfwd>

namespace dark {

struct LTLProgram {
    static auto work(std::istream &gs, std::istream &ts, std::ostream &os) -> void;
};

} // namespace dark
