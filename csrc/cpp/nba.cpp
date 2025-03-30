#include "LTL/automa.h"
#include "utils/bitset.h"
#include "utils/error.h"
#include "utils/irange.h"
#include <vector>

namespace dark {

auto Automa::validate() const -> void {
    assume(num_states > 0, "empty automa");
    assume(num_triggers > 0, "empty trigger set (AP)");
    for (const auto &edges : transitions) {
        for (const auto &[trig, set] : edges) {
            assume(trig.size() == num_triggers, "invalid trigger size");
            assume(set.size() == num_states, "invalid set size");
        }
    }
    assume(used_ap_mask.size() == num_triggers, "invalid unused AP mask size");
}

auto NBA::fromGNBA(const GNBA &src) -> NBA {
    src.validate();
    const auto num_final = src.final_states_list.size();
    auto dst             = NBA();
    if (num_final <= 1) {
        dst.num_states     = src.num_states;
        dst.num_triggers   = src.num_triggers; // same AP set as trigger.
        dst.used_ap_mask   = src.used_ap_mask;
        dst.initial_states = src.initial_states;
        dst.final_states   = bitset{src.num_states};
        if (num_final == 1)
            dst.final_states = src.final_states_list[0];
        else
            dst.final_states.set_all();
        dst.transitions = src.transitions;
        dst.validate();
        return dst;
    }

    const auto old_size = src.num_states;
    const auto new_size = old_size * num_final;

    dst.num_states     = new_size;
    dst.num_triggers   = src.num_triggers; // same AP set as trigger.
    dst.used_ap_mask   = src.used_ap_mask;
    dst.initial_states = src.initial_states.expand(new_size);
    dst.final_states   = src.final_states_list[0].expand(new_size);

    // make the transition function
    auto transitions = std::vector<EdgeMap>(new_size);
    for (const auto j : irange(num_final)) {
        const auto &final = src.final_states_list[j];
        for (const auto i : irange(old_size)) {
            auto &new_edges   = transitions[j * old_size + i];
            const auto offset = ((j + final[i]) % num_final) * old_size;
            for (const auto &[trig, set] : src.transitions[i]) {
                auto [it, success] = new_edges.try_emplace(trig);
                assume(success, "duplicate transition");
                it->second.set_at(new_size, offset, set);
            }
        }
    }

    dst.transitions = std::move(transitions);
    dst.validate();

    return dst;
}

} // namespace dark
