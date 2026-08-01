# mdf4-parser-lib

Prebuilt static library, public headers, and protobuf schema for the **mdf4-parser** — a C++17 library that reads ASAM MDF 4.2 measurement files into Protocol Buffer metadata plus a direct C++ sample-decode API.

**Author:** Danube Mechatronics Kft.

## Downloads

Prebuilt static libraries are available on the [Releases](https://github.com/dnbmch/mdf4-parser-lib/releases) page:

| Artifact | Platform |
|----------|----------|
| `mdf4parser-x86_64-windows-mingw` | Windows MinGW GCC (.a) |
| `mdf4parser-x86_64-linux-gnu` | Linux x86_64 (.a) |
| `mdf4parser-aarch64-linux-gnu` | Linux ARM64 (.a) |
| `mdf4parser-x86_64-windows-msvc` | Windows MSVC (.lib) |
| `mdf4parser-headers` | Public headers and proto files |

## Quick Start

```bash
# 1. Clone this repo
git clone https://github.com/dnbmch/mdf4-parser-lib.git
cd mdf4-parser-lib

# 2. Download and extract the prebuilt library for your platform
#    (from the Releases page, extract into lib/)
mkdir -p lib
tar xzf mdf4parser-x86_64-linux-gnu-v0.1.0.tar.gz -C lib/

# 3. Build the examples
cmake -B build -DMDF4_LIB_DIR=lib
cmake --build build

# 4. Run
./build/mdf4_basic path/to/file.mf4
```

## Contents

| Directory | Description |
|-----------|-------------|
| `include/` | Public C++ headers (`extract.h`, `series.h`) |
| `proto/` | Protobuf schema files (`.proto`) for multi-language binding generation |
| `examples/` | Example applications (basic summary, JSON export) |

## Integration

The surface is a hybrid: protobuf for the metadata document, plain C++ for bulk
samples. Both calls are stateless — the file is re-opened by path, so they are
usable from worker threads with no shared handle.

```cpp
#include "mdf4/extract.h"

// Structure only — block headers, no sample data read.
mdf4::File file = mdf4::extract::extractFile("path/to/file.mf4");

for (const auto& diag : file.diagnostics()) {
    // Anything the reader had to guess, default, or drop.
}

// Samples on demand, converted to physical doubles against the time master.
mdf4::Series series = mdf4::extract::decodeChannel("path/to/file.mf4", 0, 1);
```

## Build Requirements

- C++17 compiler (GCC, Clang, or MSVC)
- Protocol Buffers (protobuf) runtime library
- zlib — the reader inflates `##DZ` compressed data blocks, so the static
  library carries zlib symbols the consumer must resolve

## License

Dual licensed: GPL-2.0 or Commercial. See [LICENSE.md](LICENSE.md).
