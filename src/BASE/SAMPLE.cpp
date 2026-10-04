// HoMM1 sample resource loader; Buka SAMPLE.cpp supplies the suffix decoding.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/KB.h>

#include <stdlib.h>
#include <string.h>

H1_ENUM_CONST_BEGIN(SampleLoadConstant)
    SAMPLE_FILENAME_CAPACITY = 32,
    SAMPLE_FORMAT_SUFFIX_LENGTH = 3,
    SAMPLE_LOAD_RATE_11025 = 11025,
    SAMPLE_LOAD_RATE_22050 = 22050,
    SAMPLE_LOAD_RATE_44100 = 44100,
    SAMPLE_LOAD_FORMAT_8_BIT = 0,
    SAMPLE_LOAD_FORMAT_16_BIT = 1,
    SAMPLE_LOAD_STEREO = 1
H1_ENUM_CONST_END(SampleLoadConstant)

VA_COMPGEN(0x00475340, 0x2e, "??_Gsample@@UAEPAXI@Z", 0x00475050)

VA(0x00475050, 0x232)
sample::sample(char* name)
    : resource(
          RESOURCE_CATEGORY_SAMPLE,
          gpResourceManager->MakeId(name),
          RESOURCE_REFERENCE_INITIAL,
          NULL
      ) {
    char fileName[SAMPLE_FILENAME_CAPACITY];
    m_playbackData.volume = SAMPLE_VOLUME_FULL;
    m_playbackData.repeat = 0;
    m_playbackData.stereo = SAMPLE_LOAD_STEREO;
    m_playbackData.sampleFormat = SAMPLE_LOAD_FORMAT_16_BIT;
    m_playbackData.sampleRate = SAMPLE_LOAD_RATE_44100;
    strcpy(fileName, name);
    strrev(fileName);
    i32 i;
    for (i = 0; i < SAMPLE_FORMAT_SUFFIX_LENGTH; i++) {
        switch (fileName[i]) {
            case '8':
                m_playbackData.sampleFormat = SAMPLE_LOAD_FORMAT_8_BIT;
                break;
            case '6':
                m_playbackData.sampleFormat = SAMPLE_LOAD_FORMAT_16_BIT;
                break;
            case '1':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_11025;
                break;
            case '2':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_22050;
                break;
            case '4':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_44100;
                break;
            case 'M':
            case 'm':
                m_playbackData.stereo = 0;
                break;
        }
    }
    u32 size = gpResourceManager->GetFileSize(m_id);
    m_playbackData.data = new i8[size];
    m_playbackData.size = size;
    gpResourceManager->PointToFile(m_id);
    gpResourceManager->ReadBlock(m_playbackData.data, size);
    for (i = 0; i < size; ++i)
        m_playbackData.data[i] += 0x80;
}

VA(0x00475282, 0x7f)
sample::~sample() {
    StopSample(this);
    delete[] m_playbackData.data;
    memset(&m_playbackData, 0, sizeof(m_playbackData));
}
