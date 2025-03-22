#include "LTL/automa.h"
#include "utils/bitset.h"
#include "utils/error.h"
#include "utils/irange.h"
#include <vector>

namespace dark {

auto __detail::Automa::validate() const -> void {
    assume(num_states > 0, "empty automa");
    assume(num_triggers > 0, "empty trigger set (AP)");
    for (const auto &edges : transitions) {
        for (const auto &[trig, set] : edges) {
            assume(trig.size() == num_triggers, "invalid trigger size");
            assume(set.size() == num_states, "invalid set size");
        }
    }
}

auto NBA::fromGNBA(const GNBA &src) -> NBA {
    src.validate();
    const auto num_final = src.final_states_list.size();
    assume(num_final != 0, "not implemented for GNBA with no final states yet");
    const auto old_size = src.num_states;
    const auto new_size = old_size * num_final;
    auto dst            = NBA{};

    dst.num_states     = new_size;
    dst.num_triggers   = src.num_triggers; // same AP set as trigger.
    dst.initial_states = src.initial_states.expand(new_size);
    dst.final_state    = src.final_states_list[0].expand(new_size);

    // make the transition function
    auto transitions = std::vector<EdgeMap>(new_size);
    for (const auto j : irange(num_final)) {
        const auto &final = src.final_states_list[j];
        for (const auto i : irange(old_size)) {
            auto &new_edges   = transitions[j * old_size + i];
            const auto offset = ((j + final[i]) % num_final) * old_size;
            for (const auto &[trig, set] : src.transitions[i]) {
                auto [it, success] = new_edges.try_emplace(trig, new_size);
                assume(success, "duplicate transition");
                it->second.or_at(offset, set);
            }
        }
    }

    dst.transitions = std::move(transitions);
    dst.validate();
    return dst;
}

} // namespace dark
