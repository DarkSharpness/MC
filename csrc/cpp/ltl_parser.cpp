#include "LTL/input.h"
#include "LTL/node.h"
#include "LTL/node_impl.h"
#include "LTLLexer.h"
#include "LTLParser.h"
#include "LTLVisitor.h"
#include "utils/error.h"
#include <ANTLRInputStream.h>
#include <CommonTokenStream.h>
#include <any>
#include <memory>

namespace dark {

namespace {

struct Visitor final : LTLVisitor {
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

    NodePtr result;

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
};

auto Visitor::visitAtomic(LTLParser::AtomicContext *ctx) -> std::any {
    return set(std::make_unique<AtomicNode>(ctx->getText()));
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

} // namespace

auto LTLFormula::read(std::istream &is) -> LTLFormula {
    auto input   = antlr4::ANTLRInputStream(is);
    auto lexer   = LTLLexer(&input);
    auto tokens  = antlr4::CommonTokenStream(&lexer);
    auto parser  = LTLParser(&tokens);
    auto tree    = parser.prog();
    auto visitor = Visitor();
    auto root    = visitor.visit(tree);
    recursive_check(root.get());
    return LTLFormula(std::move(root));
}

} // namespace dark
