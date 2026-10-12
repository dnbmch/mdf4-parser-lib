# mdf4-parser-lib

Prebuilt static library, public headers, and protobuf schema for the **mdf4-parser** — a C++17 library that reads ASAM MDF 4.2 measurement files into Protocol Buffer metadata plus a direct C++ sample-decode API.

**Author:** Danube Mechatronics Kft.

## Downloads

Prebuilt static libraries are available on the
[Releases](https://github.com/dnbmch/mdf4-parser-lib/releases) page:

| Artifact | Platform |
|----------|----------|
| `mdf4parser-x86_64-windows-mingw` | Windows MinGW GCC (.a) |
| `mdf4parser-x86_64-linux-gnu` | Linux x86_64 (.a) |
| `mdf4parser-aarch64-linux-gnu` | Linux ARM64 (.a) |
| `mdf4parser-x86_64-windows-msvc` | Windows MSVC (.lib) |

## Quick Start

Each platform archive contains a complete install prefix: matching public and
protobuf-generated headers, the static library, schemas, CMake package files and
`share/mdf4parser/build-info.json`. Set `CMAKE_PREFIX_PATH` to the extracted prefix;
CMake resolves `mdf4parser::mdf4parser` through `find_package(mdf4parser CONFIG REQUIRED)`.
Use a compatible compiler/runtime and the producer's exact protobuf version. Dependency
libraries are supplied separately by your toolchain. Do not regenerate C++ headers
against a prebuilt binary. Historical split archives do not satisfy this contract;
choose a complete package from a deliberate future release or a local producer install.

Package CI runs when repository variable `PARSER_PACKAGE_TAG` names an existing
complete-package release, and supports manual dispatch. No tag is selected by default.


```bash
# 1. Clone this repo
git clone https://github.com/dnbmch/mdf4-parser-lib.git
cd mdf4-parser-lib

# 2. Download and extract the library for your platform
#    (from the Releases page, extract into package/)
TAG=... # Select an existing complete-package release tag.
mkdir -p package
tar xzf mdf4parser-x86_64-linux-gnu-${TAG}.tar.gz -C package/

# 3. Build the examples
cmake -B build -DCMAKE_PREFIX_PATH=/absolute/path/to/package
cmake --build build

# 4. Run
./build/mdf4_basic path/to/file.mf4
```

## Contents

| Directory | Description |
|-----------|-------------|
| `include/` | Public C++ headers (`reader.h`, `extract.h`, `series.h`) |
| `proto/` | Protobuf schema files (`.proto`) for multi-language binding generation |
| `examples/` | Example applications (basic summary, JSON export) |

## Integration

The surface is a hybrid: protobuf for the metadata document, plain C++ for bulk
samples. One `mdf4::Reader` opens and indexes the file once; its metadata and
every operation describe that opened file. A reader serves one operation at a
time; its metadata, opening outcome and time axes may be inspected from any
thread. `mdf4::Limits`, fixed at construction, bounds the opening, each
operation's scratch and a read's output; a refusal is an explicit outcome.

```cpp
#include "mdf4/reader.h"

// Structure only — block headers, no sample data read.
mdf4::Reader reader("path/to/file.mf4");
const mdf4::File& file = reader.metadata();
if (!reader.ready()) {
    // reader.openOutcome().message; the diagnostics say what was found
}

for (const auto& diag : file.diagnostics()) {
    // Anything the reader had to guess, default, or drop.
}

// Samples on demand, converted to physical doubles against the time master
// that reader.axis(0) names.
mdf4::ReadResult samples = reader.read(0, 1);
if (samples.outcome.status == mdf4::Status::Ok) {
    // samples.series.time / samples.series.value, equal length, possibly empty;
    // samples.coverage is Short when the source ended before the request
} else {
    // samples.outcome.status and .message, .location naming the channel
}

// A whole channel of any length, in bounded chunks with absolute indices.
reader.scan(0, 1, 0, UINT64_MAX, {}, [](const mdf4::SampleChunk& chunk) {
    // chunk.firstSample, chunk.time[i], chunk.value[i] for i < chunk.size
    return mdf4::Visit::Continue;
});

// A string channel (Channel.textual) delivers UTF-8 texts instead, through
// readText() and scanText(); an invalid sample's text is empty.
mdf4::ReadTextResult texts = reader.readText(0, 2);
if (texts.outcome.status == mdf4::Status::Ok) {
    // texts.series.time / texts.series.text, equal length
}

// An array channel (Channel.array) delivers each sample's whole array through
// readArray() and scanArray(), its elements in row-major order; an element
// window selects some of them.
if (file.groups(0).channels(3).has_array()) {
    // Sample 5's array: series.elements values at series.time[0].
    mdf4::ReadArrayResult map = reader.readArray(0, 3, 5, 1);
    // Element 7 over every sample: one value per sample.
    mdf4::ReadArrayResult element = reader.readArray(0, 3, 0, UINT64_MAX, {7, 1});
}
```

`mdf4::extract::extractFile(path)` (`mdf4/extract.h`) returns the metadata
document alone.

## Build Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- Protocol Buffers (protobuf) runtime library
- zlib — the reader inflates `##DZ` compressed data blocks, so the static
  library carries zlib symbols the consumer must resolve

## License

Dual licensed: GPL-2.0 or Commercial. See [LICENSE.md](LICENSE.md).
