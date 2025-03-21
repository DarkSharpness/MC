#pragma once
#include "node.h"

namespace dark {

struct UnaryNode : BaseNode {
    NodePtr child;
    UnaryNode(NodePtr child) : child(std::move(child)) {}
};

struct BinaryNode : BaseNode {
    NodePtr lhs;
    NodePtr rhs;
    BinaryNode(NodePtr l, NodePtr r) : lhs(std::move(l)), rhs(std::move(r)) {}
};

struct AtomicNode final : BaseNode {
    std::size_t index;
    enum class Type { Atomic, True, False } type;
    AtomicNode(std::size_t index, Type tp = Type::Atomic) : index(index), type(tp) {}
};

struct NotNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct NextNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct AlwaysNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct EventualNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct ConjNode final : BinaryNode {
    using BinaryNode::BinaryNode;
};

struct DisjNode final : BinaryNode {
    using BinaryNode::BinaryNode;
};

struct ImplNode final : BinaryNode {
    using BinaryNode::BinaryNode;
};

struct UntilNode final : BinaryNode {
    using BinaryNode::BinaryNode;
};

} // namespace dark
