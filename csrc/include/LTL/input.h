#pragma once
#include <iosfwd>

namespace dark {

struct LTLProgram {
    static auto work(std::istream &ts, std::istream &ltl, std::ostream &os) -> void;
};

} // namespace dark
