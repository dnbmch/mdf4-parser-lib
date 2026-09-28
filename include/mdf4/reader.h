#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>

#include "mdf4/mdf4.pb.h"
#include "mdf4/series.h"

namespace mdf4 {

// Finite allowances of one reader, fixed at construction. Tests pass small
// values to reach refusal without large allocations. A zero sliceBytes or
// chunkSamples counts as one.
struct Limits {
    // Opening: the index, its protobuf projection and the walk's live working
    // storage together, at their peak, as logical charges (requested sizes and
    // capacities). Refused before the exceeding allocation.
    std::uint64_t openingBytes = std::uint64_t{1} << 30;
    // One operation's decode scratch: record buffers, raw input, inflate state
    // and output slices, a transposed fragment and the visitor's chunk arrays,
    // for the target and any remote master stream together.
    std::uint64_t scratchBytes = std::uint64_t{64} << 20;
    // read(): owned output samples, a time and a value double each.
    std::uint64_t readSamples = std::uint64_t{4} << 20;
    // Work granularity: plain data Bytes per unit of reading, inflating,
    // un-transposing, cutting or skipping records, one cancellation and progress
    // checkpoint each; samples per visitor chunk.
    std::uint64_t sliceBytes = std::uint64_t{1} << 20;
    std::uint64_t chunkSamples = std::uint64_t{1} << 16;
};

enum class Status : std::uint8_t {
    Ok,             // completed; ScanResult::coverage says how much was found
    Cancelled,      // Control::cancel or the visitor stopped it
    ResourceLimit,  // an allowance refused a step before allocating for it
    SourceChanged,  // the file changed since opening; open a new Reader
    Error,          // unreadable or undecodable data, unknown channel, unready reader
};

enum class Resource : std::uint8_t { None, Opening, Scratch, Output, Consumer };

// A default-constructed outcome is an error: nothing reports success unsaid.
struct Outcome {
    Status status = Status::Error;
    std::string location;               // Diagnostic.location form; empty when none
    const char* message = "not run";    // static text, reportable without memory
    Resource resource = Resource::None; // ResourceLimit only
    std::uint64_t required = 0;         // Bytes (samples for Output); 0 if unknown
    std::uint64_t limit = 0;            // the refusing allowance, same unit
};

enum class Coverage : std::uint8_t {
    Complete,  // every sample of the clamped request was delivered
    Short,     // Ok, but the source ended first; Outcome::message says why
};

// Borrowed equal-length arrays, valid only during the visitor call.
struct SampleChunk {
    std::uint64_t firstSample = 0;  // absolute occurrence index of element 0
    const double* time = nullptr;   // the group's axis, see Reader::axis
    const double* value = nullptr;
    std::size_t size = 0;
};

enum class Visit : std::uint8_t { Continue, Cancel, ResourceLimit };
using Visitor = std::function<Visit(const SampleChunk&)>;

// The caller's control of one operation, valid for the operation's duration.
struct Control {
    const std::atomic<bool>* cancel = nullptr;  // set from any thread
    // On the operation's thread at checkpoints: plain data Bytes traversed and
    // the most it will traverse, target and master streams together. `total` is
    // fixed at the start; `done` never decreases and may end below `total`.
    std::function<void(std::uint64_t done, std::uint64_t total)> progress;
};

struct ScanResult {
    Outcome outcome;
    Coverage coverage = Coverage::Complete; // meaningful when status is Ok
    std::uint64_t firstSample = 0;   // request start, clamped to the cycle count
    std::uint64_t sampleCount = 0;   // request length after clamping
    std::uint64_t samplesRead = 0;   // delivered in order from firstSample
    std::uint64_t bytesRead = 0;     // stored Bytes this operation read from the file
    std::uint64_t bytesInflated = 0; // Bytes this operation's inflation produced
};

// Only an Ok outcome carries samples (samplesRead, possibly none); `time` and
// `value` then have samplesRead elements each.
struct ReadResult : ScanResult {
    Series series;
};

enum class AxisKind : std::uint8_t {
    None,         // no such group, or the reader is not ready
    SampleIndex,  // no eligible time master: time is the absolute sample index
    Master,       // the time master at (group, channel), local or remote
};

// For SampleIndex, `group` is the queried group itself and `channel` is 0.
struct Axis {
    AxisKind kind = AxisKind::None;
    std::uint32_t group = 0;
    std::uint32_t channel = 0;
};

// One opened MDF4 source. Construction opens the file and indexes its block
// graph once; metadata() and every operation describe that opened source for
// the reader's lifetime. The path is never reopened and the index never
// rebuilt; a relative path is resolved against the working directory at
// construction. `cancel`, when set during construction, stops the opening.
//
// Input problems never throw. openOutcome() is Ok exactly when ready(). An
// unopenable file, a file that is not MDF, an unusable HD block or an empty path
// is an Error, inspectable through metadata() diagnostics. An opening refused by
// Limits::openingBytes (ResourceLimit, Resource::Opening), cancelled or failed
// leaves no partial structure: metadata() holds one DROPPED diagnostic naming
// it. Malformed sections and unsupported channels degrade inside an Ok opening.
// A valid file without data groups is ready.
//
// `group` and `channel` index File.groups. Every operation clamps its request
// to the channel's sample count; an empty clamped request of a decodable
// channel is Ok and Complete with zero samples. An unready reader, an unknown
// group or channel, a channel outside the read set or unreadable data is an
// Error; a source whose length or write time changed since opening, checked
// before and after the traversal, is SourceChanged and needs a new Reader. Those
// checks do not detect every same-size edit.
class Reader {
public:
    explicit Reader(const std::string& path, const Limits& limits = Limits{},
                    const std::atomic<bool>* cancel = nullptr);
    ~Reader();
    Reader(const Reader&) = delete;
    Reader& operator=(const Reader&) = delete;

    // Fixed at construction; readable on any thread while an operation runs.
    const Outcome& openOutcome() const;
    bool ready() const;
    const File& metadata() const;
    Axis axis(std::uint32_t group) const;

    // One operation at a time per reader.
    //
    // scan() delivers the clamped request in order to `visitor` as nonempty,
    // contiguous chunks of at most chunkSamples samples; a consumer assumes no
    // other size. The visitor runs synchronously on the calling thread and
    // must not reenter the reader. Chunks are provisional: install nothing
    // before scan() returns Ok. Visit::Cancel yields Cancelled,
    // Visit::ResourceLimit yields ResourceLimit with Resource::Consumer.
    ScanResult scan(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample,
                    std::uint64_t sampleCount, const Control& control,
                    const Visitor& visitor) noexcept;
    // read() admits the clamped request against Limits::readSamples before it
    // reserves the series, then collects the same traversal.
    ReadResult read(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample = 0,
                    std::uint64_t sampleCount = std::numeric_limits<std::uint64_t>::max(),
                    const Control& control = {}) noexcept;

private:
    struct Source;
    std::unique_ptr<Source> _source;
};

} // namespace mdf4
