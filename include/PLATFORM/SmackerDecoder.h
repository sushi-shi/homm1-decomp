#ifndef HOMM1_PLATFORM_SMACKERDECODER_H
#define HOMM1_PLATFORM_SMACKERDECODER_H

// A decoder for RAD Game Tools' Smacker movies (SMK2 and SMK4), the format of
// the game's ANIM\*.SMK: 8-bit video with a palette and Smacker's own audio
// (Huffman-coded differences, or plain PCM). Bink audio tracks of SMK4 files
// are not decoded. The whole file is held in memory; damaged data ends the
// movie, never reads outside it.

#include <H1/Ints.h>

#include <array>
#include <string>
#include <vector>

namespace platform::smacker {

constexpr int kAudioTracks = 7;

class BitReader;

struct AudioFormat {
    bool present = false;
    bool compressed = false;   // Smacker's Huffman-coded differences
    bool sixteenBit = false;   // signed 16-bit little-endian, else unsigned 8-bit
    bool stereo = false;
    bool decodable = false;    // not a Bink audio track
    int rate = 0;
};

class Decoder {
public:
    // False when the data is not a Smacker file or its header is damaged.
    bool Open(std::vector<u8> file);
    bool IsOpen() const { return !m_file.empty(); }

    int Width() const { return m_width; }
    int Height() const { return m_height; }
    int FrameCount() const { return m_frames; }
    // The frame the next DecodeFrame call decodes.
    int CurrentFrame() const { return m_current; }
    double FramesPerSecond() const { return m_framesPerSecond; }
    const AudioFormat& Audio(int track) const { return m_audio[static_cast<size_t>(track)]; }

    // Decodes the next frame into Pixels() and Palette(), and appends the
    // samples of `audioTrack` in that frame (in the track's format,
    // interleaved) to audio (pass -1 for none). False after the last frame
    // or on damaged data.
    bool DecodeFrame(int audioTrack, std::vector<u8>* audio);
    // Back to the first frame (the picture and palette start black).
    void Rewind();

    // Width() * Height() palette indexes, one byte each, row by row.
    const std::vector<u8>& Pixels() const { return m_pixels; }
    // 256 RGB triplets.
    const std::array<u8, 768>& Palette() const { return m_palette; }

private:
    // A Huffman tree of 16-bit values with the three most recent values
    // cached in its escape leaves (Smacker's "big tree").
    struct BigTree {
        std::vector<u32> values;   // nodes: kNode | size of the left subtree
        std::array<size_t, 3> last = {0, 0, 0};
    };

    bool ReadTrees(const u8* data, size_t size);
    bool ReadBigTree(BitReader& bits, BigTree& tree, u32 sizeHint);
    void UpdatePalette(const u8* data, size_t size);
    bool DecodeAudio(const u8* data, size_t size, const AudioFormat& format, std::vector<u8>& out);
    bool DecodeVideo(const u8* data, size_t size);

    std::vector<u8> m_file;
    int m_width = 0;
    int m_height = 0;
    int m_frames = 0;
    double m_framesPerSecond = 10.0;
    bool m_version4 = false;
    std::array<AudioFormat, kAudioTracks> m_audio;
    std::vector<u32> m_frameSizes;
    std::vector<u8> m_frameTypes;
    size_t m_firstFrame = 0;
    size_t m_position = 0;
    int m_current = 0;
    BigTree m_mmap;
    BigTree m_mclr;
    BigTree m_full;
    BigTree m_type;
    std::vector<u8> m_pixels;
    std::array<u8, 768> m_palette = {};
};

}  // namespace platform::smacker

#endif
