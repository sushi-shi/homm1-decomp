// The WinHelp converter (src/PLATFORM/Help.cpp), which reads the help file
// the user's game data holds and writes the HTML book the Help menu shows.
//
// The input is the help file, then the text of its contents file, then the
// contents file's length as two little-endian bytes (a length past the input
// means no contents file).

#include "FuzzSupport.h"

#include <PLATFORM/Help.h>

#include <string>
#include <vector>

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 2 || size > (1u << 22))
        return 0;
    size_t contentsLength = data[size - 2] | (static_cast<size_t>(data[size - 1]) << 8);
    size_t body = size - 2;
    if (contentsLength > body)
        contentsLength = 0;
    std::vector<u8> helpFile(data, data + body - contentsLength);
    std::string contents(reinterpret_cast<const char*>(data) + body - contentsLength, contentsLength);
    std::string html;
    std::string error;
    platform::help::Report report;
    if (!platform::help::Convert(helpFile, contents, 0, html, report, error))
        gFuzzRejected = true;
    return 0;
}
