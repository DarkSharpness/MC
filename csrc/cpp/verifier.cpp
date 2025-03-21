#include "LTL/automa.h"
#include "LTL/node.h"
#include "LTL/ts.h"
#include "utils/error.h"

namespace dark {

auto verifyLTL(BaseNode *node, const TSView &ts) -> bool {
    const auto GBNA = GNBA::build(node, ts.num_atomics);
    assume(false, "not implemented yet");
    return false;
}

} // namespace dark
