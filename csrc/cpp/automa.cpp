#include "LTL/automa.h"
#include "LTL/error.h"
#include "LTL/node.h"
#include "LTL/node_impl.h"
#include "automa_aux.h"
#include "utils/dynamic_bitset.h"
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
        assume(f[0].original() < i, "Invalid formula index");
        if (f.is_binary())
            assume(f[1].original() < i, "Invalid formula index");
    }
}

struct SetBuilder {
public:
    static auto from(std::span<const Formula> formulas, std::size_t num_aps) -> SetBuilder {
        auto builder = SetBuilder{formulas, num_aps};
        builder.build();
        return builder;
    }

    auto elementary_sets() const -> std::span<const dynamic_bitset> {
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
    auto check(dynamic_bitset) const -> std::optional<dynamic_bitset>;

    // finally accepted sets
    std::vector<dynamic_bitset> sets;

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

auto SetBuilder::check(dynamic_bitset set) const -> std::optional<dynamic_bitset> {
    const auto eval = [&set](fid f) -> bool {
        if (f.is_negation())
            return f == fid::False ? false : !set[(~f).raw()];
        else
            return f == fid::True ? true : set[f.raw()];
    };

    for (const auto i : irange(num_aps, formulas.size())) {
        const auto &f = formulas[i];
        assume(!f.is_atomic(), "Atomic formula should not be here");
        if (f.is_conj()) {
            set.set(i, eval(f[0]) && eval(f[1]));
        } else if (f.is_until()) {
            // f is uncertain, and maybe conflict
            // use local until property to check
            const auto lhs = eval(f[0]);
            const auto rhs = eval(f[1]);
            const auto cur = set[i];
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
    auto current_bitset = dynamic_bitset{formulas.size()};
    // enumerate all AP and uncertain formulas
    for (const auto i : irange(std::size_t{1} << size)) {
        for (const auto j : irange(size))
            current_bitset.set(indices[j], (i >> j) & 1);
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

} // namespace

// Transform an LTL formula into a GNBA
auto GNBA::build(BaseNode *ptr, std::size_t num_atomics) -> GNBA {
    const auto num_ap = num_atomics;
    docheck(num_ap > 0, "Invalid number of atomic propositions");

    // Now we have abstract the formula into structures
    const auto collector = FormulaCollector::from(ptr, num_ap);
    const auto formulas  = collector.get_formulas();
    debug_check_formula(formulas, num_ap);
    // const auto root = collector.map(ptr);

    const auto builder = SetBuilder::from(formulas, num_ap);

    // {
    //     auto os = debugger();
    //     builder.debug(os);
    // }
    // for (auto set : builder.elementary_sets())
    //     debugger() << set.to_string() << '\n';

    // First, find all the elementary set of the formulas
    auto result = GNBA{};
    return result;
}

} // namespace dark
