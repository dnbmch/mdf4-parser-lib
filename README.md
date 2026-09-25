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
every read describe that opened file. A reader serves one thread at a time; its
metadata may be inspected from any thread.

```cpp
#include "mdf4/reader.h"

// Structure only — block headers, no sample data read.
mdf4::Reader reader("path/to/file.mf4");
const mdf4::File& file = reader.metadata();

for (const auto& diag : file.diagnostics()) {
    // Anything the reader had to guess, default, or drop.
}

// Samples on demand, converted to physical doubles against the time master.
mdf4::ReadResult samples = reader.read(0, 1);
if (samples.ok) {
    // samples.series.time / samples.series.value, equal length, possibly empty
} else {
    // samples.message, and samples.location naming the channel
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
