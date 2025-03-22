#include "LTL/automa.h"
#include "LTL/node.h"
#include "LTL/ts.h"
#include "utils/bitset.h"
#include <bit>
#include <cstddef>
#include <stack>
#include <unordered_set>
#include <vector>

namespace dark {

namespace {

struct ProductSystem {
public:
    struct State {
        std::size_t idx_ts;
        std::size_t idx_nba;
        [[maybe_unused]] // clangd false positive warning
        friend auto
        operator==(const State &lhs, const State &rhs) -> bool = default;
    };

    static auto can_run(const TSView &ts, const NBA &nba) -> bool;

private:
    auto reachable_cycle(State s) -> bool;
    auto cycle_check(State s) -> bool;

    ProductSystem(const TSView &ts, const NBA &nba);
    const TSView &ts;
    const NBA &nba;

    struct Hash {
        auto operator()(const State &s) const -> std::size_t {
            return std::rotl(s.idx_ts, 32) ^ s.idx_nba;
        }
    };

    std::unordered_set<State, Hash> R;       // visited states in outer DFS
    std::stack<State, std::vector<State>> U; // stack for outer DFS
};

ProductSystem::ProductSystem(const TSView &ts, const NBA &nba) : ts{ts}, nba{nba} {}

inline constexpr auto entry_pos = static_cast<std::size_t>(-1);

// Whether an NBA accept at a state idx with atomic propositions AP as trigger
auto accept(const NBA &nba, std::size_t idx, const bitset &AP) -> const bitset * {
    const auto &map = nba.transitions[idx];
    if (auto it = map.find(AP & nba.used_ap_mask); it != map.end())
        return &it->second;
    return nullptr;
}

auto ProductSystem::can_run(const TSView &ts, const NBA &nba) -> bool {
    auto system = ProductSystem{ts, nba};
    for (const auto i : nba.initial_states)
        if (system.reachable_cycle({entry_pos, i}))
            return true;
    return false;
}

template <typename _Fn>
auto post(ProductSystem::State s, const NBA &nba, const TSView &ts, _Fn &&f) -> void {
    auto [idx_ts, idx_nba] = s;
    const auto &range      = (idx_ts == entry_pos) ? ts.initial_set : ts.transitions[idx_ts];
    for (const auto t : range)
        if (auto *target = accept(nba, idx_nba, ts.atomics[t]))
            for (const auto q : *target)
                f({t, q});
}

auto ProductSystem::reachable_cycle(State s) -> bool {
    U.push(s);
    R.insert(s);
    do {
        const auto s = U.top();
        U.pop();
        auto has_unvisited = bool{};
        post(s, nba, ts, [&](State s) {
            if (R.contains(s))
                return;
            U.push(s);
            R.insert(s);
            has_unvisited = true;
        });
        if (!has_unvisited && cycle_check(s))
            return true;
    } while (!U.empty());
    return false;
}

auto ProductSystem::cycle_check(State start) -> bool {
    const auto [idx_ts, idx_nba] = start;
    if (idx_ts == entry_pos || !nba.final_states[idx_nba])
        return false;

    std::unordered_set<State, Hash> T;       // visited states in inner DFS
    std::stack<State, std::vector<State>> V; // stack for inner DFS

    V.push(start);
    T.insert(start);
    do {
        std::unordered_set<State, Hash> post_set;
        const auto s = V.top();
        post(s, nba, ts, [&](State s) { post_set.insert(s); });
        if (post_set.contains(start))
            return true;
        for (const auto &s : T)
            post_set.erase(s);
        if (post_set.empty()) {
            V.pop();
        } else {
            for (const auto &s : post_set) {
                V.push(s);
                T.insert(s);
            }
        }
    } while (!V.empty());
    return false;
}

} // namespace

auto verifyLTL(BaseNode *node, const TSView &ts) -> bool {
    // build the GNBA of the reverse LTL formula
    const auto GNBA_ = GNBA::build(node, ts.num_atomics, /*negate=*/true);
    // convert GNBA to NBA
    const auto NBA_ = NBA::fromGNBA(GNBA_);
    // use product system to verify the LTL formula
    return ProductSystem::can_run(ts, NBA_) ? false : true;
}

} // namespace dark
