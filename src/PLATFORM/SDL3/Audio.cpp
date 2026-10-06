#include "Internal.h"
#include "Media.h"

#include <map>
#include <memory>
#include <mutex>
#include <vector>

namespace platform {

namespace {

SDL_AudioDeviceID gDevice = 0;

// ---------------------------------------------------------------- samples

struct VoiceState {
    SDL_AudioStream* stream = nullptr;
    const void* data = nullptr;
    int bytes = 0;
    bool loop = false;
};

std::map<Voice, VoiceState> gVoices;
Voice gNextVoice = 1;

void SDLCALL RefillLoop(void* userdata, SDL_AudioStream* stream, int additional, int) {
    const VoiceState* voice = static_cast<const VoiceState*>(userdata);
    while (additional > 0 && voice->bytes > 0) {
        SDL_PutAudioStreamData(stream, voice->data, voice->bytes);
        additional -= voice->bytes;
    }
}

bool StreamBusy(SDL_AudioStream* stream) {
    return SDL_GetAudioStreamQueued(stream) > 0 || SDL_GetAudioStreamAvailable(stream) > 0;
}

void ReapVoices() {
    for (auto it = gVoices.begin(); it != gVoices.end();) {
        if (!it->second.loop && !StreamBusy(it->second.stream)) {
            SDL_DestroyAudioStream(it->second.stream);
            it = gVoices.erase(it);
        } else {
            ++it;
        }
    }
}

// ---------------------------------------------------------------- music

struct MusicState {
    std::mutex lock;
    media::Decoder decoder;
    SDL_AudioStream* stream = nullptr;
    bool loop = false;
    bool finished = true;
    i64 framesOut = 0;
    double startSeconds = 0;
    std::vector<i16> buffer;
};

MusicState gMusic;

void SDLCALL FeedMusic(void*, SDL_AudioStream* stream, int additional, int) {
    std::lock_guard<std::mutex> guard(gMusic.lock);
    while (additional > 0 && !gMusic.finished) {
        gMusic.buffer.clear();
        if (!gMusic.decoder.Next(nullptr, nullptr, gMusic.buffer)) {
            if (gMusic.loop && gMusic.decoder.SeekSeconds(0)) {
                gMusic.framesOut = 0;
                gMusic.startSeconds = 0;
                continue;
            }
            gMusic.finished = true;
            break;
        }
        int bytes = static_cast<int>(gMusic.buffer.size() * sizeof(i16));
        SDL_PutAudioStreamData(stream, gMusic.buffer.data(), bytes);
        gMusic.framesOut += static_cast<i64>(gMusic.buffer.size() / media::kMediaChannels);
        additional -= bytes;
    }
}

SDL_AudioSpec MediaSpec() {
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_S16;
    spec.channels = media::kMediaChannels;
    spec.freq = media::kMediaRate;
    return spec;
}

}  // namespace

// ---------------------------------------------------------------- device

bool OpenAudio() {
    if (gDevice != 0)
        return true;
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        Log("no audio: %s", SDL_GetError());
        return false;
    }
    gDevice = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (gDevice == 0) {
        Log("no audio device: %s", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return false;
    }
    return true;
}

void CloseAudio() {
    if (gDevice == 0)
        return;
    StopMusic();
    for (auto& entry : gVoices)
        SDL_DestroyAudioStream(entry.second.stream);
    gVoices.clear();
    SDL_CloseAudioDevice(gDevice);
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    gDevice = 0;
}

bool AudioOpen() {
    return gDevice != 0;
}

SDL_AudioStream* OpenMediaStream() {
    if (gDevice == 0)
        return nullptr;
    SDL_AudioSpec spec = MediaSpec();
    SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, nullptr);
    if (stream != nullptr && !SDL_BindAudioStream(gDevice, stream)) {
        SDL_DestroyAudioStream(stream);
        return nullptr;
    }
    return stream;
}

// ---------------------------------------------------------------- samples

Voice PlaySample(
    const void* data,
    std::size_t bytes,
    int rate,
    int channels,
    SampleFormat format,
    float volume,
    bool loop
) {
    if (gDevice == 0 || data == nullptr || bytes == 0 || rate <= 0 || channels <= 0)
        return NO_VOICE;
    ReapVoices();
    SDL_AudioSpec spec;
    spec.format = format == SAMPLE_U8 ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
    spec.channels = channels;
    spec.freq = rate;
    SDL_AudioStream* stream = SDL_CreateAudioStream(&spec, nullptr);
    if (stream == nullptr)
        return NO_VOICE;
    Voice id = gNextVoice++;
    VoiceState& voice = gVoices[id];
    voice.stream = stream;
    voice.data = data;
    voice.bytes = static_cast<int>(bytes);
    voice.loop = loop;
    SDL_SetAudioStreamGain(stream, volume);
    if (loop)
        SDL_SetAudioStreamGetCallback(stream, RefillLoop, &voice);
    SDL_PutAudioStreamData(stream, data, voice.bytes);
    if (!loop)
        SDL_FlushAudioStream(stream);
    if (!SDL_BindAudioStream(gDevice, stream)) {
        SDL_DestroyAudioStream(stream);
        gVoices.erase(id);
        return NO_VOICE;
    }
    return id;
}

void StopVoice(Voice voice) {
    auto found = gVoices.find(voice);
    if (found == gVoices.end())
        return;
    SDL_DestroyAudioStream(found->second.stream);
    gVoices.erase(found);
}

bool VoicePlaying(Voice voice) {
    auto found = gVoices.find(voice);
    if (found == gVoices.end())
        return false;
    return found->second.loop || StreamBusy(found->second.stream);
}

void SetVoiceVolume(Voice voice, float volume) {
    auto found = gVoices.find(voice);
    if (found != gVoices.end())
        SDL_SetAudioStreamGain(found->second.stream, volume);
}

// ---------------------------------------------------------------- music

bool PlayMusic(const char* hostPath, bool loop, double startSeconds, float volume) {
    StopMusic();
    if (gDevice == 0)
        return false;
    {
        std::lock_guard<std::mutex> guard(gMusic.lock);
        if (!gMusic.decoder.Open(hostPath, false, true))
            return false;
        if (startSeconds > 0 && !gMusic.decoder.SeekSeconds(startSeconds))
            startSeconds = 0;
        gMusic.loop = loop;
        gMusic.finished = false;
        gMusic.framesOut = 0;
        gMusic.startSeconds = startSeconds;
    }
    gMusic.stream = OpenMediaStream();
    if (gMusic.stream == nullptr) {
        std::lock_guard<std::mutex> guard(gMusic.lock);
        gMusic.decoder.Close();
        gMusic.finished = true;
        return false;
    }
    SDL_SetAudioStreamGain(gMusic.stream, volume);
    SDL_SetAudioStreamGetCallback(gMusic.stream, FeedMusic, nullptr);
    return true;
}

void StopMusic() {
    if (gMusic.stream != nullptr) {
        SDL_DestroyAudioStream(gMusic.stream);
        gMusic.stream = nullptr;
    }
    std::lock_guard<std::mutex> guard(gMusic.lock);
    gMusic.decoder.Close();
    gMusic.finished = true;
}

bool MusicPlaying() {
    if (gMusic.stream == nullptr)
        return false;
    std::lock_guard<std::mutex> guard(gMusic.lock);
    return !gMusic.finished || StreamBusy(gMusic.stream);
}

double MusicPosition() {
    std::lock_guard<std::mutex> guard(gMusic.lock);
    return gMusic.startSeconds + static_cast<double>(gMusic.framesOut) / media::kMediaRate;
}

void SetMusicVolume(float volume) {
    if (gMusic.stream != nullptr)
        SDL_SetAudioStreamGain(gMusic.stream, volume);
}

// ---------------------------------------------------------------- movies

struct Movie {
    media::Decoder decoder;
    MovieInfo info;
    std::vector<u8> pixels;
    std::vector<u8> palette;
    std::vector<i16> audio;
    SDL_AudioStream* stream = nullptr;
};

Movie* OpenMovie(const char* hostPath, bool withAudio, float volume) {
    auto movie = std::make_unique<Movie>();
    if (!movie->decoder.Open(hostPath, true, withAudio && gDevice != 0))
        return nullptr;
    movie->info.width = movie->decoder.Width();
    movie->info.height = movie->decoder.Height();
    movie->info.frames = movie->decoder.FrameCount();
    movie->info.framesPerSecond = movie->decoder.FramesPerSecond();
    movie->pixels.assign(static_cast<size_t>(movie->info.width * movie->info.height), 0);
    movie->palette.assign(256 * 3, 0);
    if (movie->decoder.HasAudio()) {
        movie->stream = OpenMediaStream();
        if (movie->stream != nullptr)
            SDL_SetAudioStreamGain(movie->stream, volume);
    }
    return movie.release();
}

void CloseMovie(Movie* movie) {
    if (movie == nullptr)
        return;
    if (movie->stream != nullptr)
        SDL_DestroyAudioStream(movie->stream);
    delete movie;
}

const MovieInfo& MovieDetails(const Movie* movie) {
    return movie->info;
}

bool DecodeMovieFrame(Movie* movie) {
    movie->audio.clear();
    bool decoded = movie->decoder.Next(&movie->pixels, &movie->palette, movie->audio);
    if (movie->stream != nullptr && !movie->audio.empty())
        SDL_PutAudioStreamData(movie->stream, movie->audio.data(),
                               static_cast<int>(movie->audio.size() * sizeof(i16)));
    return decoded;
}

const u8* MovieFramePixels(const Movie* movie) {
    return movie->pixels.data();
}

const u8* MovieFramePalette(const Movie* movie) {
    return movie->palette.data();
}

}  // namespace platform
