/*
 *  mdf4-parser basic example.
 *  Opens an MDF4 measurement file once, prints its structure from the protobuf
 *  metadata, then reads the first plottable channel's samples from that same
 *  opened source.
 *
 *  Build:
 *    cd examples/basic
 *    cmake -B build
 *    cmake --build build
 *
 *  Run:
 *    ./build/mdf4_basic path/to/file.mf4
 *    ./build/mdf4_basic                    # no file: one DROPPED diagnostic
 */

#include <iostream>
#include <string>

#include "mdf4/reader.h"

using namespace std;

int main(int argc, char* argv[]) {
    // Opening never fails, it reports. An absent path yields a document with no
    // structure and one DROPPED diagnostic, so the example runs without a
    // measurement file.
    const string path = argc >= 2 ? argv[1] : string();

    mdf4::Reader reader(path);
    const mdf4::File& file = reader.metadata();

    cout << "MDF4: " << (path.empty() ? "(no file)" : path) << endl;
    if (!reader.ready())
        cout << "Not ready: " << reader.openOutcome().message << endl;
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

    // Samples come from the same reader, addressed by the metadata's indices.
    for (int g = 0; reader.ready() && g < file.groups_size(); ++g) {
        const mdf4::ChannelGroup& group = file.groups(g);
        int c = 0;
        while (c < group.channels_size() &&
               (group.channels(c).is_master() || !group.channels(c).decodable()))
            ++c;
        if (c == group.channels_size())
            continue;
        // A read is bounded by the reader's output allowance; a longer channel
        // is refused explicitly and stays readable in windows or by scan().
        const mdf4::ReadResult samples = reader.read(uint32_t(g), uint32_t(c));
        cout << "Read " << group.channels(c).name() << ": ";
        if (samples.outcome.status != mdf4::Status::Ok)
            cout << "not read at " << samples.outcome.location << ": " << samples.outcome.message << endl;
        else if (samples.series.value.empty())
            cout << "no samples" << endl;
        else {
            cout << samples.series.value.size() << " samples, first " << samples.series.value.front()
                 << " at " << samples.series.time.front();
            if (samples.coverage == mdf4::Coverage::Short)
                cout << " (short: " << samples.outcome.message << ")";
            cout << endl;
        }
        break;
    }

    cout << "Diagnostics: " << file.diagnostics_size() << endl;
    for (const auto& diag : file.diagnostics())
        cout << "  [" << mdf4::Severity_Name(diag.severity()) << "] "
             << diag.location() << ": " << diag.message() << endl;

    string binary;
    file.SerializeToString(&binary);
    cout << "Serialized: " << binary.size() << " bytes" << endl;

    return 0;
}
