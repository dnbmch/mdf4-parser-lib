#pragma once

#include <cstdint>
#include <limits>
#include <memory>
#include <string>

#include "mdf4/mdf4.pb.h"
#include "mdf4/series.h"

namespace mdf4 {

// The outcome of one Reader::read. A default-constructed result is a failure,
// so nothing reports success without setting `ok`.
struct ReadResult {
    // On success `series` holds `time` and `value` of equal length, possibly
    // empty; on failure it is empty.
    bool ok = false;
    Series series;
    // Failure only. `location` is the walk path of the channel whose read
    // failed, in the form of Diagnostic.location, and is empty when the request
    // names no channel. `message` is static text, so a failure stays reportable
    // when nothing can be allocated.
    std::string location;
    const char* message = "not read";
};

// One opened MDF4 source. Construction opens the file and indexes its block
// graph once; metadata() and every read() describe that opened source for the
// reader's lifetime. The path is never reopened and the index never rebuilt;
// a relative path is resolved against the working directory at construction.
//
// Input problems never throw. An unopenable file or an unusable ID/HD root
// leaves its diagnostics in metadata() and ready() false; malformed sections
// and unsupported channels degrade inside metadata() and the reader stays
// ready. A valid file without data groups is ready.
//
// A reader is not thread safe: read() calls and destruction must not overlap,
// though ownership may move between threads. metadata() never changes, so it
// may be inspected on any thread while a read() runs.
class Reader {
public:
    explicit Reader(const std::string& path);
    ~Reader();
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    const File& metadata() const;
    bool ready() const;

    // Decodes one channel's samples plus its time master into physical doubles.
    // `group` and `channel` index File.groups. The window is clamped to the
    // channel's sample count; an empty window of a decodable channel succeeds.
    // Other channels of the group are never materialized.
    //
    // Fails for an unknown or non-decodable channel, an unready reader, a data
    // block that cannot be read or inflated, or a window too large to hold. A
    // source whose length or write time changed since opening fails the read
    // and needs a new Reader; those checks do not detect every same-size edit.
    ReadResult read(uint32_t group, uint32_t channel, uint64_t firstSample = 0,
                    uint64_t sampleCount = std::numeric_limits<uint64_t>::max()) noexcept;

private:
    struct Source;
    std::unique_ptr<Source> _source;
};

} // namespace mdf4
