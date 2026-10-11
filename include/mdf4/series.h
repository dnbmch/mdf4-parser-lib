#pragma once

#include <string>
#include <vector>

namespace mdf4 {

// One decoded channel: physical values against their time master. Bulk sample
// data deliberately stays out of the protobuf document — only the metadata
// document round-trips through proto.
struct Series {
    std::vector<double> time;
    std::vector<double> value;
};

// One decoded text channel: UTF-8 texts against their time master.
struct TextSeries {
    std::vector<double> time;
    std::vector<std::string> text;
};

} // namespace mdf4
