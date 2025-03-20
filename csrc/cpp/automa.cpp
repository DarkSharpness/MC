#include "LTL/automa.h"
#include "LTL/node.h"
#include <cstddef>
#include <unordered_map>
#include <vector>

namespace dark {

namespace {

struct Structure {
public:
    enum class op : std::size_t {
        NOT,
        NEXT,
        CONJ,
        UNTIL,
        ATOMIC,
    };

    template <op T>
    static auto atomic(std::size_t) -> Structure {
        static_assert(T == op::ATOMIC, "Invalid atomic operator");
        return Structure{T, 0};
    }

    template <op T>
    static auto unary(std::size_t x) -> Structure {
        static_assert(T == op::NOT || T == op::NEXT, "Invalid unary operator");
        return Structure{T, x};
    }

    template <op T>
    static auto binary(std::size_t l, std::size_t r) -> Structure {
        static_assert(T == op::CONJ || T == op::UNTIL, "Invalid binary operator");
        return Structure{T, l, r};
    }

    auto operator==(const Structure &other) const -> bool = default;

private:
    Structure(op t, std::size_t l, std::size_t r = 0) : type{t}, lhs{l}, rhs{r} {}
    op type;
    std::size_t lhs;
    std::size_t rhs; // optional
};

struct SubFormulaCollection {
    // map formula -> index
    // 0 ~ x is the index of the formula
    // -1 ~ ~x is the index of the negation of the formula
    std::unordered_map<BaseNode *, std::size_t> mapping;
};

auto collect_subformula(BaseNode *ptr) -> std::vector<BaseNode *> {}

} // namespace

// Transform an LTL formula into a GNBA
auto GNBA::build(BaseNode *ptr, std::size_t num_triggers) -> GNBA {
    auto result         = GNBA{};
    result.num_triggers = num_triggers;

    return result;
}

} // namespace dark
