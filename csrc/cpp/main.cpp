#include "LTL/input.h"
#include <iostream>

auto main() -> int {
    auto &is     = std::cin;
    auto graph   = dark::TSGraph::read(is);
    auto formula = dark::LTLFormula::read(is);
    return 0;
}
