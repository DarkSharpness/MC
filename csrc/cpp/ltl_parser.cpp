#include "LTL/error.h"
#include "LTL/input.h"
#include "LTL/node.h"
#include "LTL/node_impl.h"
#include "LTL/ts.h"
#include "LTLLexer.h"
#include "LTLParser.h"
#include "LTLVisitor.h"
#include "utils/bitset.h"
#include "utils/error.h"
#include "utils/irange.h"
#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <any>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

namespace dark {

namespace {

struct Visitor final : LTLVisitor {
    Visitor(const TSGraph &graph) : graph(graph) {}

    auto visitProg(LTLParser::ProgContext *) -> std::any override;
    auto visitNot(LTLParser::NotContext *) -> std::any override;
    auto visitDisjunction(LTLParser::DisjunctionContext *) -> std::any override;
    auto visitNext(LTLParser::NextContext *) -> std::any override;
    auto visitEventually(LTLParser::EventuallyContext *) -> std::any override;
    auto visitConjunction(LTLParser::ConjunctionContext *) -> std::any override;
    auto visitAtomic(LTLParser::AtomicContext *) -> std::any override;
    auto visitAlways(LTLParser::AlwaysContext *) -> std::any override;
    auto visitImplication(LTLParser::ImplicationContext *) -> std::any override;
    auto visitParen(LTLParser::ParenContext *) -> std::any override;
    auto visitUntil(LTLParser::UntilContext *) -> std::any override;

    template <typename T>
    [[nodiscard]]
    auto visit(T *ctx) -> NodePtr {
        LTLVisitor::visit(ctx);
        return std::move(result);
    }

    [[nodiscard]]
    auto set(NodePtr node) -> std::any {
        result = std::move(node);
        return {};
    }

    NodePtr result;
    const TSGraph &graph;
};

auto Visitor::visitAtomic(LTLParser::AtomicContext *ctx) -> std::any {
    const auto name = ctx->getText();
    using enum AtomicNode::Type;
    if (name == "true") {
        return set(std::make_unique<AtomicNode>(0, True));
    } else if (name == "false") {
        return set(std::make_unique<AtomicNode>(0, False));
    } else {
        return set(std::make_unique<AtomicNode>(graph.map_atomic(name)));
    }
}

auto Visitor::visitParen(LTLParser::ParenContext *ctx) -> std::any {
    return LTLVisitor::visit(ctx->formula());
}

auto Visitor::visitProg(LTLParser::ProgContext *ctx) -> std::any {
    return LTLVisitor::visit(ctx->formula());
}

auto Visitor::visitNot(LTLParser::NotContext *ctx) -> std::any {
    return set(std::make_unique<NotNode>(visit(ctx->formula())));
}

auto Visitor::visitNext(LTLParser::NextContext *ctx) -> std::any {
    return set(std::make_unique<NextNode>(visit(ctx->formula())));
}

auto Visitor::visitEventually(LTLParser::EventuallyContext *ctx) -> std::any {
    return set(std::make_unique<EventualNode>(visit(ctx->formula())));
}

auto Visitor::visitAlways(LTLParser::AlwaysContext *ctx) -> std::any {
    return set(std::make_unique<AlwaysNode>(visit(ctx->formula())));
}

auto Visitor::visitConjunction(LTLParser::ConjunctionContext *ctx) -> std::any {
    return set(std::make_unique<ConjNode>(visit(ctx->lhs), visit(ctx->rhs)));
}

auto Visitor::visitDisjunction(LTLParser::DisjunctionContext *ctx) -> std::any {
    return set(std::make_unique<DisjNode>(visit(ctx->lhs), visit(ctx->rhs)));
}

auto Visitor::visitImplication(LTLParser::ImplicationContext *ctx) -> std::any {
    return set(std::make_unique<ImplNode>(visit(ctx->lhs), visit(ctx->rhs)));
}

auto Visitor::visitUntil(LTLParser::UntilContext *ctx) -> std::any {
    return set(std::make_unique<UntilNode>(visit(ctx->lhs), visit(ctx->rhs)));
}

auto recursive_check(const BaseNode *node) -> void {
    assume(node != nullptr);
    if (auto *n = dynamic_cast<const UnaryNode *>(node)) {
        recursive_check(n->child.get());
    } else if (auto *n = dynamic_cast<const BinaryNode *>(node)) {
        recursive_check(n->lhs.get());
        recursive_check(n->rhs.get());
    } else {
        assume(dynamic_cast<const AtomicNode *>(node) != nullptr);
    }
}

auto readLTL(std::istream &is, const TSGraph &graph) -> NodePtr {
    auto input  = antlr4::ANTLRInputStream(is);
    auto lexer  = LTLLexer(&input);
    auto tokens = antlr4::CommonTokenStream(&lexer);
    auto parser = LTLParser(&tokens);
    auto tree   = parser.prog();
    docheck(parser.getNumberOfSyntaxErrors() == 0, "Syntax error in LTL formula");
    auto visitor = Visitor(graph);
    auto root    = visitor.visit(tree);
    recursive_check(root.get());
    return root;
}

} // namespace

auto BaseNode::debug_print(std::ostream &os) const -> void {
    if (auto unary = is<UnaryNode>()) {
        os << "(";
        if (unary->is<NotNode>())
            os << "! ";
        if (unary->is<NextNode>())
            os << "X ";
        if (unary->is<AlwaysNode>())
            os << "G ";
        if (unary->is<EventualNode>())
            os << "F ";
        unary->child->debug_print(os);
        os << ")";
    } else if (auto binary = is<BinaryNode>()) {
        os << "(";
        binary->lhs->debug_print(os);
        if (binary->is<DisjNode>())
            os << " \\/ ";
        if (binary->is<UntilNode>())
            os << " U ";
        if (binary->is<ImplNode>())
            os << " -> ";
        if (binary->is<ConjNode>())
            os << " /\\ ";
        binary->rhs->debug_print(os);
        os << ")";
    } else if (auto atomic = is<AtomicNode>()) {
        os << atomic->index;
    }
}

auto TSGraph::map_atomic(std::string_view action) const -> std::size_t {
    auto it = atomic_rev_map.find(action);
    docheck(it != atomic_rev_map.end(), "Unknown atomic action: {}", action);
    return it->second;
}

auto LTLProgram::work(std::istream &gs, std::istream &ts, std::ostream &os) -> void {
    static constexpr auto readline = [](std::istream &is) {
        auto line = std::string{};
        docheck(std::getline(is, line), "expect more lines");
        return std::stringstream{std::move(line)};
    };

    const auto graph  = TSGraph::read(gs);
    auto num_test_all = std::size_t{};
    auto num_test_one = std::size_t{};
    readline(ts) >> num_test_all >> num_test_one;

    const auto view = TSView{graph};
    for ([[maybe_unused]] const auto _ : irange(num_test_all)) {
        auto ss      = readline(ts);
        auto formula = readLTL(ss, graph);
        os << static_cast<int>(verifyLTL(formula.get(), view)) << '\n';
    }

    auto set = bitset{graph.states()};

    for ([[maybe_unused]] const auto _ : irange(num_test_one)) {
        auto ss         = readline(ts);
        const auto view = [&] {
            auto num = std::size_t{};
            ss >> num;
            set[num]  = true;
            auto view = TSView{graph, set};
            set[num]  = false;
            return view;
        }();
        auto formula = readLTL(ss, graph);
        os << static_cast<int>(verifyLTL(formula.get(), view)) << '\n';
    }
}

} // namespace dark
