// The WinHelp converter (include/PLATFORM/Help.h).
//
// Without game data: a small help file built here (two topics, a link
// between them, a picture, a contents file) converts completely, and damaged
// copies of it (truncated, bytes changed) are refused or converted without
// reading outside the file (run under the sanitizers).
//
// With the game's help file ($HOMM1_HELP, else HELP\HEROES.HLP under
// $HOMM1_DATA): every topic, contents entry, link and picture converts, and
// the document is UTF-8 whose every in-page link has its target. No content of
// the book is printed or compared: the book is the user's data.

#include <PLATFORM/File.h>
#include <PLATFORM/Help.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <set>
#include <string>
#include <vector>

namespace {

int gFailures = 0;

#define CHECK(condition)                                                        \
    do {                                                                        \
        if (!(condition)) {                                                     \
            std::fprintf(stderr, "%s:%d: CHECK(%s)\n", __FILE__, __LINE__, #condition); \
            gFailures++;                                                        \
        }                                                                       \
    } while (0)

using Bytes = std::vector<u8>;

void Put8(Bytes& out, u32 value) {
    out.push_back(static_cast<u8>(value));
}
void Put16(Bytes& out, u32 value) {
    Put8(out, value);
    Put8(out, value >> 8);
}
void Put32(Bytes& out, u32 value) {
    Put16(out, value & 0xFFFF);
    Put16(out, value >> 16);
}
void PutText(Bytes& out, const char* text) {
    out.insert(out.end(), text, text + std::strlen(text) + 1);
}
// The short forms of WinHelp's variable-length numbers.
void PutCompressed16(Bytes& out, u32 value) {
    Put8(out, value << 1);
}
void PutCompressed32(Bytes& out, u32 value) {
    Put16(out, value << 1);
}

u32 Hash(const char* context) {
    u32 hash = 0;
    for (const char* c = context; *c != '\0'; c++) {
        int value = *c >= 'a' && *c <= 'z' ? *c - 0x50 : *c - 0x30;
        hash = hash * 43u + static_cast<u32>(value);
    }
    return hash;
}

// A one-page, one-level B+ tree of the given entries.
Bytes Tree(const char* structure, const std::vector<Bytes>& entries) {
    const u32 pageSize = 1024;
    Bytes tree;
    Put16(tree, 0x293B);
    Put16(tree, 2);
    Put16(tree, pageSize);
    char name[16] = {};
    std::strncpy(name, structure, sizeof(name) - 1);
    tree.insert(tree.end(), name, name + 16);
    Put16(tree, 0);
    Put16(tree, 0);
    Put16(tree, 0);       // root page
    Put16(tree, 0xFFFF);
    Put16(tree, 1);       // pages
    Put16(tree, 1);       // levels
    Put32(tree, static_cast<u32>(entries.size()));
    size_t page = tree.size();
    Put16(tree, 0);
    Put16(tree, static_cast<u32>(entries.size()));
    Put16(tree, 0xFFFF);
    Put16(tree, 0xFFFF);
    for (const Bytes& entry : entries)
        tree.insert(tree.end(), entry.begin(), entry.end());
    tree.resize(page + pageSize, 0);
    return tree;
}

struct Link {
    u8 type;
    Bytes data1;
    Bytes data2;
};

// A help file with two topics; the first links to the second ("SECOND") and
// shows a 2x2 picture.
Bytes BuildHelpFile() {
    // Topic text records.
    Bytes text1;
    for (const char* part : {"Hello ", "link", "", "", ""})
        PutText(text1, part);
    Bytes format1;
    PutCompressed32(format1, 0);
    PutCompressed16(format1, static_cast<u32>(text1.size()));
    Put32(format1, 0x00008000);  // paragraph prefix
    Put16(format1, 0);           // paragraph bits
    Put8(format1, 0xE3);         // jump
    Put32(format1, Hash("SECOND"));
    Put8(format1, 0x89);         // end of hotspot
    Put8(format1, 0x86);         // picture
    Put8(format1, 0x03);
    PutCompressed32(format1, 4);
    Put16(format1, 0);           // in |bm0
    Put16(format1, 0);
    Put8(format1, 0x82);
    Put8(format1, 0xFF);

    Bytes text2;
    PutText(text2, "Second <page> & more");
    Bytes format2;
    PutCompressed32(format2, 0);
    PutCompressed16(format2, static_cast<u32>(text2.size()));
    Put32(format2, 0x00008000);
    Put16(format2, 0x0800);  // centred
    Put8(format2, 0xFF);

    std::vector<Link> links;
    auto header = [](const char* title) {
        Link link{0x02, {}, {}};
        for (int i = 0; i < 7; i++)
            Put32(link.data1, i == 1 || i == 2 || i == 4 || i == 5 ? 0xFFFFFFFF : 0);
        PutText(link.data2, title);
        return link;
    };
    links.push_back(header("One"));
    links.push_back(Link{0x20, format1, text1});
    links.push_back(header("Two"));
    links.push_back(Link{0x20, format2, text2});

    // One uncompressed 4 KB block; positions count its 12-byte header.
    Bytes block;
    std::vector<u32> positions;
    u32 position = 12;
    for (const Link& link : links) {
        positions.push_back(position);
        position += static_cast<u32>(21 + link.data1.size() + link.data2.size());
    }
    for (size_t i = 0; i < links.size(); i++) {
        const Link& link = links[i];
        u32 data1 = static_cast<u32>(21 + link.data1.size());
        Put32(block, data1 + static_cast<u32>(link.data2.size()));
        Put32(block, static_cast<u32>(link.data2.size()));
        Put32(block, i == 0 ? 0xFFFFFFFF : positions[i - 1]);
        Put32(block, i + 1 < links.size() ? positions[i + 1] : 0xFFFFFFFF);
        Put32(block, data1);
        Put8(block, link.type);
        block.insert(block.end(), link.data1.begin(), link.data1.end());
        block.insert(block.end(), link.data2.begin(), link.data2.end());
    }
    Bytes topic;
    Put32(topic, positions.back());
    Put32(topic, positions.front());
    Put32(topic, positions[2]);
    topic.insert(topic.end(), block.begin(), block.end());
    topic.resize(4096, 0);

    Bytes system;
    Put16(system, 0x036C);
    Put16(system, 33);
    Put16(system, 1);
    Put32(system, 0);
    Put16(system, 0);  // not compressed
    Put16(system, 1);
    Put16(system, 2);
    PutText(system, "T");

    // Topic offsets count text characters: the second topic starts after
    // the first's text.
    std::vector<Bytes> contexts;
    for (auto [name, offset] : {std::pair<const char*, u32>{"FIRST", 0},
                                {"SECOND", static_cast<u32>(text1.size())}}) {
        Bytes entry;
        Put32(entry, Hash(name));
        Put32(entry, offset);
        contexts.push_back(entry);
    }

    Bytes picture;
    Put16(picture, 0x706C);
    Put16(picture, 1);
    Put32(picture, 8);
    Put8(picture, 6);  // DIB
    Put8(picture, 0);  // not packed
    PutCompressed32(picture, 96);
    PutCompressed32(picture, 96);
    PutCompressed16(picture, 1);
    PutCompressed16(picture, 8);
    PutCompressed32(picture, 2);  // width
    PutCompressed32(picture, 2);  // height
    PutCompressed32(picture, 2);  // colours
    PutCompressed32(picture, 0);
    PutCompressed32(picture, 8);  // data size
    PutCompressed32(picture, 0);
    size_t offsetField = picture.size();
    Put32(picture, 0);
    Put32(picture, 0);
    for (u32 color : {0x000000u, 0xFFFFFFu})
        Put32(picture, color);
    u32 dataOffset = static_cast<u32>(picture.size() - 8);
    for (int i = 0; i < 4; i++)
        picture[offsetField + static_cast<size_t>(i)] = static_cast<u8>(dataOffset >> (8 * i));
    for (u8 pixel : {0, 1, 0, 0, 1, 0, 0, 0})
        Put8(picture, pixel);

    std::vector<std::pair<const char*, Bytes>> files = {
        {"|SYSTEM", system},
        {"|TOPIC", topic},
        {"|CONTEXT", Tree("L4", contexts)},
        {"|bm0", picture},
    };
    Bytes help(16, 0);
    std::vector<Bytes> directory;
    for (const auto& [name, data] : files) {
        Bytes entry;
        PutText(entry, name);
        Put32(entry, static_cast<u32>(help.size()));
        directory.push_back(entry);
        Put32(help, static_cast<u32>(data.size()));
        Put32(help, static_cast<u32>(data.size()));
        Put8(help, 0);
        help.insert(help.end(), data.begin(), data.end());
    }
    u32 directoryStart = static_cast<u32>(help.size());
    Bytes tree = Tree("z4", directory);
    Put32(help, static_cast<u32>(tree.size()));
    Put32(help, static_cast<u32>(tree.size()));
    Put8(help, 0);
    help.insert(help.end(), tree.begin(), tree.end());
    Bytes fileHeader;
    Put32(fileHeader, 0x00035F3F);
    Put32(fileHeader, directoryStart);
    Put32(fileHeader, 0xFFFFFFFF);
    Put32(fileHeader, static_cast<u32>(help.size()));
    std::copy(fileHeader.begin(), fileHeader.end(), help.begin());
    return help;
}

bool ValidUtf8(const std::string& text) {
    size_t i = 0;
    while (i < text.size()) {
        u8 c = static_cast<u8>(text[i]);
        size_t length = c < 0x80 ? 1 : (c & 0xE0) == 0xC0 ? 2 : (c & 0xF0) == 0xE0 ? 3
                                       : (c & 0xF8) == 0xF0                  ? 4
                                                                             : 0;
        if (length == 0 || i + length > text.size())
            return false;
        for (size_t k = 1; k < length; k++) {
            if ((static_cast<u8>(text[i + k]) & 0xC0) != 0x80)
                return false;
        }
        i += length;
    }
    return true;
}

// Every href="#x" (x non-empty) has an element with id="x".
bool LinksResolve(const std::string& html, int& links) {
    std::set<std::string> ids;
    for (size_t at = 0; (at = html.find(" id=\"", at)) != std::string::npos;) {
        at += 5;
        ids.insert(html.substr(at, html.find('"', at) - at));
    }
    links = 0;
    bool ok = true;
    for (size_t at = 0; (at = html.find("href=\"#", at)) != std::string::npos;) {
        at += 7;
        std::string target = html.substr(at, html.find('"', at) - at);
        if (target.empty())
            continue;
        links++;
        if (ids.count(target) == 0) {
            std::fprintf(stderr, "link target #%s missing\n", target.c_str());
            ok = false;
        }
    }
    return ok;
}

void TestSynthetic() {
    Bytes help = BuildHelpFile();
    std::string html;
    std::string error;
    platform::help::Report report;
    bool ok = platform::help::Convert(help, "1 Book\r\n2 Second=SECOND\r\n", 0, html, report, error);
    if (!ok)
        std::fprintf(stderr, "synthetic: %s\n", error.c_str());
    CHECK(ok);
    CHECK(report.topics == 2);
    CHECK(report.records == 2);
    CHECK(report.recordsFailed == 0);
    CHECK(report.jumps == 1);
    CHECK(report.jumpsUnresolved == 0);
    CHECK(report.images == 1);
    CHECK(report.imagesFailed == 0);
    CHECK(report.contentsEntries == 1);
    CHECK(report.contentsUnresolved == 0);
    CHECK(html.find("<a class=\"jump\" href=\"#t1\">link</a>") != std::string::npos);
    CHECK(html.find("data:image/bmp;base64,Qk") != std::string::npos);
    CHECK(html.find("Second &lt;page&gt; &amp; more") != std::string::npos);
    CHECK(html.find("text-align:center") != std::string::npos);
    CHECK(html.find("<a href=\"#t1\">Second</a>") != std::string::npos);
    int links = 0;
    CHECK(LinksResolve(html, links));
    CHECK(ValidUtf8(html));

    // Damaged files: refused, or converted, never read outside the buffer.
    CHECK(!platform::help::Convert(Bytes(), "", 0, html, report, error));
    Bytes wrongMagic = help;
    wrongMagic[0] ^= 0xFF;
    CHECK(!platform::help::Convert(wrongMagic, "", 0, html, report, error));
    for (size_t length = 0; length < help.size(); length += 7) {
        Bytes truncated(help.begin(), help.begin() + static_cast<std::ptrdiff_t>(length));
        platform::help::Convert(truncated, "", 0, html, report, error);
    }
    u32 seed = 12345;
    for (int round = 0; round < 3000; round++) {
        Bytes damaged = help;
        for (int k = 0; k < 4; k++) {
            seed = seed * 1103515245u + 12345u;
            size_t at = (seed >> 8) % damaged.size();
            seed = seed * 1103515245u + 12345u;
            damaged[at] = static_cast<u8>(seed >> 16);
        }
        if (platform::help::Convert(damaged, "1 A=SECOND\n2 B\n9 C=X@other.hlp\n", 0, html, report,
                                    error))
            CHECK(ValidUtf8(html));
    }
}

bool ReadFile(const std::string& path, Bytes& bytes) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    bytes.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

// The game's help file: 0 checked, 77 when there is none.
int TestGameHelp() {
    std::string path;
    const char* given = std::getenv("HOMM1_HELP");
    const char* data = std::getenv("HOMM1_DATA");
    if (given != nullptr && *given != '\0') {
        path = given;
    } else if (data != nullptr && *data != '\0') {
        char resolved[FILE_PATH_CAPACITY];
        FileSetRoot(data);
        if (FileExists("HELP\\HEROES.HLP")
            && FileResolve("HELP\\HEROES.HLP", FILE_OPEN_READ, resolved, sizeof(resolved)))
            path = resolved;
    }
    Bytes help;
    if (path.empty() || !ReadFile(path, help)) {
        std::printf("no game help file (HOMM1_HELP, or HELP/HEROES.HLP under HOMM1_DATA)\n");
        return 77;
    }
    Bytes contentsBytes;
    std::string contentsPath = path.substr(0, path.size() - 3);
    for (const char* extension : {"CNT", "cnt"}) {
        if (ReadFile(contentsPath + extension, contentsBytes))
            break;
    }
    std::string contents(contentsBytes.begin(), contentsBytes.end());
    std::string html;
    std::string error;
    platform::help::Report report;
    bool ok = platform::help::Convert(help, contents, 0, html, report, error);
    if (!ok)
        std::fprintf(stderr, "%s: %s\n", path.c_str(), error.c_str());
    CHECK(ok);
    int links = 0;
    CHECK(LinksResolve(html, links));
    CHECK(ValidUtf8(html));
    CHECK(report.topics > 0);
    CHECK(report.recordsFailed == 0);
    CHECK(report.jumpsUnresolved == 0);
    CHECK(report.imagesFailed == 0);
    CHECK(contents.empty() || report.contentsEntries > 0);
    CHECK(report.contentsUnresolved == 0);
    std::printf("game help: version %d, code page %d, %d topics, %d text records, %d contents "
                "entries, %d links in the text, %d keywords, %d pictures, %d in-page links\n",
                report.version, report.codepage, report.topics, report.records,
                report.contentsEntries, report.jumps, report.keywords, report.images, links);

    // Damaged copies of the real file.
    for (size_t length = 0; length < help.size(); length += 997) {
        Bytes truncated(help.begin(), help.begin() + static_cast<std::ptrdiff_t>(length));
        platform::help::Convert(truncated, contents, 0, html, report, error);
    }
    u32 seed = 777;
    for (int round = 0; round < 200; round++) {
        Bytes damaged = help;
        for (int k = 0; k < 16; k++) {
            seed = seed * 1103515245u + 12345u;
            size_t at = (seed >> 8) % damaged.size();
            seed = seed * 1103515245u + 12345u;
            damaged[at] = static_cast<u8>(seed >> 16);
        }
        if (platform::help::Convert(damaged, contents, 0, html, report, error))
            CHECK(ValidUtf8(html));
    }
    return 0;
}

}  // namespace

// help_test: the synthetic file; help_test --game: the game's help file.
int main(int argc, char** argv) {
    int result = 0;
    if (argc > 1 && std::strcmp(argv[1], "--game") == 0)
        result = TestGameHelp();
    else
        TestSynthetic();
    if (gFailures != 0) {
        std::fprintf(stderr, "%d check(s) failed\n", gFailures);
        return 1;
    }
    return result;
}
