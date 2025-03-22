#include "LTL/input.h"
#include "utils/error.h"
#include <argparse/argparse.hpp>
#include <fstream>
#include <iostream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

auto work(int argc, const char **argv) -> void {
    if (argc == 1) {
        std::cerr << "No arguments provided, using stdin/stdout\n";
        std::cerr << "Use --help for more information\n";
        std::cerr << "Please input the transition system and LTL formula\n";
        return dark::LTLProgram::work(std::cin, std::cin, std::cout);
    }

    auto program = argparse::ArgumentParser{"LTL", "1.0.0"};

    program.add_argument("file").help("Transition system file path").remaining().nargs(0, 1);
    program.add_argument("--ts").help("Transition system file path").nargs(1);
    program.add_argument("--ltl").help("LTL formula file path").nargs(1);
    program.add_argument("--output").help("Output file path").nargs(1);
    program.add_argument("--silent", "-S")
        .help("Disable debug output")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--verbose", "-V")
        .help("Enable verbose output")
        .default_value(false)
        .implicit_value(true);

    program.parse_args(argc, argv);

    if (program["--verbose"] == true && program["--silent"] == true)
        throw std::runtime_error("Cannot be both verbose and silent");
    if (program["--verbose"] == true)
        dark::debugger(true);
    if (program["--silent"] == true)
        dark::debugger(false);

    auto out_file    = std::ofstream{};
    auto &out_stream = [&] -> std::ostream & {
        if (program.present("--output")) {
            out_file.open(program.get("--output"));
            return out_file;
        } else {
            return std::cout;
        }
    }();

    if (auto vec = program.present<std::vector<std::string>>("file"); vec && vec->size() > 0) {
        if (program.present("--ts") || program.present("--ltl"))
            throw std::runtime_error("Cannot provide both positional and --ts/--ltl arguments");
        if (vec->size() != 1)
            throw std::runtime_error("Only one positional argument is allowed");
        auto in_file = std::ifstream{vec->at(0)};
        return dark::LTLProgram::work(in_file, in_file, out_stream);
    }

    auto ts_stream  = std::ifstream{program.get("--ts")};
    auto ltl_stream = std::ifstream{program.get("--ltl")};

    return dark::LTLProgram::work(ts_stream, ltl_stream, out_stream);
}

auto main(int argc, const char **argv) -> int {
    try {
        work(argc, argv);
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
