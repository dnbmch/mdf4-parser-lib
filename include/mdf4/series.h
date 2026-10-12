#pragma once

#include <cstdint>
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

// One array channel's element window: per sample its time and `elements`
// physical values in row-major element order, sample after sample.
struct ArraySeries {
    std::vector<double> time;
    std::vector<double> value;          // time.size() x elements
    std::uint64_t firstElement = 0;     // the clamped window
    std::uint64_t elements = 0;
};

} // namespace mdf4
