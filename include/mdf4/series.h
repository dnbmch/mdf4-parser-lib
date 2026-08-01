#pragma once

#include <vector>

namespace mdf4 {

// One decoded channel: physical values against their time master. Bulk sample
// data deliberately stays out of the protobuf document — only the metadata
// document round-trips through proto.
struct Series {
    std::vector<double> time;
    std::vector<double> value;
};

} // namespace mdf4
