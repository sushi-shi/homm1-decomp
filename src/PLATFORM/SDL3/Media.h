#ifndef HOMM1_PLATFORM_SDL3_MEDIA_H
#define HOMM1_PLATFORM_SDL3_MEDIA_H

// Decoding for the backend: Smacker movies (PLATFORM/SmackerDecoder.h) and
// Ogg Vorbis music (stb_vorbis). Audio comes out as interleaved signed
// 16-bit stereo at kMediaRate, converted by SDL.

#include <H1/Ints.h>

#include <PLATFORM/SmackerDecoder.h>

#include <string>
#include <vector>

struct SDL_AudioStream;
struct stb_vorbis;

namespace platform::media {

constexpr int kMediaRate = 44100;
constexpr int kMediaChannels = 2;

class Decoder {
public:
    Decoder() = default;
    ~Decoder();
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    // A Smacker movie (wantVideo) or Ogg Vorbis music, by the file's
    // signature. hostPath is UTF-8.
    bool Open(const std::string& hostPath, bool wantVideo, bool wantAudio);
    void Close();

    bool HasVideo() const { return m_movie.IsOpen(); }
    bool HasAudio() const { return m_converter != nullptr; }
    int Width() const { return m_movie.Width(); }
    int Height() const { return m_movie.Height(); }
    int FrameCount() const { return m_movie.FrameCount(); }
    double FramesPerSecond() const { return m_movie.IsOpen() ? m_movie.FramesPerSecond() : 15.0; }

    // Decodes the next video frame (copied to pixels and palette, as RGB
    // triplets) or, without video, some audio. Decoded audio is appended to
    // audio. False at the end of the stream.
    bool Next(std::vector<u8>* pixels, std::vector<u8>* palette, std::vector<i16>& audio);
    bool SeekSeconds(double seconds);

private:
    bool OpenConverter(int format, int channels, int rate);
    void Convert(const void* data, int bytes, std::vector<i16>& audio);
    void Drain(std::vector<i16>& audio);

    std::vector<u8> m_file;
    smacker::Decoder m_movie;
    int m_movieTrack = -1;
    std::vector<u8> m_movieAudio;
    stb_vorbis* m_music = nullptr;
    int m_musicRate = 0;
    int m_musicChannels = 2;
    SDL_AudioStream* m_converter = nullptr;
    bool m_ended = false;
};

}  // namespace platform::media

#endif
