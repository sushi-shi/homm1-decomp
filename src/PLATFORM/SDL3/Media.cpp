#include "Media.h"

#include <PLATFORM/Platform.h>

#include <cstring>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}

namespace platform::media {

namespace {

AVCodecContext* OpenCodec(AVFormatContext* format, int stream) {
    const AVCodecParameters* parameters = format->streams[stream]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(parameters->codec_id);
    if (codec == nullptr)
        return nullptr;
    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (context == nullptr)
        return nullptr;
    if (avcodec_parameters_to_context(context, parameters) < 0
        || avcodec_open2(context, codec, nullptr) < 0) {
        avcodec_free_context(&context);
        return nullptr;
    }
    return context;
}

}  // namespace

Decoder::~Decoder() {
    Close();
}

void Decoder::Close() {
    if (m_resampler != nullptr)
        swr_free(&m_resampler);
    if (m_video != nullptr)
        avcodec_free_context(&m_video);
    if (m_audio != nullptr)
        avcodec_free_context(&m_audio);
    if (m_frame != nullptr)
        av_frame_free(&m_frame);
    if (m_packet != nullptr)
        av_packet_free(&m_packet);
    if (m_format != nullptr)
        avformat_close_input(&m_format);
    m_videoStream = -1;
    m_audioStream = -1;
    m_draining = false;
}

bool Decoder::Open(const std::string& path, bool wantVideo, bool wantAudio) {
    Close();
    av_log_set_level(AV_LOG_ERROR);
    if (avformat_open_input(&m_format, path.c_str(), nullptr, nullptr) < 0) {
        m_format = nullptr;
        return false;
    }
    if (avformat_find_stream_info(m_format, nullptr) < 0) {
        Close();
        return false;
    }
    if (wantVideo) {
        m_videoStream = av_find_best_stream(m_format, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (m_videoStream >= 0)
            m_video = OpenCodec(m_format, m_videoStream);
        if (m_video == nullptr) {
            Close();
            return false;
        }
    }
    if (wantAudio) {
        m_audioStream = av_find_best_stream(m_format, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (m_audioStream >= 0)
            m_audio = OpenCodec(m_format, m_audioStream);
        if (m_audio != nullptr) {
            AVChannelLayout stereo;
            av_channel_layout_default(&stereo, kMediaChannels);
            if (swr_alloc_set_opts2(&m_resampler, &stereo, AV_SAMPLE_FMT_S16, kMediaRate,
                                    &m_audio->ch_layout, m_audio->sample_fmt,
                                    m_audio->sample_rate, 0, nullptr)
                    < 0
                || swr_init(m_resampler) < 0) {
                avcodec_free_context(&m_audio);
                swr_free(&m_resampler);
            }
        }
        if (m_audio == nullptr && !wantVideo) {
            Close();
            return false;
        }
    }
    m_frame = av_frame_alloc();
    m_packet = av_packet_alloc();
    return m_frame != nullptr && m_packet != nullptr;
}

int Decoder::Width() const {
    return m_video != nullptr ? m_video->width : 0;
}

int Decoder::Height() const {
    return m_video != nullptr ? m_video->height : 0;
}

int Decoder::FrameCount() const {
    if (m_videoStream < 0)
        return 0;
    const AVStream* stream = m_format->streams[m_videoStream];
    if (stream->nb_frames > 0)
        return static_cast<int>(stream->nb_frames);
    return 0;
}

double Decoder::FramesPerSecond() const {
    if (m_videoStream < 0)
        return 15.0;
    const AVStream* stream = m_format->streams[m_videoStream];
    AVRational rate = stream->avg_frame_rate.num > 0 ? stream->avg_frame_rate : stream->r_frame_rate;
    if (rate.num <= 0 || rate.den <= 0)
        return 15.0;
    return av_q2d(rate);
}

bool Decoder::DecodeAudio(std::vector<i16>& audio) {
    bool produced = false;
    while (avcodec_receive_frame(m_audio, m_frame) == 0) {
        int capacity = swr_get_out_samples(m_resampler, m_frame->nb_samples);
        if (capacity <= 0) {
            av_frame_unref(m_frame);
            continue;
        }
        size_t start = audio.size();
        audio.resize(start + static_cast<size_t>(capacity * kMediaChannels));
        u8* output = reinterpret_cast<u8*>(audio.data() + start);
        int converted = swr_convert(m_resampler, &output, capacity,
                                    const_cast<const u8**>(m_frame->extended_data),
                                    m_frame->nb_samples);
        audio.resize(start + static_cast<size_t>(std::max(converted, 0) * kMediaChannels));
        produced = produced || converted > 0;
        av_frame_unref(m_frame);
    }
    return produced;
}

bool Decoder::DecodeVideo(std::vector<u8>* pixels, std::vector<u8>* palette) {
    if (avcodec_receive_frame(m_video, m_frame) != 0)
        return false;
    int width = m_frame->width;
    int height = m_frame->height;
    if (pixels != nullptr && m_frame->format == AV_PIX_FMT_PAL8) {
        pixels->resize(static_cast<size_t>(width * height));
        for (int row = 0; row < height; row++)
            std::memcpy(pixels->data() + static_cast<size_t>(row * width),
                        m_frame->data[0] + static_cast<ptrdiff_t>(row) * m_frame->linesize[0],
                        static_cast<size_t>(width));
        if (palette != nullptr) {
            palette->resize(256 * 3);
            for (int i = 0; i < 256; i++) {
                u32 entry;
                std::memcpy(&entry, m_frame->data[1] + i * 4, sizeof(entry));
                (*palette)[static_cast<size_t>(i * 3)] = static_cast<u8>(entry >> 16);
                (*palette)[static_cast<size_t>(i * 3 + 1)] = static_cast<u8>(entry >> 8);
                (*palette)[static_cast<size_t>(i * 3 + 2)] = static_cast<u8>(entry);
            }
        }
    }
    av_frame_unref(m_frame);
    return true;
}

bool Decoder::Next(std::vector<u8>* pixels, std::vector<u8>* palette, std::vector<i16>& audio) {
    if (m_format == nullptr)
        return false;
    for (;;) {
        if (m_video != nullptr && DecodeVideo(pixels, palette))
            return true;
        if (m_video == nullptr && m_audio != nullptr && DecodeAudio(audio))
            return true;
        if (m_draining)
            return false;
        int result = av_read_frame(m_format, m_packet);
        if (result < 0) {
            m_draining = true;
            if (m_video != nullptr)
                avcodec_send_packet(m_video, nullptr);
            if (m_audio != nullptr) {
                avcodec_send_packet(m_audio, nullptr);
                DecodeAudio(audio);
            }
            continue;
        }
        if (m_packet->stream_index == m_videoStream && m_video != nullptr) {
            avcodec_send_packet(m_video, m_packet);
        } else if (m_packet->stream_index == m_audioStream && m_audio != nullptr) {
            avcodec_send_packet(m_audio, m_packet);
            if (m_video != nullptr)
                DecodeAudio(audio);
        }
        av_packet_unref(m_packet);
    }
}

bool Decoder::SeekSeconds(double seconds) {
    if (m_format == nullptr)
        return false;
    i64 target = static_cast<i64>(seconds * AV_TIME_BASE);
    if (av_seek_frame(m_format, -1, target, AVSEEK_FLAG_BACKWARD) < 0)
        return false;
    if (m_audio != nullptr)
        avcodec_flush_buffers(m_audio);
    if (m_video != nullptr)
        avcodec_flush_buffers(m_video);
    m_draining = false;
    return true;
}

}  // namespace platform::media
