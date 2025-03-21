#pragma once
#include "node.h"
#include "utils/dynamic_bitset.h"
#include "utils/type.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace dark {

namespace __detail {

struct Automa {
    using EdgeSet = std::vector<dynamic_bitset>; // action -> set of visitable states

    std::size_t num_states;   // states
    std::size_t num_triggers; // trigger of a transition

    dynamic_bitset initial_states;
    std::unordered_map<std::size_t, EdgeSet> transitions;
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
    std::vector<dynamic_bitset> final_states_list;
};

struct NBA : private __detail::AutomaImpl<NBA> {
public:
    static auto fromGNBA(const GNBA &) -> NBA;

private:
    dynamic_bitset final_state;
};

} // namespace dark
