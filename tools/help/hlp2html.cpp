// homm1-hlp2html: converts a WinHelp file to one HTML document with the
// port's converter (include/PLATFORM/Help.h), for inspection and for
// converting at install time.
//
//     homm1-hlp2html HEROES.HLP [HEROES.CNT] OUT.html

#include <PLATFORM/Help.h>

#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) {
        std::fprintf(stderr, "usage: %s HELP.HLP [HELP.CNT] OUT.html\n", argv[0]);
        return 2;
    }
    std::ifstream helpFile(argv[1], std::ios::binary);
    std::vector<u8> help((std::istreambuf_iterator<char>(helpFile)), std::istreambuf_iterator<char>());
    if (!helpFile.good() && !helpFile.eof()) {
        std::fprintf(stderr, "%s: cannot read\n", argv[1]);
        return 1;
    }
    std::string contents;
    if (argc == 4) {
        std::ifstream contentsFile(argv[2], std::ios::binary);
        contents.assign(std::istreambuf_iterator<char>(contentsFile), std::istreambuf_iterator<char>());
    }
    std::string html;
    std::string error;
    platform::help::Report report;
    if (!platform::help::Convert(help, contents, 0, html, report, error)) {
        std::fprintf(stderr, "%s: %s\n", argv[1], error.c_str());
        return 1;
    }
    const char* output = argv[argc - 1];
    std::ofstream out(output, std::ios::binary | std::ios::trunc);
    out.write(html.data(), static_cast<std::streamsize>(html.size()));
    if (!out) {
        std::fprintf(stderr, "%s: cannot write\n", output);
        return 1;
    }
    std::fprintf(stderr,
                 "%s: WinHelp version %d, code page %d: %d topics, %d text records (%d with "
                 "unparsed formatting), %d contents entries (%d unresolved), %d links (%d "
                 "unresolved), %d keywords, %d pictures (%d failed)\n",
                 output, report.version, report.codepage, report.topics, report.records,
                 report.recordsFailed, report.contentsEntries, report.contentsUnresolved,
                 report.jumps, report.jumpsUnresolved, report.keywords, report.images,
                 report.imagesFailed);
    return 0;
}
