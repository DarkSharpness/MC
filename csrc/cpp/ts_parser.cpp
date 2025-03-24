#include "LTL/error.h"
#include "LTL/ts.h"
#include "utils/bitset.h"
#include "utils/irange.h"
#include <algorithm>
#include <cstddef>
#include <istream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace dark {

[[maybe_unused]]
static auto operator>>(std::istream &is, TSGraph::Transition &set) -> std::istream & {
    is >> set.from >> set.action >> set.into;
    return is;
}

auto TSGraph::read(std::istream &is) -> TSGraph {
    static constexpr auto readline = [](std::istream &is) {
        auto line = std::string{};
        docheck(std::getline(is, line), "expect more lines");
        return std::stringstream{std::move(line)};
    };
    static constexpr auto readrange = [](std::istream &is, auto tmp, auto iter) {
        auto ss = readline(is);
        using T = decltype(tmp);
        std::ranges::copy(std::istream_iterator<T>{ss}, std::istream_iterator<T>{}, iter);
    };
    auto initial_set   = std::vector<std::size_t>{};
    const auto readset = [&initial_set](std::istream &is, bitset &set) {
        initial_set.clear();
        readrange(is, std::make_signed_t<std::size_t>{}, std::back_inserter(initial_set));
        // empty set means all states
        if (initial_set.size() == 1 && initial_set[0] == static_cast<std::size_t>(-1))
            return;
        for (const auto i : initial_set) {
            docheck(i < set.size(), "initial state index out of range");
            set[i] = true;
        }
    };

    auto result = TSGraph{};
    readline(is) >> result.num_states >> result.num_transitions;
    result.initial_set = bitset{result.num_states};
    readset(is, result.initial_set);
    readrange(is, std::string{}, std::back_inserter(result.action_map));
    readrange(is, std::string{}, std::back_inserter(result.atomic_map));
    const auto kNumAP = result.atomic_map.size();
    for ([[maybe_unused]] const auto _ : irange(result.num_transitions))
        readline(is) >> result.transitions.emplace_back();
    for ([[maybe_unused]] const auto _ : irange(result.num_states))
        readset(is, result.ap_sets.emplace_back(kNumAP));

    result.post_init();
    return result;
}

auto TSGraph::post_init() -> void {
    atomic_rev_map.reserve(atomic_map.size());
    for (const auto &s : atomic_map)
        atomic_rev_map[s] = atomic_rev_map.size();
    transition_list.assign(num_states, bitset{num_states});
    for (const auto &[from, action, into] : transitions) {
        docheck(from < num_states, "transition from out of range");
        docheck(into < num_states, "transition to out of range");
        docheck(action < action_map.size(), "transition action out of range");
        transition_list[from][into] = true;
    }
}

auto TSGraph::debug(std::ostream &os) const -> void {
    os << num_states << ' ' << num_transitions << '\n';
    os << "initial_set: ";
    for (const auto i : irange(initial_set.size()))
        if (initial_set[i])
            os << i << ' ';
    os << '\n';
    os << "action_map: ";
    for (const auto i : irange(action_map.size()))
        os << action_map[i] << (i + 1 == action_map.size() ? '\n' : ' ');
    os << "atomic_map: ";
    for (const auto i : irange(atomic_map.size()))
        os << atomic_map[i] << (i + 1 == atomic_map.size() ? '\n' : ' ');
    os << "transitions:\n";
    for (const auto &t : transitions)
        os << t.from << ' ' << action_map[t.action] << ' ' << t.into << '\n';
    for (const auto &s : ap_sets) {
        for (const auto i : irange(s.size()))
            os << int(s[i]);
        os << '\n';
    }
}

} // namespace dark
