#pragma once
#include <memory>

namespace dark {

struct BaseNode {
    virtual ~BaseNode() = default;
};

using NodePtr = std::unique_ptr<BaseNode>;

} // namespace dark
