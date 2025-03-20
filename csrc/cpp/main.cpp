#include "LTL/input.h"
#include <iostream>

auto main() -> int {
    dark::LTLProgram::work(std::cin, std::cin, std::cout);
    return 0;
}
