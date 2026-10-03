// HoMM1 sample resource loader; Buka SAMPLE.cpp supplies the suffix decoding.

#include <match.h>

#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <H1/KB.h>

#include <stdlib.h>
#include <string.h>


H1_ENUM_CONST_BEGIN(SampleLoadConstant)
SAMPLE_FILENAME_CAPACITY = 32, SAMPLE_FORMAT_SUFFIX_LENGTH = 3, SAMPLE_LOAD_RATE_11025 = 11025,
                               SAMPLE_LOAD_RATE_22050 = 22050, SAMPLE_LOAD_RATE_44100 = 44100,
                               SAMPLE_LOAD_FORMAT_8_BIT = 0, SAMPLE_LOAD_FORMAT_16_BIT = 1,
                               SAMPLE_LOAD_STEREO = 2 H1_ENUM_CONST_END(SampleLoadConstant)

                                   VA(0x0047fa60, 0x17d)
sample::sample(char* name, long channelType, long volume, long loopCount)
    : resource(
          RESOURCE_CATEGORY_SAMPLE,
          gpResourceManager->MakeId(name),
          RESOURCE_REFERENCE_INITIAL,
          NULL
      ) {
    char fileName[SAMPLE_FILENAME_CAPACITY];
    m_playbackData.channelType = channelType;
    m_playbackData.volume = volume;
    m_playbackData.loopCount = loopCount;
    int stereo = SAMPLE_LOAD_STEREO;
    strcpy(fileName, name);
    strrev(fileName);
    for (int i = 0; i < SAMPLE_FORMAT_SUFFIX_LENGTH; i++) {
        switch (fileName[i]) {
            case '1':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_11025;
                break;
            case '2':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_22050;
                break;
            case '4':
                m_playbackData.sampleRate = SAMPLE_LOAD_RATE_44100;
                break;
            case '6':
                m_playbackData.format = SAMPLE_LOAD_FORMAT_16_BIT;
                break;
            case '8':
                m_playbackData.format = SAMPLE_LOAD_FORMAT_8_BIT;
                break;
            case 'M':
            case 'm':
                stereo = 0;
                break;
        }
    }
    m_playbackData.format += stereo;
    unsigned long size = gpResourceManager->GetFileSize(m_id);
    m_playbackData.data = static_cast<signed char*>(malloc(size));
    m_playbackData.size = size;
    gpResourceManager->PointToFile(m_id);
    gpResourceManager->ReadBlock(m_playbackData.data, size);
}

// Retail has no out-of-line ~sample: the scalar deleting destructor at
// 0x0047fbe0 expands this body between the vptr reset and ~resource.
VA_COMPGEN(0x0047fbe0, 0x3b, "??_Gsample@@UAEPAXI@Z", 0x0047fa60)
inline sample::~sample() {
    free(m_playbackData.data);
    m_playbackData.size = 0;
    m_playbackData.volume = 0;
}
