#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include "mdf4/mdf4.pb.h"
#include "mdf4/series.h"
#include "mdf4/common.pb.h"

namespace mdf4::extract {

// The reader surface. Both calls are stateless — the file is re-opened by path,
// so they are usable from worker threads with no shared handle, and neither
// throws: a malformed file degrades into diagnostics, an undecodable channel
// into an empty Series.

// Walk the block graph into the typed metadata document. Reads block headers
// only; open cost is proportional to structure, not file size.
mdf4::File extractFile(const std::string& path);

// Decode one channel's samples plus its time master into physical doubles.
// `group` and `channel` index the same flattened walk as File.groups. The
// window is clamped to the channel's sample count, so `time` and `value` always
// come back the same length. Other channels of the group are never materialized.
Series decodeChannel(const std::string& path, uint32_t group, uint32_t channel,
                     uint64_t firstSample = 0,
                     uint64_t sampleCount = std::numeric_limits<uint64_t>::max());

} // namespace mdf4::extract
