#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <string_view>

#include "mdf4/mdf4.pb.h"
#include "mdf4/series.h"

namespace mdf4 {

// Finite allowances of one reader, fixed at construction. Tests pass small
// values to reach refusal without large allocations. A zero sliceBytes or
// chunkValues counts as one.
struct Limits {
    // Opening: the index, its protobuf projection and the walk's live working
    // storage together, at their peak, as logical charges (requested sizes and
    // capacities). Refused before the exceeding allocation.
    std::uint64_t openingBytes = std::uint64_t{1} << 30;
    // One operation's decode scratch: record buffers, raw input, inflate state
    // and output slices, a transposed fragment and the visitor's chunk arrays,
    // for the target, any remote master stream and a VLSD channel's signal
    // stream together, and a text operation's arenas.
    std::uint64_t scratchBytes = std::uint64_t{64} << 20;
    // read(), readText() and readArray(): owned output values beside a time per
    // sample: a number or a text per sample, and for readArray() a number per
    // element of its window.
    std::uint64_t readValues = std::uint64_t{4} << 20;
    // Work granularity: plain data Bytes per unit of reading, inflating,
    // un-transposing, cutting or skipping records, one cancellation and progress
    // checkpoint each; values per visitor chunk, in whole samples, at least one.
    std::uint64_t sliceBytes = std::uint64_t{1} << 20;
    std::uint64_t chunkValues = std::uint64_t{1} << 16;
    // One text operation's arena, within scratchBytes: the UTF-8 texts of the
    // chunk being filled. A text conversion stages each input text in a second
    // arena of this size.
    std::uint64_t textBytes = std::uint64_t{16} << 20;
    // readText(): owned text Bytes, the samples' UTF-8 lengths together.
    std::uint64_t readTextBytes = std::uint64_t{256} << 20;
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
    // Bytes, except values for a Limits::readValues refusal, saturating at
    // 2^64 - 1; 0 if unknown.
    std::uint64_t required = 0;
    std::uint64_t limit = 0;            // the refusing allowance, same unit
};

enum class Coverage : std::uint8_t {
    Complete,  // every sample of the clamped request was delivered
    Short,     // Ok, but the source ended first; Outcome::message says why
};

// Borrowed equal-length arrays, valid only during the visitor call.
struct SampleChunk {
    std::uint64_t firstSample = 0;  // absolute sample index of time[0] and value[0]
    const double* time = nullptr;   // the group's axis, see Reader::axis
    const double* value = nullptr;
    std::size_t size = 0;
};

enum class Visit : std::uint8_t { Continue, Cancel, ResourceLimit };
using Visitor = std::function<Visit(const SampleChunk&)>;

// An array operation's element window: [first, first + count) of the element
// index k, row-major over the concatenated dimensions N*(1...D*) (ASAM MDF 4.2
// §4.20); for a single CABLOCK this is the spec's line-oriented k (§4.19).
// Clamped to ChannelArray.element_count as samples are to the sample count.
struct ElementWindow {
    std::uint64_t first = 0;
    std::uint64_t count = std::numeric_limits<std::uint64_t>::max();
};

// Borrowed arrays, valid only during the visitor call: `size` samples, a time
// each and `elements` values each, sample after sample; a sample's values are
// its window's elements in row-major order whatever the stored layout.
struct ArrayChunk {
    std::uint64_t firstSample = 0;   // absolute sample index of time[0]
    const double* time = nullptr;    // the group's axis, see Reader::axis
    const double* value = nullptr;   // size x elements
    std::size_t size = 0;            // samples
    std::uint64_t firstElement = 0;  // k of each sample's first value
    std::uint64_t elements = 0;      // values per sample: the clamped window
};

using ArrayVisitor = std::function<Visit(const ArrayChunk&)>;

// Borrowed equal-length arrays, valid only during the visitor call; text is
// UTF-8 (a UTF-8 source passes unvalidated), an invalid sample's is empty.
struct TextChunk {
    std::uint64_t firstSample = 0;          // absolute sample index of time[0] and text[0]
    const double* time = nullptr;           // the group's axis, see Reader::axis
    const std::string_view* text = nullptr;
    std::size_t size = 0;
};

using TextVisitor = std::function<Visit(const TextChunk&)>;

// The caller's control of one operation, valid for the operation's duration.
struct Control {
    const std::atomic<bool>* cancel = nullptr;  // set from any thread
    // On the operation's thread at checkpoints: plain data Bytes traversed and
    // the most it will traverse, target, master and signal streams together.
    // `total` is fixed at the start; `done` never decreases, never passes
    // `total` and may end below it.
    std::function<void(std::uint64_t done, std::uint64_t total)> progress;
};

struct ScanResult {
    Outcome outcome;
    Coverage coverage = Coverage::Complete; // meaningful when status is Ok
    std::uint64_t firstSample = 0;   // request start, clamped to the cycle count
    std::uint64_t sampleCount = 0;   // request length after clamping
    std::uint64_t samplesRead = 0;   // delivered in order from firstSample
    std::uint64_t invalidValues = 0;  // of the values delivered, the invalid ones: NaN or an empty text
    std::uint64_t bytesRead = 0;     // stored Bytes this operation read from the file
    std::uint64_t bytesInflated = 0; // Bytes this operation's inflation produced
};

// Only an Ok outcome carries samples (samplesRead, possibly none); `time` and
// `value` then have samplesRead elements each.
struct ReadResult : ScanResult {
    Series series;
};

// readText()'s counterpart: only an Ok outcome carries texts, samplesRead each.
struct ReadTextResult : ScanResult {
    TextSeries series;
};

// readArray()'s counterpart: only an Ok outcome carries values; `time` then has
// samplesRead entries and `value` samplesRead x series.elements.
struct ReadArrayResult : ScanResult {
    ArraySeries series;
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
// channel is Ok and Complete with zero samples. A textual channel
// (Channel.textual) delivers UTF-8 texts through scanText() and readText(), an
// array channel (Channel.array) its elements through scanArray() and
// readArray(), every other decodable channel values through scan() and read();
// each pair refuses the other kinds with an Error. A value the file marks
// invalid (its invalidation bit set in row storage — an array element's own
// bit, cn_inval_bit_pos plus its offset — or the channel's all-values-invalid
// flag) is delivered at its time as NaN, or as an empty text, and counted in
// invalidValues, and so is an MLSD sample whose size channel's sample is
// marked invalid; column storage's invalidation bits are not evaluated. An
// unready reader, an unknown group or channel, a channel outside the read set
// or unreadable data is an Error; a source whose length or write time changed
// since opening, checked before and after the traversal, is SourceChanged and
// needs a new Reader. Those checks do not detect every same-size edit.
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
    // contiguous chunks of at most chunkValues samples; a consumer assumes no
    // other size. The visitor runs synchronously on the calling thread and
    // must not reenter the reader. Chunks are provisional: install nothing
    // before scan() returns Ok. Visit::Cancel yields Cancelled,
    // Visit::ResourceLimit yields ResourceLimit with Resource::Consumer.
    ScanResult scan(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample,
                    std::uint64_t sampleCount, const Control& control,
                    const Visitor& visitor) noexcept;
    // read() admits the clamped request against Limits::readValues before it
    // reserves the series, then collects the same traversal.
    ReadResult read(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample = 0,
                    std::uint64_t sampleCount = std::numeric_limits<std::uint64_t>::max(),
                    const Control& control = {}) noexcept;

    // scanText() is scan() for a textual channel, except that a chunk also
    // closes early when its next text would not fit Limits::textBytes, so it
    // may hold fewer than chunkValues samples. A single text larger than that
    // arena is a ResourceLimit (Resource::Scratch).
    ScanResult scanText(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample,
                        std::uint64_t sampleCount, const Control& control,
                        const TextVisitor& visitor) noexcept;
    // readText() admits the clamped request against Limits::readValues, then
    // collects the same traversal and refuses the sample whose text would pass
    // Limits::readTextBytes (Resource::Output, in Bytes), discarding the texts.
    ReadTextResult readText(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample = 0,
                            std::uint64_t sampleCount = std::numeric_limits<std::uint64_t>::max(),
                            const Control& control = {}) noexcept;

    // scanArray() is scan() for an array channel: each sample delivers the
    // values of `elements`, the window clamped to Channel.array.element_count;
    // an empty clamped window is an empty request. A chunk holds at most
    // max(1, chunkValues / elements) samples; a consumer assumes no other size.
    ScanResult scanArray(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample,
                         std::uint64_t sampleCount, const ElementWindow& elements,
                         const Control& control, const ArrayVisitor& visitor) noexcept;
    // readArray() admits the clamped samples x elements against
    // Limits::readValues before it reserves the series, then collects the same
    // traversal.
    ReadArrayResult readArray(std::uint32_t group, std::uint32_t channel, std::uint64_t firstSample = 0,
                              std::uint64_t sampleCount = std::numeric_limits<std::uint64_t>::max(),
                              const ElementWindow& elements = {}, const Control& control = {}) noexcept;

private:
    struct Source;
    std::unique_ptr<Source> _source;
};

} // namespace mdf4
