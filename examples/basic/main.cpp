/*
 *  mdf4-parser basic example.
 *  Reads an MDF4 measurement file's structure into protobuf, prints a summary.
 *
 *  Build:
 *    cd examples/basic
 *    cmake -B build
 *    cmake --build build
 *
 *  Run:
 *    ./build/mdf4_basic path/to/file.mf4
 *    ./build/mdf4_basic                    # empty document
 */

#include <iostream>
#include <string>

#include "mdf4/extract.h"

using namespace std;

int main(int argc, char* argv[]) {
    // extractFile is total — it never fails, it reports. An absent path yields
    // the empty document, so the example runs without a measurement file.
    const string path = argc >= 2 ? argv[1] : string();

    mdf4::File file = mdf4::extract::extractFile(path);

    cout << "MDF4: " << (path.empty() ? "(no file)" : path) << endl;
    cout << "Diagnostics: " << file.diagnostics_size() << endl;
    for (const auto& diag : file.diagnostics())
        cout << "  [" << mdf4::Severity_Name(diag.severity()) << "] "
             << diag.location() << ": " << diag.message() << endl;

    string binary;
    file.SerializeToString(&binary);
    cout << "Serialized: " << binary.size() << " bytes" << endl;

    return 0;
}
