#pragma once

#include <cstdint>
#include <string>

#include "mdf4/mdf4.pb.h"
#include "mdf4/series.h"
#include "mdf4/common.pb.h"

namespace mdf4::extract {

// The reader surface. Both calls are stateless — the file is re-opened by path,
// so they are usable from worker threads with no shared handle. The block-graph
// walk and the record decode behind them land with the reader core.

// Walk the block graph into the typed metadata document. Reads block headers
// only; open cost is proportional to structure, not file size.
mdf4::File extractFile(const std::string& path);

// Decode one channel's samples plus its time master into physical doubles.
// Other channels of the group are never materialized.
Series decodeChannel(const std::string& path, uint32_t group, uint32_t channel);

} // namespace mdf4::extract
