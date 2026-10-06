#ifndef HOMM1_PLATFORM_SDL3_MEDIA_H
#define HOMM1_PLATFORM_SDL3_MEDIA_H

// FFmpeg demuxing and decoding for the backend: Ogg Vorbis music and Smacker
// movies. Audio comes out as interleaved signed 16-bit stereo at kMediaRate.

#include <H1/Ints.h>

#include <string>
#include <vector>

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVPacket;
struct SwrContext;

namespace platform::media {

constexpr int kMediaRate = 44100;
constexpr int kMediaChannels = 2;

class Decoder {
public:
    Decoder() = default;
    ~Decoder();
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;

    bool Open(const std::string& path, bool wantVideo, bool wantAudio);
    void Close();

    bool HasVideo() const { return m_video != nullptr; }
    bool HasAudio() const { return m_audio != nullptr; }
    int Width() const;
    int Height() const;
    int FrameCount() const;
    double FramesPerSecond() const;

    // Reads packets until a video frame is decoded (copied to pixels and
    // palette) or, with no video, until some audio is decoded. Decoded audio
    // is appended to audio. False at the end of the stream.
    bool Next(std::vector<u8>* pixels, std::vector<u8>* palette, std::vector<i16>& audio);
    bool SeekSeconds(double seconds);

private:
    bool DecodeAudio(std::vector<i16>& audio);
    bool DecodeVideo(std::vector<u8>* pixels, std::vector<u8>* palette);

    AVFormatContext* m_format = nullptr;
    AVCodecContext* m_video = nullptr;
    AVCodecContext* m_audio = nullptr;
    SwrContext* m_resampler = nullptr;
    AVFrame* m_frame = nullptr;
    AVPacket* m_packet = nullptr;
    int m_videoStream = -1;
    int m_audioStream = -1;
    bool m_draining = false;
};

}  // namespace platform::media

#endif
