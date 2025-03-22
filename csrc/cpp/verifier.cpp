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

// template <typename _Fn>
// auto post(ProductSystem::State s, const NBA &nba, const TSView &ts, _Fn &&f) -> void {
//     auto [idx_ts, idx_nba] = s;
//     const auto &range      = (idx_ts == entry_pos) ? ts.initial_set : ts.transitions[idx_ts];
//     for (const auto t : range)
//         if (auto *target = accept(nba, idx_nba, ts.atomics[t]))
//             for (const auto q : *target)
//                 f({t, q});
// }

#define for_each_post(input, ss, f)                                                                \
    do {                                                                                           \
        auto [idx_ts, idx_nba] = input;                                                            \
        const auto &range      = (idx_ts == entry_pos) ? ts.initial_set : ts.transitions[idx_ts];  \
        for (const auto t : range)                                                                 \
            if (auto *target = accept(nba, idx_nba, ts.atomics[t]))                                \
                for (const auto q : *target) {                                                     \
                    const auto ss = State{t, q};                                                   \
                    do                                                                             \
                        f while (0);                                                               \
                }                                                                                  \
    } while (0)

auto ProductSystem::reachable_cycle(State input) -> bool {
    U.push(input);
    R.insert(input);
    do {
        const auto cur     = U.top();
        auto has_unvisited = false;
        for_each_post(cur, s, {
            if (R.insert(s).second) {
                has_unvisited = true;
                U.push(s);
            }
        });
        if (!has_unvisited) {
            U.pop();
            if (cycle_check(cur))
                return true;
        }
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
        const auto cur     = V.top();
        auto has_unvisited = false;
        for_each_post(cur, s, {
            if (s == start)
                return true;
            if (T.insert(s).second) {
                has_unvisited = true;
                V.push(s);
            }
        });

        if (!has_unvisited)
            V.pop();
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
