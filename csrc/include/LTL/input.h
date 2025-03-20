#pragma once
#include "node.h"
#include "utils/dynamic_bitset.h"
#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

namespace dark {

struct TSGraph {
public:
    static auto read(std::istream &is) -> TSGraph;

    struct Transition {
        std::size_t from;
        std::size_t action;
        std::size_t into;
    };

    auto debug(std::ostream &os) const -> void;

private:
    std::size_t num_states;
    std::size_t num_transitions;
    dynamic_bitset initial_set;

    std::vector<std::string> action_map; // action
    std::vector<std::string> atomic_map; // AP
    std::vector<Transition> transitions;
    std::vector<dynamic_bitset> ap_sets;
};

struct LTLFormula {
public:
    static auto read(std::istream &is) -> LTLFormula;
    auto root_node() const & -> BaseNode * {
        return root.get();
    }

private:
    LTLFormula(NodePtr root) : root(std::move(root)) {}
    NodePtr root;
};

} // namespace dark
