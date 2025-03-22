#pragma once
#include "utils/bitset.h"
#include <cstddef>
#include <iosfwd>
#include <optional>
#include <string_view>
#include <vector>

namespace dark {

struct TSGraph {
public:
    static auto read(std::istream &) -> TSGraph;

    struct Transition {
        std::size_t from;
        std::size_t action;
        std::size_t into;
    };

    auto debug(std::ostream &) const -> void;
    // map an atomic proposition to its index
    auto map_atomic(std::string_view) const -> std::size_t;

private:
    std::size_t num_states;
    std::size_t num_transitions;
    bitset initial_set;

    std::vector<std::string> action_map; // action
    std::vector<std::string> atomic_map; // atomic proposition
    std::vector<Transition> transitions;
    std::vector<bitset> ap_sets;

    // Post init function and post init data
    auto post_init() -> void;
    std::vector<bitset> transition_list;
    std::unordered_map<std::string_view, std::size_t> atomic_rev_map;
    friend struct TSView;
};

struct TSView {
    TSView(const TSGraph &, std::optional<bitset> = std::nullopt);

    std::size_t num_states;  // number of states
    std::size_t num_atomics; // number of atomic propositions

    bitset initial_set;                  // set of initial state
    std::span<const bitset> transitions; // state -> set of states
    std::span<const bitset> atomics;     // state -> set of atomic propositions
};

inline TSView::TSView(const TSGraph &graph, std::optional<bitset> new_init) :
    num_states(graph.num_states), num_atomics(graph.atomic_map.size()),
    initial_set(std::move(new_init).value_or(graph.initial_set)),
    transitions(graph.transition_list), atomics(graph.ap_sets) {}

} // namespace dark
