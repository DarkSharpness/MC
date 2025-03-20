#pragma once
#include <memory>

namespace dark {

struct TSView;

struct BaseNode {
    virtual ~BaseNode() = default;
};

using NodePtr = std::unique_ptr<BaseNode>;

[[nodiscard]]
auto verifyLTL(BaseNode *, const TSView &ts) -> bool;

} // namespace dark
