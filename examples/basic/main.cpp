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
    if (!path.empty())
        cout << "Version: " << file.version() << " (" << file.version_num() << ")"
             << (file.finalized() ? ", finalized" : ", unfinalized") << endl;

    for (int g = 0; g < file.groups_size(); ++g) {
        const mdf4::ChannelGroup& group = file.groups(g);
        cout << "Group " << g << " \"" << group.name() << "\": " << group.cycle_count()
             << " cycles, " << mdf4::StorageLayout_Name(group.storage()) << endl;
        for (int c = 0; c < group.channels_size(); ++c) {
            const mdf4::Channel& channel = group.channels(c);
            cout << "  [" << c << "] " << channel.name();
            if (!channel.unit().empty())
                cout << " [" << channel.unit() << "]";
            cout << " " << mdf4::DataType_Name(channel.data_type()) << "/"
                 << mdf4::ConversionKind_Name(channel.conversion().kind());
            if (channel.is_master())
                cout << " master";
            if (!channel.decodable())
                cout << " - not decodable: " << channel.not_decodable_reason();
            cout << endl;
        }
    }

    // Samples come back separately: decodeChannel(path, group, channel).
    cout << "Diagnostics: " << file.diagnostics_size() << endl;
    for (const auto& diag : file.diagnostics())
        cout << "  [" << mdf4::Severity_Name(diag.severity()) << "] "
             << diag.location() << ": " << diag.message() << endl;

    string binary;
    file.SerializeToString(&binary);
    cout << "Serialized: " << binary.size() << " bytes" << endl;

    return 0;
}
