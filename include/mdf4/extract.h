#pragma once

#include <string>

#include "mdf4/mdf4.pb.h"
#include "mdf4/common.pb.h"

namespace mdf4::extract {

// Walk the block graph into the typed metadata document: the metadata of a
// temporary mdf4::Reader (mdf4/reader.h). Reads block headers and descriptor
// payloads only, so open cost is proportional to structure, not file size.
// Never throws; a malformed file degrades into diagnostics. Samples are read
// through a retained Reader.
mdf4::File extractFile(const std::string& path) noexcept;

} // namespace mdf4::extract
