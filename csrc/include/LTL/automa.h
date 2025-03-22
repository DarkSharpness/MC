#pragma once
#include "node.h"
#include "utils/bitset.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace dark {

namespace __detail {

struct Automa {
    std::size_t num_states;   // states
    std::size_t num_triggers; // triggers (= num ap)

    // mapping: (trigger -> all potential next states)
    using EdgeMap = std::unordered_map<bitset, bitset>;

    // state index -> (which state can be reached next)
    bitset initial_states;
    std::vector<EdgeMap> transitions;

    // only use the AP that appear in the formula
    bitset used_ap_mask;

    // try to validate. if false, throw an exception
    auto validate() const -> void;
};

} // namespace __detail

struct GNBA;

struct NBA : __detail::Automa {
    static auto fromGNBA(const GNBA &) -> NBA;
    bitset final_states;
};

struct GNBA : __detail::Automa {
    static auto build(BaseNode *, std::size_t, bool negate) -> GNBA;
    std::vector<bitset> final_states_list;
};

} // namespace dark
