#ifndef HOMM1_PLATFORM_HELP_H
#define HOMM1_PLATFORM_HELP_H

// The game's help book. The original shipped a WinHelp file (HELP\HEROES.HLP
// with its contents file HEROES.CNT) and opened it with WinHelp, which current
// systems no longer have. This converter reads WinHelp 3.1 and 4.0 files and
// writes one self-contained HTML document (contents, keyword index, every
// topic, links between them and the pictures inline), which the port shows in
// the system's browser. The help file is the user's game data: it is
// converted on the user's machine, never shipped converted.

#include <H1/Ints.h>

#include <string>
#include <vector>

namespace platform::help {

// Bumped whenever the output changes, so cached conversions are redone.
constexpr int CONVERTER_VERSION = 2;

// What a conversion found, for tests and diagnostics.
struct Report {
    int version = 0;              // |SYSTEM minor version (21: 3.1, 33: 4.0)
    int codepage = 0;             // of the topic text
    int topics = 0;
    int records = 0;              // text records
    int recordsFailed = 0;        // text records with unparsed formatting
    int contentsEntries = 0;      // contents file lines naming a topic
    int contentsUnresolved = 0;   // ... whose topic is not in the file
    int jumps = 0;                // links in the topic text
    int jumpsUnresolved = 0;
    int keywords = 0;
    int images = 0;               // pictures shown
    int imagesFailed = 0;         // pictures that could not be decoded
};

// The page's own words, UTF-8: the headings of the contents and the keyword
// index, and each topic's link back to the contents. The program passes them
// in its language (the catalog's help.contents and help.index).
struct Labels {
    std::string contents = "Contents";
    std::string index = "Index";
};

// Converts the help file's bytes and the text of its contents file (empty
// when there is none). codepage 0 takes the text's code page from the file.
// On failure returns false with a reason in error.
bool Convert(
    const std::vector<u8>& helpFile,
    const std::string& contentsFile,
    int codepage,
    std::string& html,
    Report& report,
    std::string& error,
    const Labels& labels = Labels()
);

// The same for files on disk; cntPath may be empty.
bool ConvertWinHelp(
    const std::string& hlpPath,
    const std::string& cntPath,
    std::string& html,
    std::string& error,
    const Labels& labels = Labels()
);

// Converts hlpPath (with cntPath, which may be empty or missing) into an HTML
// file in outputDirectory unless a conversion of the same files by the same
// converter version is already there. Sets htmlPath to the HTML file.
bool PrepareHelp(
    const std::string& hlpPath,
    const std::string& cntPath,
    const std::string& outputDirectory,
    std::string& htmlPath,
    std::string& error,
    const Labels& labels = Labels()
);

}  // namespace platform::help

#endif
