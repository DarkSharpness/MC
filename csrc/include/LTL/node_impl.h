#pragma once
#include "node.h"
#include <string>

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

struct NotNode final : UnaryNode {
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

struct NextNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct AlwaysNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct EventualNode final : UnaryNode {
    using UnaryNode::UnaryNode;
};

struct UntilNode final : BinaryNode {
    using BinaryNode::BinaryNode;
};

struct AtomicNode final : BaseNode {
    std::string name;
    AtomicNode(std::string name) : name(std::move(name)) {}
};

} // namespace dark
