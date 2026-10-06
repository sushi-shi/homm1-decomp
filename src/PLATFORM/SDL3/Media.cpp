#include "Media.h"

#include <PLATFORM/Platform.h>

#include <SDL3/SDL.h>

#define STB_VORBIS_HEADER_ONLY
#include <stb_vorbis.c>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace platform::media {

namespace {

constexpr int kMusicChunkFrames = 4096;

bool ReadHostFile(const std::string& hostPath, std::vector<u8>& data) {
    std::ifstream file(std::filesystem::path(std::u8string(hostPath.begin(), hostPath.end())),
                       std::ios::binary);
    if (!file)
        return false;
    data.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return !file.bad();
}

}  // namespace

Decoder::~Decoder() {
    Close();
}

void Decoder::Close() {
    if (m_converter != nullptr) {
        SDL_DestroyAudioStream(m_converter);
        m_converter = nullptr;
    }
    if (m_music != nullptr) {
        stb_vorbis_close(m_music);
        m_music = nullptr;
    }
    m_movie = smacker::Decoder();
    m_movieTrack = -1;
    m_file.clear();
    m_ended = false;
}

bool Decoder::OpenConverter(int format, int channels, int rate) {
    SDL_AudioSpec source;
    source.format = static_cast<SDL_AudioFormat>(format);
    source.channels = channels;
    source.freq = rate;
    SDL_AudioSpec target;
    target.format = SDL_AUDIO_S16;
    target.channels = kMediaChannels;
    target.freq = kMediaRate;
    m_converter = SDL_CreateAudioStream(&source, &target);
    return m_converter != nullptr;
}

bool Decoder::Open(const std::string& hostPath, bool wantVideo, bool wantAudio) {
    Close();
    std::vector<u8> file;
    if (!ReadHostFile(hostPath, file) || file.size() < 4)
        return false;
    if (wantVideo) {
        if (!m_movie.Open(std::move(file)))
            return false;
        // The first track the decoder can play, as the game's movies have.
        for (int track = 0; wantAudio && track < smacker::kAudioTracks; track++) {
            const smacker::AudioFormat& format = m_movie.Audio(track);
            if (!format.decodable)
                continue;
            if (OpenConverter(format.sixteenBit ? SDL_AUDIO_S16LE : SDL_AUDIO_U8,
                              format.stereo ? 2 : 1, format.rate))
                m_movieTrack = track;
            break;
        }
        return true;
    }
    if (!wantAudio || std::memcmp(file.data(), "OggS", 4) != 0)
        return false;
    m_file = std::move(file);
    int error = 0;
    m_music = stb_vorbis_open_memory(m_file.data(), static_cast<int>(m_file.size()), &error, nullptr);
    if (m_music == nullptr) {
        Close();
        return false;
    }
    stb_vorbis_info info = stb_vorbis_get_info(m_music);
    m_musicRate = static_cast<int>(info.sample_rate);
    // Mono stays mono until SDL spreads it to both channels; more than two
    // channels are mixed down to two by the decoder.
    m_musicChannels = info.channels == 1 ? 1 : 2;
    if (m_musicRate <= 0 || !OpenConverter(SDL_AUDIO_S16, m_musicChannels, m_musicRate)) {
        Close();
        return false;
    }
    return true;
}

void Decoder::Convert(const void* data, int bytes, std::vector<i16>& audio) {
    if (m_converter == nullptr || bytes <= 0)
        return;
    SDL_PutAudioStreamData(m_converter, data, bytes);
    int available = SDL_GetAudioStreamAvailable(m_converter);
    if (available <= 0)
        return;
    size_t start = audio.size();
    audio.resize(start + static_cast<size_t>(available) / sizeof(i16));
    int got = SDL_GetAudioStreamData(m_converter, audio.data() + start, available);
    audio.resize(start + static_cast<size_t>(got > 0 ? got : 0) / sizeof(i16));
}

void Decoder::Drain(std::vector<i16>& audio) {
    if (m_converter == nullptr)
        return;
    SDL_FlushAudioStream(m_converter);
    Convert(nullptr, 0, audio);
    int available = SDL_GetAudioStreamAvailable(m_converter);
    if (available > 0) {
        size_t start = audio.size();
        audio.resize(start + static_cast<size_t>(available) / sizeof(i16));
        int got = SDL_GetAudioStreamData(m_converter, audio.data() + start, available);
        audio.resize(start + static_cast<size_t>(got > 0 ? got : 0) / sizeof(i16));
    }
}

bool Decoder::Next(std::vector<u8>* pixels, std::vector<u8>* palette, std::vector<i16>& audio) {
    if (m_ended)
        return false;
    if (m_movie.IsOpen()) {
        m_movieAudio.clear();
        if (!m_movie.DecodeFrame(m_movieTrack, m_movieTrack >= 0 ? &m_movieAudio : nullptr)) {
            m_ended = true;
            Drain(audio);
            return false;
        }
        Convert(m_movieAudio.data(), static_cast<int>(m_movieAudio.size()), audio);
        if (pixels != nullptr)
            pixels->assign(m_movie.Pixels().begin(), m_movie.Pixels().end());
        if (palette != nullptr)
            palette->assign(m_movie.Palette().begin(), m_movie.Palette().end());
        return true;
    }
    if (m_music == nullptr)
        return false;
    i16 samples[kMusicChunkFrames * 2];
    int frames = stb_vorbis_get_samples_short_interleaved(m_music, m_musicChannels, samples,
                                                          kMusicChunkFrames * m_musicChannels);
    if (frames <= 0) {
        m_ended = true;
        Drain(audio);
        return false;
    }
    Convert(samples, frames * m_musicChannels * static_cast<int>(sizeof(i16)), audio);
    return true;
}

bool Decoder::SeekSeconds(double seconds) {
    if (m_converter != nullptr)
        SDL_ClearAudioStream(m_converter);
    m_ended = false;
    if (seconds < 0)
        seconds = 0;
    if (m_music != nullptr)
        return stb_vorbis_seek(m_music, static_cast<unsigned int>(seconds * m_musicRate)) != 0;
    if (!m_movie.IsOpen())
        return false;
    // Movies only go back to the start, then forward frame by frame.
    m_movie.Rewind();
    int target = static_cast<int>(seconds * m_movie.FramesPerSecond());
    while (m_movie.CurrentFrame() < target && m_movie.DecodeFrame(-1, nullptr)) {
    }
    return true;
}

}  // namespace platform::media
