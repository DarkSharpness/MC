#pragma once
#include <iosfwd>
#include <memory>

namespace dark {

struct TSView;

struct BaseNode {
    virtual ~BaseNode() = default;
    template <typename T>
    auto is() -> T * {
        return dynamic_cast<T *>(this);
    }
    template <typename T>
    auto is() const -> const T * {
        return dynamic_cast<const T *>(this);
    }
    // for internal debug use
    auto debug_print(std::ostream &) const -> void;
};

using NodePtr = std::unique_ptr<BaseNode>;

[[nodiscard]]
auto verifyLTL(BaseNode *, const TSView &ts) -> bool;

} // namespace dark
