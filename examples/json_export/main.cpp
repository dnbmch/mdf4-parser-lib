/*
 *  mdf4-parser JSON export example.
 *  Reads an MDF4 measurement file's structure into protobuf, serializes to JSON.
 *  The JSON output can be consumed by Python, web UIs, databases, etc.
 *
 *  Build:
 *    cd examples/json_export
 *    cmake -B build
 *    cmake --build build
 *
 *  Run:
 *    ./build/mdf4_json path/to/file.mf4              # JSON to stdout
 *    ./build/mdf4_json path/to/file.mf4 output.json  # JSON to file
 */

#include <fstream>
#include <iostream>
#include <string>

#include "mdf4/extract.h"

#include <google/protobuf/util/json_util.h>

using namespace std;

int main(int argc, char* argv[]) {
    // extractFile is total — an absent path yields the empty document.
    const string path = argc >= 2 ? argv[1] : string();

    mdf4::File file = mdf4::extract::extractFile(path);

    google::protobuf::util::JsonPrintOptions opts;
    opts.add_whitespace = true;

    string json;
    auto status = google::protobuf::util::MessageToJsonString(file, &json, opts);
    if (!status.ok()) {
        cerr << "JSON serialization failed: " << status.message() << endl;
        return 1;
    }

    if (argc >= 3) {
        ofstream out(argv[2]);
        if (!out.is_open()) {
            cerr << "Cannot open output file: " << argv[2] << endl;
            return 1;
        }
        out << json;
        out.close();
        cerr << "Wrote " << json.size() << " bytes to " << argv[2] << endl;
    } else {
        cout << json << endl;
    }

    return 0;
}
