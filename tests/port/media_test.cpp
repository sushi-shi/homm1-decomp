// The movie and music decoders on the shipped files (needs $HOMM1_DATA):
// every movie in ANIM decodes to the frames, palettes and sound FFmpeg's
// Smacker decoder gave (their hashes, recorded when the port moved off
// FFmpeg), and every music file in SOUND decodes to its length.
//
//   media_test [--print]   --print lists the hashes of the files found

#include <H1/Ints.h>

#include <PLATFORM/File.h>
#include <PLATFORM/SmackerDecoder.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace {

struct Movie {
    const char* path;
    int frames;
    u64 video;   // FNV-1a over every frame's pixels and palette
    u64 audio;   // FNV-1a over the first audio track's samples
};

const Movie kMovies[] = {
    {"ANIM\\BUKA.SMK", 420, 0x28bebd61bccb096bull, 0x884ca98e1ff6a14cull},
    {"ANIM\\NWCLOGO.SMK", 101, 0xf3269cd6f04332a6ull, 0x0e74929e208e1a94ull},
    {"ANIM\\INTRO.SMK", 161, 0xc2a513e661520297ull, 0x0c58ce8030749ce4ull},
    {"ANIM\\LOSE.SMK", 40, 0xa32291f6ffbb41c6ull, 0xcdb479b06775ef8dull},
    {"ANIM\\WIN1.SMK", 108, 0xe7d5fbe12dd514d1ull, 0x56b0f07b64b50800ull},
    {"ANIM\\WIN2.SMK", 66, 0x92b072b5266fea15ull, 0xb23ee448d8624882ull},
};

// Mono samples per file, as FFmpeg decodes them; stb_vorbis ends HEROES18 360
// samples earlier, at the stream's final granule position.
struct Music {
    int number;
    int samples;
};

const Music kMusic[] = {
    {0, 3427092},  {1, 2584368},  {2, 3575706},  {3, 3440502},  {4, 2744260},  {5, 2399714},
    {6, 2584368},  {7, 97788},    {8, 107264},   {9, 89564},    {10, 122368},  {11, 105472},
    {12, 164774},  {13, 188980},  {14, 119298},  {15, 113796},  {16, 51470},   {17, 131072},
    {18, 13848},   {19, 109136},  {20, 93456},   {21, 54610},   {22, 39728},   {23, 42556},
    {24, 44608},   {25, 54913},   {26, 75778},   {27, 90270},   {28, 171008},  {29, 4298880},
    {30, 5922818}, {31, 4942146}, {32, 5863424}, {40, 3628356}, {41, 2315132}, {42, 2632018},
    {43, 268000},  {44, 412336},  {45, 412336},  {46, 233450},  {47, 208609},  {48, 1875712},
    {49, 1385472}, {50, 83710},   {51, 77566},   {52, 18814},   {53, 3157630}, {54, 1339390},
    {99, 1464320},
};

int gFailures = 0;

u64 Fnv(u64 hash, const u8* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        hash ^= data[i];
        hash *= 0x100000001b3ull;
    }
    return hash;
}

bool ReadGameFile(const char* path, std::vector<u8>& data) {
    i32 file = FileOpen(path, FILE_OPEN_READ);
    if (file == FILE_INVALID)
        return false;
    data.resize(static_cast<size_t>(FileLength(file)));
    bool ok = data.empty() || FileReadExact(file, data.data(), static_cast<i32>(data.size()));
    FileClose(file);
    return ok;
}

}  // namespace

int main(int argc, char** argv) {
    bool print = argc > 1 && std::strcmp(argv[1], "--print") == 0;
    const char* root = std::getenv("HOMM1_DATA");
    if (root == nullptr || *root == '\0') {
        std::printf("skipped: needs HOMM1_DATA\n");
        return 77;
    }
    FileSetRoot(root);
    int checked = 0;
    for (const Movie& movie : kMovies) {
        std::vector<u8> file;
        if (!ReadGameFile(movie.path, file))
            continue;
        platform::smacker::Decoder decoder;
        if (!decoder.Open(file)) {
            std::fprintf(stderr, "FAIL: %s does not open\n", movie.path);
            gFailures++;
            continue;
        }
        u64 video = 0xcbf29ce484222325ull;
        u64 audio = 0xcbf29ce484222325ull;
        std::vector<u8> samples;
        int frames = 0;
        while (decoder.DecodeFrame(0, &samples)) {
            video = Fnv(video, decoder.Pixels().data(), decoder.Pixels().size());
            video = Fnv(video, decoder.Palette().data(), decoder.Palette().size());
            audio = Fnv(audio, samples.data(), samples.size());
            samples.clear();
            frames++;
        }
        checked++;
        if (print) {
            std::printf("    {\"%s\", %d, 0x%016" PRIx64 "ull, 0x%016" PRIx64 "ull},\n", movie.path,
                        frames, video, audio);
            continue;
        }
        if (frames != movie.frames || video != movie.video || audio != movie.audio) {
            std::fprintf(stderr, "FAIL: %s: %d frames, video %016" PRIx64 ", audio %016" PRIx64 "\n",
                         movie.path, frames, video, audio);
            gFailures++;
        }
    }
    for (const Music& music : kMusic) {
        char path[32];
        std::snprintf(path, sizeof(path), "SOUND\\HEROES%02d.OGG", music.number);
        std::vector<u8> file;
        if (!ReadGameFile(path, file))
            continue;
        int error = 0;
        stb_vorbis* decoder =
            stb_vorbis_open_memory(file.data(), static_cast<int>(file.size()), &error, nullptr);
        if (decoder == nullptr) {
            std::fprintf(stderr, "FAIL: %s does not open (%d)\n", path, error);
            gFailures++;
            continue;
        }
        short buffer[4096];
        int samples = 0;
        int got;
        while ((got = stb_vorbis_get_samples_short_interleaved(decoder, 1, buffer, 4096)) > 0)
            samples += got;
        stb_vorbis_close(decoder);
        checked++;
        if (samples != music.samples) {
            std::fprintf(stderr, "FAIL: %s: %d samples, expected %d\n", path, samples, music.samples);
            gFailures++;
        }
    }
    if (checked == 0) {
        std::printf("skipped: no movies or music under HOMM1_DATA\n");
        return 77;
    }
    std::printf("%d files checked, %d failed\n", checked, gFailures);
    return gFailures == 0 ? 0 : 1;
}
