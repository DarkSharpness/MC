#pragma once
#include "node.h"
#include "utils/bitset.h"
#include "utils/type.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace dark {

namespace __detail {

struct Automa {
    std::size_t num_states;   // states
    std::size_t num_triggers; // triggers

    // mapping: (trigger -> all potential next states)
    using EdgeMap = std::unordered_map<bitset, bitset>;

    // state index -> (which state can be reached next)
    bitset initial_states;
    std::unordered_map<bitset, EdgeMap> transitions;
};

struct StateTag;
struct ActionTag;

template <typename Derived>
struct AutomaImpl : public Automa {
    using State  = tagged_size_t<Derived, StateTag>;
    using Action = tagged_size_t<Derived, ActionTag>;
};

} // namespace __detail

struct GNBA : private __detail::AutomaImpl<GNBA> {
public:
    static auto build(BaseNode *, std::size_t) -> GNBA;

private:
    std::vector<bitset> final_states_list;
};

struct NBA : private __detail::AutomaImpl<NBA> {
public:
    static auto fromGNBA(const GNBA &) -> NBA;

private:
    bitset final_state;
};

} // namespace dark
