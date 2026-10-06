// The Smacker calls the game makes (smack.h), implemented on the platform's
// movie decoder in place of RAD's SMACKW32.DLL. A movie decodes one frame
// ahead, so that the palette of the frame about to be shown is in
// Smack::Palette when the game looks at it, as with RAD's library; after the
// last frame the frame number wraps to 0, which is how the game notices the
// end.

#include <H1/Ints.h>

#include <SOURCE/smack.h>

#include <PLATFORM/File.h>
#include <PLATFORM/Platform.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <memory>
#include <string>

float SmackSoundVolume();

namespace {

struct Playback {
    platform::Movie* movie = nullptr;
    std::string path;
    bool withAudio = false;
    u8* buffer = nullptr;
    u32 pitch = 0;
    u32 bufferHeight = 0;
    u32 left = 0;
    u32 top = 0;
    bool frameReady = false;
    bool rectPending = false;
    std::array<u8, 768> lastPalette{};
    bool started = false;
    u32 startTick = 0;
};

std::map<Smack*, std::unique_ptr<Playback>> gPlaybacks;

// Decodes the next frame and records whether its palette changed.
void DecodeAhead(Smack* smack, Playback& playback) {
    playback.frameReady = platform::DecodeMovieFrame(playback.movie);
    if (!playback.frameReady)
        return;
    const u8* palette = platform::MovieFramePalette(playback.movie);
    smack->NewPalette =
        smack->FrameNum == 0 || std::memcmp(palette, playback.lastPalette.data(), 768) != 0;
    std::memcpy(smack->Palette, palette, 768);
    std::memcpy(playback.lastPalette.data(), palette, 768);
}

// The frame count and rate from the Smacker file header: "SMK2" or "SMK4",
// width, height, frame count, then the rate - milliseconds per frame when
// positive, hundredths of a millisecond when negative, 10 frames a second
// when zero - all little-endian 32-bit.
bool ReadHeader(const char* path, u32& frames, u32& millisecondsPerFrame) {
    u8 header[20];
    i32 file = FileOpen(path, FILE_OPEN_READ);
    if (file == FILE_INVALID)
        return false;
    bool ok = FileReadExact(file, header, sizeof(header));
    FileClose(file);
    if (!ok || std::memcmp(header, "SMK", 3) != 0)
        return false;
    auto word = [&header](int offset) {
        return static_cast<u32>(header[offset]) | (static_cast<u32>(header[offset + 1]) << 8)
               | (static_cast<u32>(header[offset + 2]) << 16)
               | (static_cast<u32>(header[offset + 3]) << 24);
    };
    frames = word(12);
    i32 rate = static_cast<i32>(word(16));
    if (rate > 0)
        millisecondsPerFrame = static_cast<u32>(rate);
    else if (rate < 0)
        millisecondsPerFrame = static_cast<u32>(-rate) / 100;
    else
        millisecondsPerFrame = 100;
    return frames > 0;
}

bool Restart(Smack* smack, Playback& playback) {
    platform::CloseMovie(playback.movie);
    playback.movie =
        platform::OpenMovie(playback.path.c_str(), playback.withAudio, SmackSoundVolume());
    if (playback.movie == nullptr)
        return false;
    DecodeAhead(smack, playback);
    return true;
}

}  // namespace

extern "C" Smack* SmackOpen(char* name, u32 flags, i32) {
    char resolved[FILE_PATH_CAPACITY];
    if (!FileResolve(name, FILE_OPEN_READ, resolved, sizeof(resolved)))
        return nullptr;
    auto playback = std::make_unique<Playback>();
    playback->path = resolved;
    playback->withAudio = (flags & SMACK_TRACKS) != 0;
    playback->movie = platform::OpenMovie(resolved, playback->withAudio, SmackSoundVolume());
    if (playback->movie == nullptr) {
        platform::Log("cannot play %s", resolved);
        return nullptr;
    }
    const platform::MovieInfo& info = platform::MovieDetails(playback->movie);
    Smack* smack = new Smack();
    smack->Width = static_cast<u32>(info.width);
    smack->Height = static_cast<u32>(info.height);
    u32 frames = 0;
    u32 millisecondsPerFrame = 0;
    if (!ReadHeader(name, frames, millisecondsPerFrame)) {
        frames = static_cast<u32>(info.frames > 0 ? info.frames : 1);
        millisecondsPerFrame = static_cast<u32>(1000.0 / info.framesPerSecond);
    }
    smack->Frames = frames;
    smack->MSPerFrame = millisecondsPerFrame;
    smack->FrameNum = 0;
    DecodeAhead(smack, *playback);
    gPlaybacks[smack] = std::move(playback);
    return smack;
}

extern "C" void SmackClose(Smack* smack) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end())
        return;
    platform::CloseMovie(found->second->movie);
    gPlaybacks.erase(found);
    delete smack;
}

extern "C" void SmackToBuffer(Smack* smack, u32 left, u32 top, u32 pitch, u32 height, void* buffer,
                              u32) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end())
        return;
    Playback& playback = *found->second;
    playback.buffer = static_cast<u8*>(buffer);
    playback.left = left;
    playback.top = top;
    playback.pitch = pitch;
    playback.bufferHeight = height;
}

extern "C" u32 SmackDoFrame(Smack* smack) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end())
        return 0;
    Playback& playback = *found->second;
    if (!playback.frameReady || playback.buffer == nullptr)
        return 0;
    const u8* pixels = platform::MovieFramePixels(playback.movie);
    u32 width = std::min(smack->Width, playback.pitch > playback.left ? playback.pitch - playback.left : 0);
    u32 height = std::min(smack->Height,
                          playback.bufferHeight > playback.top ? playback.bufferHeight - playback.top : 0);
    for (u32 row = 0; row < height; row++)
        std::memcpy(playback.buffer + (playback.top + row) * playback.pitch + playback.left,
                    pixels + row * smack->Width, width);
    smack->LastRectx = static_cast<i32>(playback.left);
    smack->LastRecty = static_cast<i32>(playback.top);
    smack->LastRectw = static_cast<i32>(width);
    smack->LastRecth = static_cast<i32>(height);
    playback.rectPending = width > 0 && height > 0;
    return 0;
}

extern "C" u32 SmackToBufferRect(Smack* smack, u32) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end() || !found->second->rectPending)
        return 0;
    found->second->rectPending = false;
    return 1;
}

extern "C" void SmackNextFrame(Smack* smack) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end())
        return;
    Playback& playback = *found->second;
    smack->FrameNum++;
    if (smack->FrameNum >= smack->Frames) {
        smack->FrameNum = 0;
        playback.started = false;
        Restart(smack, playback);
        return;
    }
    DecodeAhead(smack, playback);
    if (!playback.frameReady) {
        // The container promised more frames than it holds.
        smack->Frames = smack->FrameNum;
        smack->FrameNum = 0;
        playback.started = false;
        Restart(smack, playback);
    }
}

// Nonzero while the current frame is not yet due.
extern "C" u32 SmackWait(Smack* smack) {
    auto found = gPlaybacks.find(smack);
    if (found == gPlaybacks.end())
        return 0;
    Playback& playback = *found->second;
    u32 now = platform::Ticks();
    if (!playback.started) {
        playback.started = true;
        playback.startTick = now;
    }
    u32 due = playback.startTick + smack->FrameNum * smack->MSPerFrame;
    if (static_cast<i32>(due - now) > 0) {
        platform::Sleep(1);
        return 1;
    }
    return 0;
}

extern "C" u8 SmackSoundUseMSS(void*) {
    return 1;
}

extern "C" void SmackSummary(Smack*, SmackSum* summary) {
    std::memset(summary, 0, sizeof(*summary));
}
