#include "LTL/automa.h"
#include "LTL/error.h"
#include "LTL/node.h"
#include "LTL/node_impl.h"
#include "automa_aux.h"
#include "utils/bitset.h"
#include "utils/error.h"
#include "utils/irange.h"
#include <cstddef>
#include <format>
#include <optional>
#include <ostream>
#include <span>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace dark {

namespace {

struct formula_bitset : private bitset {
    using bitset::bitset;
    using bitset::subset;
    using bitset::to_string;
    using bitset::operator[];
    using bitset::as_bitset;
    using bitset::size;
    auto operator[](fid f) const -> bool {
        if (f.is_negation())
            return f == fid::False ? false : !(*this)[(~f).raw()];
        else
            return f == fid::True ? true : (*this)[f.raw()];
    }
};

using fset = formula_bitset;

struct FormulaCollector {
    auto get_formulas() const -> std::span<const Formula> {
        return formulas;
    }

    auto map(BaseNode *node) const -> fid {
        auto iter = mapping.find(node);
        assume(iter != mapping.end(), "Node not found, call build first");
        return iter->second;
    }

    static auto from(BaseNode *ptr, std::size_t num_atomics) -> FormulaCollector {
        auto collector = FormulaCollector{num_atomics};
        collector.build(ptr);
        return collector;
    }

private:
    FormulaCollector(std::size_t num_atomics) {
        formulas.reserve(num_atomics);
        for (std::size_t i = 0; i < num_atomics; ++i)
            formulas.push_back(Formula::atomic<Formula::op::ATOMIC>(fid(i)));
    }

    auto build(BaseNode *) -> fid;
    std::vector<Formula> formulas;
    std::unordered_map<BaseNode *, fid> mapping;
    std::unordered_map<Formula, std::size_t, Formula::Hash> visited;
};

auto FormulaCollector::build(BaseNode *ptr) -> fid {
    if (auto iter = mapping.find(ptr); iter != mapping.end())
        return iter->second;
    using For = Formula;

    const auto for2id = [this](For s) {
        assume(!s.is_atomic(), "Do not use atomic formula here");
        auto [it, success] = visited.try_emplace(s);
        if (success) { // double direction mapping
            it->second = formulas.size();
            formulas.push_back(s);
        }
        return fid(it->second);
    };

    const auto update = [this, ptr](fid id) {
        assume(mapping.try_emplace(ptr, id).second, "Node already exists");
        return id;
    };

    // build the formula recursively
    using enum Formula::op;

    if (auto unary = ptr->is<UnaryNode>()) {
        const auto child = build(unary->child.get());
        if (unary->is<NotNode>())
            return update(~child);
        if (unary->is<NextNode>())
            return update(for2id(For::unary<NEXT>(child)));
        if (unary->is<EventualNode>()) // -> true U child
            return update(for2id(For::binary<UNTIL>(fid::True, child)));
        if (unary->is<AlwaysNode>()) // -> not (true U not child)
            return update(~for2id(For::binary<UNTIL>(fid::True, ~child)));
    } else if (auto binary = ptr->is<BinaryNode>()) {
        const auto lhs = build(binary->lhs.get());
        const auto rhs = build(binary->rhs.get());
        if (binary->is<ConjNode>())
            return update(for2id(For::binary<CONJ>(lhs, rhs)));
        if (binary->is<DisjNode>()) // -> not (not lhs and not rhs)
            return update(~for2id(For::binary<CONJ>(~lhs, ~rhs)));
        if (binary->is<UntilNode>())
            return update(for2id(For::binary<UNTIL>(lhs, rhs)));
        if (binary->is<ImplNode>()) // -> not (lhs and not rhs)
            return update(~for2id(For::binary<CONJ>(lhs, ~rhs)));
    } else if (auto atomic = ptr->is<AtomicNode>()) {
        using enum AtomicNode::Type;
        if (atomic->type == True)
            return fid::True;
        if (atomic->type == False)
            return fid::False;
        return fid(atomic->index);
    }

    docheck(false, "Invalid node type");
    std::unreachable();
}

auto debug_check_formula(std::span<const Formula> formulas, std::size_t num_ap) -> void {
    assume(formulas.size() >= num_ap, "Invalid formula size");
    assume(num_ap > 0, "Invalid number of atomic propositions");

    // first num_ap must be atomic
    // for (const auto &f : formulas | std::views::take(num_ap))
    for (const auto i : irange(num_ap))
        assume(formulas[i].is_atomic(), "Invalid formula type");

    // Must be DAG-like structure
    for (const auto i : irange(num_ap, formulas.size())) {
        const auto &f = formulas[i];
        assume(f.is_binary() || f.is_unary(), "Invalid formula type");
        const auto valid = [i](fid f) -> bool {
            return f.original() < i || f == fid::True || f == fid::False;
        };
        assume(valid(f[0]), "Invalid formula index");
        if (f.is_binary())
            assume(valid(f[1]), "Invalid formula index");
    }
}

struct SetBuilder {
public:
    static auto from(std::span<const Formula> formulas, std::size_t num_aps) -> SetBuilder {
        auto builder = SetBuilder{formulas, num_aps};
        builder.build();
        return builder;
    }

    auto elementary_sets() const -> std::span<const fset> {
        return sets;
    }

    auto debug(std::ostream &os) const -> void;

private:
    SetBuilder(std::span<const Formula> formulas, std::size_t num_aps) :
        formulas(formulas), num_aps(num_aps) {}

    // hidden build function
    auto build() -> void;

    // prepare the indices for the set
    auto prepare() -> std::vector<std::size_t>;

    // whether the set is an elementary set. if so, return the full bitset
    // the given set already define whether the ap/given formula is true or false
    auto check(fset) const -> std::optional<fset>;

    // finally accepted sets
    std::vector<fset> sets;

    // common parameters
    const std::span<const Formula> formulas;
    const std::size_t num_aps;
};

auto SetBuilder::prepare() -> std::vector<std::size_t> {
    auto indice_set = std::unordered_set<std::size_t>{};
    auto try_add_ap = [this, &indice_set](fid f) {
        const auto n = f.original();
        if (n < num_aps)
            indice_set.insert(n);
    };

    // extract all the ap in the subformula and those uncertain
    // formulas (i.e., next and until), which will also be taken
    // into account when we enumerate the sets
    for (const auto i : irange(num_aps, formulas.size())) {
        const auto &f = formulas[i];
        try_add_ap(f[0]);
        if (f.is_binary())
            try_add_ap(f[1]);
        if (f.is_uncertain())
            indice_set.insert(i);
    }

    return {indice_set.begin(), indice_set.end()};
}

auto SetBuilder::check(fset set) const -> std::optional<fset> {
    for (const auto i : irange(num_aps, formulas.size())) {
        const auto &f = formulas[i];
        assume(!f.is_atomic(), "Atomic formula should not be here");
        if (f.is_conj()) {
            set[i] = set[f[0]] && set[f[1]];
        } else if (f.is_until()) {
            // f is uncertain, and maybe conflict
            // use local until property to check
            const auto lhs = set[f[0]];
            const auto rhs = set[f[1]];
            const auto cur = bool(set[i]);
            if (!cur && rhs)
                return std::nullopt;
            if (cur && !lhs && !rhs)
                return std::nullopt;
        }
        // do nothing, all the atomic & next have already been set
    }
    return set;
}

auto SetBuilder::build() -> void {
    const auto indices = prepare();
    const auto size    = indices.size();
    assume(size < 32, "Too many indices to enumerate, stop here");
    auto current_bitset = fset{formulas.size()};
    // enumerate all AP and uncertain formulas
    for (const auto i : irange(std::size_t{1} << size)) {
        for (const auto j : irange(size))
            current_bitset[indices[j]] = (i >> j) & 1;
        if (auto result = check(current_bitset))
            sets.push_back(std::move(*result));
    }
}

[[maybe_unused]]
auto SetBuilder::debug(std::ostream &os) const -> void {
    // first of all print all the formula
    os << std::format("Number of atomic propositions: {}\n", num_aps);
    os << std::format("Number of formulas: {}\n", formulas.size());
    auto display = std::vector<std::string>(formulas.size());
    auto nameof  = [&display](fid f) -> std::string {
        if (f.is_negation())
            return f == fid::False ? "False" : "!" + display[(~f).raw()];
        else
            return f == fid::True ? "True" : display[f.raw()];
    };

    for (const auto i : irange(num_aps)) {
        const auto &f = formulas[i];
        assume(f.is_atomic(), "First num_aps must be atomic");
        display[i] = std::format("[{}]", i);
    }

    for (const auto i : irange(num_aps, formulas.size())) {
        const auto &f = formulas[i];
        assume(!f.is_atomic(), "Atomic formula should not be here");
        if (f.is_conj())
            display[i] = std::format("({} /\\ {})", nameof(f[0]), nameof(f[1]));
        else if (f.is_until())
            display[i] = std::format("({} U {})", nameof(f[0]), nameof(f[1]));
        else
            display[i] = std::format("(O {})", nameof(f[0])); // next
    }

    for (const auto i : irange(formulas.size()))
        os << std::format("Formula {}: {}\n", i, display[i]);
}

struct VisitHelper {
public:
    VisitHelper(std::size_t num_aps, std::span<const Formula> formulas) :
        num_aps(num_aps), formulas(formulas) {}

    auto build(const fset &f) -> void;
    auto accept(const fset &f) const -> bool;

private:
    bitset require;
    bitset indices;

    const std::size_t num_aps;
    const std::span<const Formula> formulas;
};

auto VisitHelper::build(const fset &x) -> void {
    bitset new_r{formulas.size()};
    bitset new_i{formulas.size()};
    auto insert = [&new_r, &new_i](std::size_t idx, bool value) {
        new_i[idx] = true;
        new_r[idx] = value;
    };
    for (const auto i : irange(num_aps, formulas.size())) {
        const auto &f = formulas[i];
        assume(!f.is_atomic(), "Atomic formula should not be here");
        if (f.is_next()) {
            if (f[0] == fid::True || f[0] == fid::False)
                continue; // nothing to do

            // x[i] = y[f[0]]
            insert(f[0].original(), f[0].is_negation() ^ x[i]);
        } else if (f.is_until()) {
            if (x[f[1]]) // ok, already true
                continue;

            if (x[i]) {
                assume(x[f[0]], "implementation error");
                insert(i, true);
                continue;
            }

            if (x[f[0]])
                insert(i, false);
        }
        // do nothing, all the atomic & next have already been set
    }
    this->require = std::move(new_r);
    this->indices = std::move(new_i);
}

auto VisitHelper::accept(const fset &f) const -> bool {
    assume(f.size() == indices.size() && f.size() == require.size());
    // all the indiced formula must be satisfied
    const auto &r = require.as_bitset();
    const auto &i = indices.as_bitset();
    const auto &F = f.as_bitset();
    return (F & i) == r;
}

} // namespace

// Transform an LTL formula into a GNBA
auto GNBA::build(BaseNode *ptr, std::size_t num_atomics) -> GNBA {
    const auto num_ap = num_atomics;
    docheck(num_ap > 0, "Invalid number of atomic propositions");

    // Now we have abstract the formula into structures
    const auto collector = FormulaCollector::from(ptr, num_ap);
    const auto formulas  = collector.get_formulas();
    debug_check_formula(formulas, num_ap);

    // First, find all the elementary set of the formulas
    const auto builder = SetBuilder::from(formulas, num_ap);

    const auto sets = builder.elementary_sets();
    const auto size = sets.size(); // the size of the GNBA
    const auto root = collector.map(ptr);

    auto initial_set = [&] {
        auto initial   = bitset{size};
        const auto idx = root.original();
        for (const auto i : irange(size))
            if (sets[i][idx] != root.is_negation())
                initial[i] = true; // if negation, require false (not in set)
        return initial;
    }();

    auto transition = [&] {
        auto transition = std::vector<EdgeMap>(size);
        auto visit_aux  = VisitHelper{num_ap, formulas};
        for (const auto i : irange(size)) {
            const auto &s = sets[i];
            auto trigger  = s.subset(num_ap);
            auto targets  = bitset{size};
            visit_aux.build(s);
            for (const auto j : irange(size))
                if (visit_aux.accept(sets[j]))
                    targets[j] = true;
            transition[i] = {{trigger, targets}};
        }
        return transition;
    }();

    auto result = GNBA{};
    using __detail::Automa;
    static_cast<Automa &>(result) = Automa{
        .num_states     = size,
        .num_triggers   = num_ap,
        .initial_states = std::move(initial_set),
        .transitions    = std::move(transition),
    };
    auto final_list = [&] {
        auto final = std::vector<bitset>{};
        for (const auto i : irange(num_ap, formulas.size())) {
            const auto &f = formulas[i];
            if (f.is_until()) {
                auto final_set = bitset{size};
                for (const auto j : irange(size))
                    if (!sets[j][i] || sets[j][f[1]])
                        final_set[j] = true;
                final.push_back(std::move(final_set));
            }
        }
        return final;
    }();
    result.final_states_list = std::move(final_list);
    return result;
}

} // namespace dark
