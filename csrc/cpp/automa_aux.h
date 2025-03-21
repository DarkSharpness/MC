// should only be included once
#ifndef AUTOMA_AUX_H
#define AUTOMA_AUX_H
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#else
#error "automa_aux.h" included twice
#endif

namespace dark {

static_assert(sizeof(std::size_t) == sizeof(std::int64_t), "Size mismatch");

// formulat indexx
struct FormulaIndex {
public:
    explicit constexpr FormulaIndex(std::int64_t i) : index(i) {}

    constexpr auto operator~() const -> FormulaIndex {
        return FormulaIndex(~index);
    }

    auto is_negation() const -> bool {
        return index < 0;
    }

    auto operator==(const FormulaIndex &other) const -> bool = default;

    auto raw() const -> std::int64_t {
        return index;
    }

    auto original() const -> std::size_t {
        return is_negation() ? ~index : index;
    }

    static const FormulaIndex True;
    static const FormulaIndex False;

private:
    std::int64_t index;
};

using fid = FormulaIndex;

constexpr const fid fid::True  = fid(std::numeric_limits<std::int64_t>::max());
constexpr const fid fid::False = ~fid::True;

struct Formula {
public:
    enum class op : std::uint64_t { ATOMIC, NEXT, CONJ, UNTIL };

    template <op Type>
    static auto atomic(fid x) -> Formula {
        static_assert(Type == op::ATOMIC, "Invalid atomic operator");
        return Formula{Type, x};
    }

    template <op Type>
    static auto unary(fid x) -> Formula {
        static_assert(Type == op::NEXT, "Invalid unary operator");
        return Formula{Type, x};
    }

    template <op Type>
    static auto binary(fid l, fid r) -> Formula {
        static_assert(Type == op::CONJ || Type == op::UNTIL, "Invalid binary operator");
        if constexpr (Type == op::CONJ) {
            if (l.raw() < r.raw()) {
                return Formula{Type, l, r};
            } else {
                return Formula{Type, r, l};
            }
        } else {
            return Formula{Type, l, r};
        }
    }

    auto operator==(const Formula &other) const -> bool = default;

    struct Hash {
        auto operator()(const Formula &s) const -> std::size_t {
            return s.hash();
        }
    };

    auto is_atomic() const -> bool {
        return type == op::ATOMIC;
    }

    auto is_unary() const -> bool {
        return type == op::NEXT;
    }

    auto is_conj() const -> bool {
        return type == op::CONJ;
    }

    auto is_until() const -> bool {
        return type == op::UNTIL;
    }

    auto is_binary() const -> bool {
        return type == op::CONJ || type == op::UNTIL;
    }

    auto is_uncertain() const -> bool {
        return type == op::NEXT || type == op::UNTIL;
    }

    auto operator[](bool right) const -> fid {
        return right ? rhs : lhs;
    }

private:
    auto hash() const -> std::size_t {
        using u64     = std::uint64_t;
        const u64 lhs = this->lhs.raw();
        const u64 rhs = this->rhs.raw();
        return lhs ^ std::rotl(rhs, 32) ^ (static_cast<u64>(type) << 62);
    }

    explicit Formula(op t, fid l, fid r = fid(0)) : type(t), lhs(l), rhs(r) {}

    op type;
    fid lhs;
    fid rhs; // optional
};

} // namespace dark
