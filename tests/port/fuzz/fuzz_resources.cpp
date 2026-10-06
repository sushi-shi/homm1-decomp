// The resource archive and the decoders that read a resource in place, in
// the game's units started headless. The input is a kind byte and a payload;
// the harness writes a small archive holding the payload as one resource,
// opens it with resourceManager::LoadAggregateHeader in a resource manager of
// its own, loads the resource through the real Get* call and draws what it
// decoded into exactly-sized buffers:
//
//   0  icon     (ICON.cpp) every frame through each icon blitter, normal,
//               flipped, mono, dimmed, clipped, at offsets that cut it
//   1  bitmap   (BITMAP.cpp) blitted whole; the payload also as a backdrop
//               read into the screen (GetBackdrop, GetBackdropAtLoc)
//   2  tileset  (TILESET.cpp) every tile with each flip
//   3  palette  (PALETTE.cpp)
//   4  font     (FONT.cpp) the first 17 bytes are the font record, the rest
//               the glyph icon it names (FUZZ.ICN in the seeds); text drawn,
//               measured and wrapped
//   5  sample   (SAMPLE.cpp) the header and its size only
//   6  archive  the payload is the whole archive: its directory, then every
//               entry looked up and read
//
// Kinds are taken modulo 7.

#include "FuzzSupport.h"

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/Icon2b.h>
#include <BASE/Icond2b.h>
#include <BASE/Iconm2b.h>
#include <BASE/IconEntry.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/tileset.h>
#include <BASE/TILE.h>
#include <PLATFORM/File.h>
#include <SOURCE/saveRecords.h>
#include <SOURCE/KB.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace {

std::string gRoot;

enum Kind { ICON, BITMAP, TILESET, PALETTE, FONT, SAMPLE, ARCHIVE, KIND_COUNT };

// The largest image the harness allocates for one draw.
const i32 kMaxPixels = 1 << 22;
const i32 kScreenWidth = 640;
const i32 kScreenHeight = 480;

u16 FileId(const char* name) {
    return static_cast<u16>(gResourceManager->MakeId(const_cast<char*>(name)));
}

void Put16(std::vector<u8>& out, u32 value) {
    out.push_back(static_cast<u8>(value));
    out.push_back(static_cast<u8>(value >> 8));
}

void Put32(std::vector<u8>& out, u32 value) {
    for (int i = 0; i < 4; i++)
        out.push_back(static_cast<u8>(value >> (8 * i)));
}

// An archive of the given entries: count, directory, then the payloads.
std::vector<u8> Archive(const std::vector<std::pair<u16, std::vector<u8>>>& entries) {
    std::vector<u8> out;
    Put16(out, static_cast<u32>(entries.size()));
    u32 offset = static_cast<u32>(2 + entries.size() * AGG_ENTRY_RECORD_SIZE);
    for (const auto& entry : entries) {
        Put16(out, entry.first);
        Put32(out, offset);
        Put32(out, static_cast<u32>(entry.second.size()));
        offset += static_cast<u32>(entry.second.size());
    }
    for (const auto& entry : entries)
        out.insert(out.end(), entry.second.begin(), entry.second.end());
    return out;
}

// An image whose pixel block is exactly width * height bytes.
struct Image {
    std::unique_ptr<bitmap> image;
    explicit Image(i32 width, i32 height) : image(new bitmap()) {
        image->m_width = static_cast<i16>(width);
        image->m_height = static_cast<i16>(height);
        size_t bytes = static_cast<size_t>(width) * static_cast<size_t>(height);
        image->m_pixels = static_cast<u8*>(std::malloc(bytes > 0 ? bytes : 1));
        std::memset(image->m_pixels, 0, bytes > 0 ? bytes : 1);
    }
};

void DrawIcon(icon* ic) {
    Image screen(kScreenWidth, kScreenHeight);
    for (i32 frame = 0; frame < ic->m_frameCount; frame++) {
        IconEntry entry;
        std::memcpy(&entry, ic->m_data + frame * sizeof(IconEntry), sizeof(entry));
        i32 width = static_cast<u16>(entry.w);
        i32 height = static_cast<u16>(entry.h);
        if (width == 0 || height == 0 || width > 0x7fff || height > 0x7fff
            || width * height > kMaxPixels)
            continue;
        // The frame exactly covering an image of its size.
        Image exact(width, height);
        bitmap* image = exact.image.get();
        i32 left = -entry.x;
        i32 top = -entry.y;
        i32 right = width - 1 + entry.x;
        IconToBitmap(ic, image, left, top, frame, ICON_DRAW_OFFSET_FULL);
        FlipIconToBitmap(ic, image, right, top, frame, ICON_DRAW_OFFSET_FULL);
        MonoIconToBitmap(ic, image, left, top, frame, 0x55, ICON_DRAW_OFFSET_FULL);
        FlipMonoIconToBitmap(ic, image, right, top, frame, 0x55, ICON_DRAW_OFFSET_FULL);
        ClippedIconToBitmap(ic, image, left, top, frame, ICON_DRAW_OFFSET_FULL);
        FlipClippedIconToBitmap(ic, image, right, top, frame, ICON_DRAW_OFFSET_FULL);
        // Cut on each side.
        for (i32 shift = -1; shift <= 1; shift += 2) {
            ClippedIconToBitmap(ic, image, left + shift * width / 2, top + shift * height / 2, frame,
                                ICON_DRAW_OFFSET_FULL);
            FlipClippedIconToBitmap(ic, image, right + shift * width / 2, top - shift * height / 2,
                                    frame, ICON_DRAW_OFFSET_FULL);
        }
        // The dimmed blitters step rows by the screen's 640 bytes; the game
        // draws them on the screen.
        if (width <= kScreenWidth && height <= kScreenHeight) {
            DimIconToBitmap(ic, screen.image.get(), left, top, frame, ICON_DRAW_OFFSET_FULL);
            FlipDimIconToBitmap(ic, screen.image.get(), right, top, frame, ICON_DRAW_OFFSET_FULL);
        }
        // The C++ clip routines draw on the screen inside a clip rectangle
        // (the adventure view, the radar).
        for (i32 shift = -1; shift <= 1; shift++) {
            i32 x = left + shift * width / 2;
            i32 y = top + shift * height / 2;
            ClipIconToBitmap(ic, screen.image.get(), x, y, frame, ICON_DRAW_OFFSET_FULL, 0, 0,
                             kScreenWidth, kScreenHeight);
            ClippedMonoIconToBitmap(ic, screen.image.get(), x, y, frame, 0x33,
                                    ICON_DRAW_OFFSET_FULL, 0, 0, kScreenWidth, kScreenHeight);
        }
    }
}

void DrawBitmap(bitmap* image) {
    i32 width = image->m_width;
    i32 height = image->m_height;
    if (width <= 0 || height <= 0 || width * height > kMaxPixels)
        return;
    Image exact(width, height);
    BlitBitmap(image, 0, 0, width, height, exact.image.get(), 0, 0);
}

void DrawTileset(tileset* tiles) {
    i32 width = tiles->m_tileWidth;
    i32 height = tiles->m_tileHeight;
    if (width == 0 || height == 0 || width * height > kMaxPixels)
        return;
    Image exact(width, height);
    for (u32 tile = 0; tile < tiles->m_tileCount && tile < 0x1000; tile++)
        for (u32 flip = 0; flip < 4; flip++)
            TileToBitmap(tiles, tile | (flip << 14), exact.image.get(), 0, 0);
}

void DrawText(font* face) {
    static char sample[] =
        "The quick brown fox jumps over the lazy dog. 0123456789 !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    char every[256];
    for (int i = 0; i < 255; i++)
        every[i] = static_cast<char>(i + 1);
    every[255] = '\0';
    char words[] = "Aaaa bbbb cccc\ndddd eeee ffff gggg hhhh iiii jjjj kkkk llll mmmm";
    for (char* text : {sample, every, words}) {
        face->LineWidth(text);
        face->LineLength(text, 200);
        face->DrawString(text, 10, 10, 1);
        face->DrawBoundedString(text, 20, 20, 200, 300, 1, FONT_ALIGN_LEFT);
        face->DrawBoundedString(text, 20, 20, 200, 300, 1, FONT_ALIGN_CENTER);
        face->DrawBoundedString(text, 20, 20, 200, 300, 1, FONT_ALIGN_RIGHT);
    }
}

void Exercise(Kind kind, const u8* payload, size_t size) {
    std::vector<u8> body(payload, payload + size);
    std::vector<u8> archive;
    switch (kind) {
        case ICON:
            archive = Archive({{FileId("FUZZ.ICN"), body}});
            break;
        case BITMAP:
            archive = Archive({{FileId("FUZZ.BMP"), body}});
            break;
        case TILESET:
            archive = Archive({{FileId("FUZZ.TIL"), body}});
            break;
        case PALETTE:
            archive = Archive({{FileId("FUZZ.PAL"), body}});
            break;
        case FONT: {
            size_t record = size < 17 ? size : 17;
            archive = Archive({{FileId("FUZZ.FNT"), std::vector<u8>(payload, payload + record)},
                               {FileId("FUZZ.ICN"), std::vector<u8>(payload + record, payload + size)}});
            break;
        }
        case SAMPLE:
            archive = Archive({{FileId("FUZZ.82M"), body}});
            break;
        case ARCHIVE:
        default:
            archive = body;
            break;
    }
    if (!FuzzWriteFile(gRoot + "/FUZZ.AGG", archive.data(), archive.size()))
        return;

    resourceManager* saved = gResourceManager;
    resourceManager* fuzzed = new resourceManager;
    gResourceManager = fuzzed;
    RunGame([&] {
        if (fuzzed->LoadAggregateHeader(const_cast<char*>("FUZZ.AGG")) != 0)
            return;
        switch (kind) {
            case ICON:
                DrawIcon(fuzzed->GetIcon(const_cast<char*>("FUZZ.ICN")));
                break;
            case BITMAP: {
                DrawBitmap(fuzzed->GetBitmap(const_cast<char*>("FUZZ.BMP")));
                Image screen(kScreenWidth, kScreenHeight);
                fuzzed->GetBackdrop(const_cast<char*>("FUZZ.BMP"), screen.image.get());
                fuzzed->GetBackdropAtLoc(const_cast<char*>("FUZZ.BMP"), screen.image.get(), 0, 0);
                fuzzed->GetBackdropAtLoc(const_cast<char*>("FUZZ.BMP"), screen.image.get(), 96, 0);
                break;
            }
            case TILESET:
                DrawTileset(fuzzed->GetTileset(const_cast<char*>("FUZZ.TIL")));
                break;
            case PALETTE:
                fuzzed->GetPalette(const_cast<char*>("FUZZ.PAL"));
                break;
            case FONT:
                DrawText(fuzzed->GetFont(const_cast<char*>("FUZZ.FNT")));
                break;
            case SAMPLE:
                fuzzed->GetSample(const_cast<char*>("FUZZ.82M"));
                break;
            case ARCHIVE:
            default:
                for (i32 entry = 0; entry < fuzzed->m_aggregateEntryCount; entry++) {
                    i16 id = fuzzed->m_aggregateDir[entry].id;
                    u32 length = fuzzed->GetFileSize(id);
                    fuzzed->PointToFile(id);
                    std::vector<u8> bytes(length > (1u << 22) ? (1u << 22) : length);
                    fuzzed->ReadBlock(bytes.data(), static_cast<u32>(bytes.size()));
                }
                break;
        }
    });
    RunGame([&] { fuzzed->Expunge(); });
    if (fuzzed->m_aggregateDir != NULL)
        std::free(fuzzed->m_aggregateDir);
    if (fuzzed->m_aggregateFd != FILE_DESCRIPTOR_INVALID)
        FileClose(fuzzed->m_aggregateFd);
    fuzzed->m_aggregateDir = NULL;
    fuzzed->m_aggregateFd = FILE_DESCRIPTOR_INVALID;
    delete fuzzed;
    gResourceManager = saved;
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_resources");
    gRoot = FuzzScratchGame();
    FuzzStartHost(gRoot);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > (1u << 22))
        return 0;
    Exercise(static_cast<Kind>(data[0] % KIND_COUNT), data + 1, size - 1);
    return 0;
}
