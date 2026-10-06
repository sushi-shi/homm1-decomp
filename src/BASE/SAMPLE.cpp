#include <H1/Ints.h>

#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <SOURCE/KB.h>

#include <stdlib.h>
#include <string.h>

enum SampleLoadConstant {
    SAMPLE_FILENAME_CAPACITY = 32,
    SAMPLE_FORMAT_SUFFIX_LENGTH = 3,
    SAMPLE_LOAD_RATE_11025 = 11025,
    SAMPLE_LOAD_RATE_22050 = 22050,
    SAMPLE_LOAD_RATE_44100 = 44100,
    SAMPLE_LOAD_FORMAT_8_BIT = 0,
    SAMPLE_LOAD_FORMAT_16_BIT = 1,
    SAMPLE_LOAD_STEREO = 2
};

sample::sample(char* name, i32 channelType, i32 volume, i32 loopCount)
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
    i32 stereo = SAMPLE_LOAD_STEREO;
    strcpy(fileName, name);
    strrev(fileName);
    for (i32 i = 0; i < SAMPLE_FORMAT_SUFFIX_LENGTH; i++) {
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
    u32 size = gpResourceManager->GetFileSize(m_id);
    m_playbackData.data = static_cast<i8*>(malloc(size));
    m_playbackData.size = size;
    gpResourceManager->PointToFile(m_id);
    gpResourceManager->ReadBlock(m_playbackData.data, size);
}

inline sample::~sample() {
    free(m_playbackData.data);
    m_playbackData.size = 0;
    m_playbackData.volume = 0;
}
