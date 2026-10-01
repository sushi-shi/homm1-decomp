// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <BASE/soundmgr.h>

#include <windows.h>

#include <BASE/Misc.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/NOOPT.h>

#include <io.h>
#include <mss.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma intrinsic(strcpy, memset)

short gSoundManagerAssertLine = 0;
char gSoundManagerAssertFile1[] = "D:\\Heroes\\Base\\Soundmgr.cpp";
char gSoundManagerAssertFile2[] = "D:\\Heroes\\Base\\Soundmgr.cpp";
char gSoundManagerAssertFile3[] = "D:\\Heroes\\Base\\Soundmgr.cpp";
char gSoundManagerAssertFile4[] = "D:\\Heroes\\Base\\Soundmgr.cpp";
char gSoundManagerAssertFile5[] = "D:\\Heroes\\Base\\Soundmgr.cpp";

// PoL preserves this helper family; retail expands these helpers in CDPlay.
inline void HandleMCIError(int errorCode, char* command) {
    mciGetErrorStringA(errorCode, lpszReturnString, CD_MCI_RESULT_LAST);
    sprintf(
        gText,
        "CD MUSIC ERROR\n\nDescription '%s'\n\nCommand '%s'\n\n\nBecause of this problem running "
        "with CD stereo music, Heroes has been configured to run with 8 bit mono music in the "
        "future.  You can always manually change this setting in the control panel within the "
        "game.",
        lpszReturnString,
        command
    );
    gConfig.musicSource = 0;
    WritePrefs();
    ShutDown(gText);
}

inline void soundManager::ValidatePreviousPosition(int track) {
    char buffer[CD_POSITION_BUFFER_SIZE];
    char* separator;
    ProcessAssert(
        track >= 0 && track < MUSIC_TRACK_COUNT,
        gCDPositionAssertFile,
        gCDPositionAssertLine + 4
    );
    if (CDPreviousPosition[track][0] == 0)
        return;
    strcpy(buffer, CDPreviousPosition[track]);
    separator = FindToken(buffer, ':');
    if (separator != NULL)
        *separator = 0;
    if (atoi(buffer) != track)
        CDPreviousPosition[track][0] = 0;
}

inline void soundManager::CDSetVolume(int volume, int fadeScale) {
    int level;
    unsigned long stereoVolume;
    if (gbNoSound != 0 || m_auxDevice == -1)
        return;
    if (volume == -1)
        level = gConfig.musicVolume;
    else
        level = volume;
    if (level != 0) {
        int channel;
        if (fadeScale != 0)
            channel =
                CD_VOLUME_LEVEL_COUNT - (MUSIC_FADE_TOTAL_STEPS - level / CD_VOLUME_LEVEL_COUNT);
        else
            channel = CD_VOLUME_LEVEL_COUNT - level;
        channel <<= CD_VOLUME_LEVEL_SHIFT;
        stereoVolume = channel << CD_STEREO_CHANNEL_SHIFT | channel;
    } else {
        stereoVolume = 0;
    }
    auxSetVolume(m_auxDevice, stereoVolume);
}

VA(0x00476f00, 0x20e)
void soundManager::CDStop(void) {
    char position[CD_POSITION_BUFFER_SIZE];
    if (gbNoSound != 0)
        return;
    wsprintfA(CommandString, "stop CD");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
    if (strcmpi(lpszReturnString, "stopped") != 0) {
        wsprintfA(CommandString, "status CD position");
        nMCIError = mciSendStringA(CommandString, position, sizeof(position), NULL);
        if (nMCIError != 0)
            HandleMCIError(nMCIError, CommandString);
        strcpy(CDPreviousPosition[CDTrackMap[m_currentTrack]], position);
        ValidatePreviousPosition(CDTrackMap[m_currentTrack]);
    }
    CDPlaying = 0;
}

// PoL keeps this out of line; HoMM1 retains only its expansions.
inline int soundManager::CDIsPlaying(void) {
    if (gbNoSound != 0)
        return 0;
    wsprintfA(CommandString, "status CD mode");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
    return strcmpi(lpszReturnString, "playing") == 0;
}

VA(0x00477110, 0xd7)
unsigned long soundManager::CDStartup(void) {
    int numDevices;
    int device;
    if (gbNoSound != 0)
        return 0;
    wsprintfA(CommandString, "open %c: type cdaudio alias CD shareable", gcSoundPath[0]);
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0) {
        m_cdStarted = 0;
        gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
        m_cdReady = 0;
        WritePrefs();
        return 0;
    }
    m_cdStarted = 1;
    numDevices = auxGetNumDevs();
    m_auxDevice = -1;
    for (device = 0; device < numDevices; device++) {
        memset(&gAuxCaps, 0, sizeof(gAuxCaps));
        auxGetDevCapsA(device, &gAuxCaps, sizeof(gAuxCaps));
        if (gAuxCaps.wTechnology == AUXCAPS_CDAUDIO) {
            m_auxDevice = static_cast<short>(device);
            break;
        }
    }
    return nMCIError;
}

VA(0x004771f0, 0x5b7)
void soundManager::CDPlay(int track, int resume, int volume, int restart) {
    char buffer[CD_POSITION_BUFFER_SIZE];
    unsigned char notify;
    int cdTrack;
    HWND window;
    HWND newWindow;
    if (gbNoSound != 0 || gConfig.musicVolume == 0)
        return;
    if (track == -1) {
        CDStop();
        return;
    }
    if (m_currentTrack == track && CDPlaying != 0 && restart == 0)
        return;
    Process1WindowsMessage();
    ServiceSound();
    if (volume == -1) {
        if (gConfig.musicVolume != 0) {
            if (m_fadeSteps == 0)
                volume = gConfig.musicVolume;
            else
                volume = 1;
        } else {
            volume = 0;
        }
    }
    m_cdTrack = track;
    m_cdPlayFrame = volume;
    if ((track >= 0 && track < MUSIC_POSITION_TRACK_END) || track == MUSIC_POSITION_TRACK_1
        || track == MUSIC_POSITION_TRACK_3 || resume > 0)
        resume = 1;
    notify = 0;
    if (track < MUSIC_POSITION_TRACK_END || (track >= CD_NOTIFY_FIRST && track <= CD_NOTIFY_LAST)
        || (track >= CD_NOTIFY_EXTRA_FIRST && track <= CD_NOTIFY_EXTRA_LAST)
        || track == MUSIC_POSITION_TRACK_1 || track == MUSIC_POSITION_TRACK_2
        || track == MUSIC_POSITION_TRACK_3
        || (track >= CD_NOTIFY_SCENARIO_FIRST && track <= CD_NOTIFY_SCENARIO_LAST))
        notify = 1;
    cdTrack = CDTrackMap[track];
    wsprintfA(CommandString, "set CD time format tmsf");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
    wsprintfA(CommandString, "status CD mode");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
    if (strcmpi(lpszReturnString, "stopped") != 0) {
        wsprintfA(CommandString, "status CD position");
        nMCIError = mciSendStringA(CommandString, buffer, sizeof(buffer), NULL);
        if (nMCIError != 0)
            HandleMCIError(nMCIError, CommandString);
        strcpy(CDPreviousPosition[CDTrackMap[m_currentTrack]], buffer);
        ValidatePreviousPosition(CDTrackMap[m_currentTrack]);
    }
    Process1WindowsMessage();
    ServiceSound();
    if (restart == 0 && resume != 0 && CDPreviousPosition[cdTrack][0] != 0) {
        wsprintfA(
            CommandString,
            "play CD from %s to %d%s",
            CDPreviousPosition[cdTrack],
            cdTrack + 1,
            notify ? " notify" : ""
        );
        window = notify ? hwndApp : NULL;
        nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, window);
        if (nMCIError != 0)
            HandleMCIError(nMCIError, CommandString);
    } else {
        wsprintfA(
            CommandString,
            "play CD from %d to %d%s",
            cdTrack,
            cdTrack + 1,
            notify ? " notify" : ""
        );
        newWindow = notify ? hwndApp : NULL;
        nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, newWindow);
        if (nMCIError != 0)
            HandleMCIError(nMCIError, CommandString);
    }
    CDPlaying = 1;
    CDPlayOnce = 1 - notify;
    Process1WindowsMessage();
    ServiceSound();
    if (m_fadeSteps > 0) {
        m_fadeSteps = MUSIC_FADE_TOTAL_STEPS;
        gMusicFadeTimer = KBTickCount() + CD_FADE_DELAY_TICKS;
        CDSetVolume(10, 0);
    } else {
        CDSetVolume(volume, 0);
    }
    m_currentTrack = static_cast<char>(track);
}

// PoL SetReady2Poll correspondence; HoMM1 also requests a stream poll.
VA(0x004777b0, 0x42)
void SetReady2Poll(void) {
    if (gpSoundManager == NULL)
        return;
    if (gpSoundManager->m_musicStreamOpen != 0)
        gpSoundManager->m_pollRequested = 1;
    gpSoundManager->m_pollToggle ^= 1;
    if (gpSoundManager->m_pollToggle != 0)
        gpSoundManager->m_pollDue = 1;
}

VA(0x00477800, 0x63)
soundManager::soundManager(void) {
    m_active = 0;
    m_fadeSteps = 0;
    m_field_0x566 = 0;
    m_field_0x56e = 1;
    memset(gSampleVolumes, 0, SAMPLE_VOLUME_TABLE_BYTES);
    memset(&m_samplesReady, 0, SOUND_STATE_RESET_SPAN);
    m_musicReady = 0;
    m_digitalDriver = NULL;
    m_activeSample = NULL;
    m_cdTrack = 0;
    m_cdPlayFrame = 0;
}

VA(0x00477870, 0xf3)
struct _DIG_DRIVER* WAVE_init_driver(
    unsigned long sampleRate,
    unsigned short bitsPerSample,
    unsigned short channels,
    unsigned short showErrors
) {
    unsigned int numDevs;
    struct _DIG_DRIVER* drvr;
    WAVEOUTCAPSA caps;
    int rc;
    numDevs = waveOutGetNumDevs();
    if (numDevs == 0) {
        drvr = NULL;
        return NULL;
    }
    if (waveOutGetDevCapsA(0, &caps, sizeof(caps)) != 0) {
        MessageBoxA(
            hwndApp,
            "Sound initialization error!  No wave devices found.",
            "Startup Error",
            0
        );
        drvr = NULL;
        return NULL;
    }
    gWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
    gWaveFormat.wf.nChannels = channels;
    gWaveFormat.wf.nSamplesPerSec = sampleRate;
    gWaveFormat.wf.nAvgBytesPerSec =
        (bitsPerSample >> PCM_BITS_PER_BYTE_SHIFT) * channels * sampleRate;
    gWaveFormat.wf.nBlockAlign = (bitsPerSample >> PCM_BITS_PER_BYTE_SHIFT) * channels;
    gWaveFormat.wBitsPerSample = bitsPerSample;
    rc = AIL_waveOutOpen(&drvr, NULL, 0, &gWaveFormat.wf);
    if (rc != 0) {
        if (showErrors != 0)
            MessageBoxA(hwndApp, AIL_last_error(), "Sound initialization error!", 0);
        drvr = NULL;
        return NULL;
    }
    return drvr;
}

inline void soundManager::AllocateSampleHandles(void) {
    int sampleIndex;
    if (gbNoSound != 0)
        return;
    if (m_digitalDriver == NULL)
        return;
    for (sampleIndex = 0; sampleIndex < SOUND_SAMPLE_HANDLE_COUNT; sampleIndex++) {
        m_sampleHandles[sampleIndex] = AIL_allocate_sample_handle(m_digitalDriver);
        if (m_sampleHandles[sampleIndex] == NULL)
            break;
    }
    m_musicSample = NULL;
    m_numSampleHandles = sampleIndex;
}

VA(0x00477970, 0x1bc)
short soundManager::Open(short) {
    int keyState;
    keyState = GetAsyncKeyState(VK_F6);
    if (HIBYTE(keyState)) {
        gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
        WritePrefs();
    }
    keyState = GetAsyncKeyState(VK_F7);
    if (HIBYTE(keyState)) {
        gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
        WritePrefs();
    }
    m_currentTrack = -1;
    m_cdReady = gConfig.musicSource == SOUND_MUSIC_SOURCE_CD;
    if (gbNoSound == 0) {
        m_pollToggle = m_pollDue = m_pollRequested = 0;
        AIL_startup();
        CDStartup();
        m_musicReady = 1;
        m_musicBuffers[0] = malloc(MUSIC_STREAM_BUFFER_SIZE);
        m_musicBuffers[1] = malloc(MUSIC_STREAM_BUFFER_SIZE);
        if (m_digitalDriver == NULL)
            m_digitalDriver = WAVE_init_driver(
                MUSIC_STREAM_RATE,
                SOUND_DEFAULT_SAMPLE_BITS,
                SOUND_DEFAULT_SAMPLE_CHANNELS,
                0
            );
        if (m_digitalDriver == NULL) {
            gbNoSound = 1;
            if (m_musicBuffers[0] != NULL)
                free(m_musicBuffers[0]);
            m_musicBuffers[0] = NULL;
            if (m_musicBuffers[1] != NULL)
                free(m_musicBuffers[1]);
            m_musicBuffers[1] = NULL;
            m_musicReady = 0;
            AIL_shutdown();
        } else {
            AllocateSampleHandles();
            m_samplesReady = 1;
            m_musicStreamOpen = 0;
            m_musicSample = NULL;
            m_midiFile = NULL;
            memset(m_savedTrackPositions, 0, sizeof(m_savedTrackPositions));
            m_fading = 1;
        }
    }
    m_messageMask = BASE_MANAGER_ACCEPT_LEFT_BUTTON_UP;
    m_priority = SOUND_MANAGER_PRIORITY;
    m_active = 1;
    strcpy(m_name, "soundManager");
    return 0;
}

inline void soundManager::CDShutdown(void) {
    if (gbNoSound != 0)
        return;
    if (m_cdStarted == 0)
        return;
    wsprintfA(CommandString, "stop CD");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
    wsprintfA(CommandString, "close CD");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != 0)
        HandleMCIError(nMCIError, CommandString);
}

VA(0x00477b30, 0x179)
void soundManager::Close(void) {
    if (m_active != 1)
        return;
    if (gbNoSound == 0) {
        AIL_shutdown();
        CDShutdown();
    }
    m_active = 0;
    gbNoSound = 1;
    if (m_musicBuffers[0] != NULL)
        free(m_musicBuffers[0]);
    m_musicBuffers[0] = NULL;
    if (m_musicBuffers[1] != NULL)
        free(m_musicBuffers[1]);
    m_musicBuffers[1] = NULL;
}

VA(0x00477cb0, 0x6)
short soundManager::Main(tag_message&) {
    return 0;
}

inline int soundManager::ConvertVolume(int volume, int soundType) {
    int result = 0;
    if (soundType == SOUND_VOLUME_MUSIC) {
        if (gConfig.musicVolume >= SOUND_VOLUME_FIRST && gConfig.musicVolume <= SOUND_VOLUME_LAST) {
            result = ((SOUND_VOLUME_LAST + 1 - gConfig.musicVolume) * volume) / SOUND_VOLUME_LAST;
            if (result < 1)
                result = 1;
        }
    } else if (gConfig.soundVolume >= SOUND_VOLUME_FIRST
               && gConfig.soundVolume <= SOUND_VOLUME_LAST) {
        result = ((SOUND_VOLUME_LAST + 1 - gConfig.soundVolume) * volume) / SOUND_VOLUME_LAST;
        if (result < 1)
            result = 1;
    }
    if (result < 0)
        result = 0;
    if (result > MIDI_VOLUME_MAX)
        result = MIDI_VOLUME_MAX;
    return result;
}

VA(0x00477cc0, 0x331)
struct _SAMPLE* soundManager::StartSample(
    char* name,
    char**,
    short,
    short loop,
    int volume,
    int channelType,
    long resume
) {
    short channel;
    struct _SAMPLE* sample;
    int sampleType;
    int stereo;
    int sampleRate;
    short index;
    char* filename;
    char path[SAMPLE_PATH_CAPACITY];
    FILE* file;
    if (gbNoSound != 0)
        return NULL;
    if (m_musicReady == 0)
        return NULL;
    if (m_samplesReady == 0)
        return NULL;
    Process1WindowsMessage();
    if (channelType == 0) {
        channel = 0;
        if (m_musicStreamOpen != 0) {
            StopSample(m_musicSample);
            m_musicStreamOpen = 0;
            ProcessAssert(
                reinterpret_cast<int>(
                    m_midiFile
                ), // byte-evidenced: retail passes its FILE pointer to the integer assertion API.
                gStartSampleAssertFile,
                gStartSampleAssertLine + 37
            );
            fclose(m_midiFile);
            m_midiFile = NULL;
        }
    }
    Process1WindowsMessage();
    sample = m_sampleHandles[channel];
    gSampleVolumes[channel] = static_cast<short>(volume);
    m_channelVolumes[channel] = static_cast<char>(volume);
    sampleType = SAMPLE_FORMAT_16_BIT;
    sampleRate = SAMPLE_RATE_NORMAL;
    stereo = SAMPLE_FORMAT_STEREO;
    filename = _strrev(name);
    for (index = 0; index < SAMPLE_SUFFIX_COUNT; index++) {
        switch (filename[index]) {
            case '1':
                sampleRate = SAMPLE_RATE_LOW;
                break;
            case '2':
                sampleRate = SAMPLE_RATE_NORMAL;
                break;
            case '4':
                sampleRate = SAMPLE_RATE_HIGH;
                break;
            case '6':
                sampleType = SAMPLE_FORMAT_16_BIT;
                break;
            case '8':
                sampleType = 0;
                break;
            case 'M':
            case 'm':
                stereo = 0;
                break;
        }
    }
    sampleType += stereo;
    filename = _strrev(filename);
    AIL_init_sample(sample);
    AIL_set_sample_type(sample, sampleType, 1);
    AIL_set_sample_playback_rate(sample, sampleRate);
    Process1WindowsMessage();
    sprintf(path, "%s%s", gcSoundPath, filename);
    if (_access(path, 0) == -1) {
        if (_access(path, 0) == -1) {
            sprintf(path, "%s%s", gcDataPath, filename);
            if (_access(path, 0) == -1)
                return NULL;
        }
    }
    file = fopen(path, "rb");
    fseek(file, resume, SEEK_SET);
    if (file == NULL)
        return NULL;
    AIL_set_sample_volume(sample, ConvertVolume(volume, SOUND_VOLUME_MUSIC));
    m_midiFile = file;
    m_musicSample = sample;
    m_musicStreamOpen = 1;
    m_musicStreamRestart = static_cast<char>(loop);
    Process1WindowsMessage();
    return sample;
}

VA(0x00478000, 0xe1)
void soundManager::StopAllSamples(void) {
    short sampleIndex;
    int wait;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    for (sampleIndex = 0; sampleIndex < m_numSampleHandles; sampleIndex++) {
        if (AIL_sample_status(m_sampleHandles[sampleIndex]) == SAMPLE_STATUS_PLAYING)
            AIL_end_sample(m_sampleHandles[sampleIndex]);
    }
    m_fadeSteps = 0;
    if (m_cdReady != 0) {
        CDStop();
    } else if (m_musicStreamOpen != 0) {
        m_musicStreamOpen = 0;
        ProcessAssert(
            reinterpret_cast<int>(
                m_midiFile
            ), // byte-evidenced: retail passes the FILE pointer as its assertion condition.
            gStopAllSamplesAssertFile,
            gStopAllSamplesAssertLine + 27
        );
        fclose(m_midiFile);
        m_midiFile = NULL;
    }
    for (wait = 0; wait < SAMPLE_STOP_ALL_WAIT_COUNT; wait++) {
        ServiceSound();
        DelayMilli(1);
    }
}

// /Ob2 expands this ordinary routine in StartSample and PlayAmbientMusic.
VA(0x004780f0, 0x4f)
void soundManager::StopSample(struct _SAMPLE* sample) {
    int wait;
    unsigned char music;
    if (gbNoSound != 0)
        return;
    music = 0;
    if (m_sampleHandles[0] == sample)
        music = 1;
    AIL_end_sample(sample);
    if (music != 0) {
        for (wait = 0; wait < MUSIC_STOP_WAIT_COUNT; wait++) {
            ServiceSound();
            DelayMilli(1);
        }
    }
}

VA(0x00478140, 0x1f1)
void soundManager::ModifySample(struct _SAMPLE* sampleHandle, short operation, long value) {
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (m_samplesReady == 0)
        return;
    int foundChannel = -1;
    for (int sampleIndex = 0; sampleIndex < m_numSampleHandles; sampleIndex++) {
        if (m_sampleHandles[sampleIndex] == sampleHandle)
            foundChannel = sampleIndex;
    }
    switch (operation) {
        case SOUND_OPERATION_VOLUME:
        case SOUND_OPERATION_EFFECT_VOLUME:
            AIL_set_sample_volume(sampleHandle, ConvertVolume(value, SOUND_VOLUME_EFFECT));
            if (foundChannel >= 0)
                gSampleVolumes[foundChannel] = static_cast<short>(value);
            break;
        case SOUND_OPERATION_MUSIC_VOLUME:
            ProcessAssert(m_cdReady == 0, gModifySampleAssertFile, gModifySampleAssertLine + 27);
            AIL_set_sample_volume(sampleHandle, ConvertVolume(value, SOUND_VOLUME_MUSIC));
            if (foundChannel >= 0)
                gSampleVolumes[foundChannel] = static_cast<short>(value);
            break;
        case SOUND_OPERATION_START:
            AIL_start_sample(sampleHandle);
            break;
    }
    Process1WindowsMessage();
}

// The retail volume updater expands this same query helper.
VA(0x00478340, 0x4d)
long soundManager::DigitalReport(struct _SAMPLE* sample, short reportType) {
    int sampleStatus;
    if (gbNoSound != 0)
        return 0;
    switch (reportType) {
        case SAMPLE_REPORT_VOLUME:
            return AIL_sample_volume(sample);
        case SAMPLE_REPORT_PLAYING:
            sampleStatus = AIL_sample_status(sample);
            return sampleStatus == SAMPLE_STATUS_PLAYING;
    }
    return 0;
}

VA(0x00478390, 0xbd)
void soundManager::AdjustSoundVolumes(void) {
    int sampleIndex;
    struct _SAMPLE* sampleHandle;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    for (sampleIndex = 1; sampleIndex < m_numSampleHandles; sampleIndex++) {
        sampleHandle = m_sampleHandles[sampleIndex];
        if (gConfig.soundVolume != 0) {
            if (DigitalReport(sampleHandle, SAMPLE_REPORT_PLAYING) != 0)
                ModifySample(
                    sampleHandle,
                    SOUND_OPERATION_EFFECT_VOLUME,
                    gSampleVolumes[sampleIndex]
                );
        } else {
            ModifySample(sampleHandle, SOUND_OPERATION_VOLUME, 0);
        }
    }
}

VA(0x00478450, 0x16c)
void soundManager::AdjustMusicVolumes(void) {
    unsigned char savePosition;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (m_currentTrack < 0)
        return;
    savePosition = 0;
    if (m_currentTrack < MUSIC_POSITION_TRACK_END || m_currentTrack == MUSIC_POSITION_TRACK_1
        || m_currentTrack == MUSIC_POSITION_TRACK_3)
        savePosition = 1;
    if (gConfig.musicVolume != 0) {
        if (m_cdReady != 0)
            CDSetVolume(-1, 0);
        else
            ModifySample(m_sampleHandles[0], SOUND_OPERATION_MUSIC_VOLUME, SAMPLE_VOLUME_MAX);
        PlayAmbientMusic(
            m_currentTrack,
            savePosition ? m_savedTrackPositions[m_currentTrack] : 0,
            -1
        );
    } else {
        if (m_cdReady != 0) {
            CDSetVolume(-1, 0);
        } else {
            if (savePosition != 0) {
                ProcessAssert(
                    reinterpret_cast<int>(m_midiFile), // byte-evidenced
                    gAdjustMusicAssertFile,
                    gAdjustMusicAssertLine + 40
                );
                m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
            }
            ModifySample(m_sampleHandles[0], SOUND_OPERATION_VOLUME, 0);
        }
    }
}

VA(0x004785c0, 0x104)
void soundManager::SetMusicQuality(int musicSource) {
    int track;
    if (gbNoSound != 0)
        return;
    if (m_samplesReady == 0)
        return;
    if (gConfig.musicVolume == 0)
        return;
    if (m_cdStarted == 0)
        return;
    if (m_cdReady != 0) {
        track = m_currentTrack;
        CDStop();
    } else if (m_musicStreamOpen != 0) {
        track = m_currentTrack;
        StopSample(m_musicSample);
        m_musicStreamOpen = 0;
        if (m_midiFile != NULL)
            fclose(m_midiFile);
        m_midiFile = NULL;
    } else {
        track = -1;
    }
    memset(m_savedTrackPositions, 0, sizeof(m_savedTrackPositions));
    gConfig.musicSource = musicSource;
    m_cdReady = musicSource == SOUND_MUSIC_SOURCE_CD;
    if (track >= 0)
        PlayAmbientMusic(track, 0, -1);
}

VA(0x004786d0, 0x26e)
void soundManager::PlayAmbientMusic(int track, long resume, int volume) {
    char filename[MUSIC_FILENAME_CAPACITY];
    char* data;
    unsigned char loop;
    int wait;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (m_samplesReady == 0)
        return;
    if (m_cdReady != 0) {
        if (gConfig.musicVolume == 0) {
            m_currentTrack = static_cast<char>(track);
            return;
        }
        CDPlay(track, resume, volume, 0);
        m_currentTrack = static_cast<char>(track);
        return;
    }
    if (m_midiFile != NULL
        && ((m_currentTrack >= 0 && m_currentTrack < MUSIC_POSITION_TRACK_END)
            || m_currentTrack == MUSIC_POSITION_TRACK_1 || m_currentTrack == MUSIC_POSITION_TRACK_2
            || m_currentTrack == MUSIC_POSITION_TRACK_3)) {
        ProcessAssert(
            reinterpret_cast<int>(
                m_midiFile
            ), // byte-evidenced: retail passes the FILE pointer as its assertion condition.
            gAmbientMusicAssertFile,
            gAmbientMusicAssertLine + 37
        );
        m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
    }
    m_currentTrack = static_cast<char>(track);
    if (gConfig.musicVolume == 0)
        return;
    m_fadeSteps = 0;
    m_fadeTargetTrack = track;
    data = NULL;
    if (m_musicStreamOpen != 0) {
        StopSample(m_musicSample);
        for (wait = 0; wait < MUSIC_STOP_WAIT_COUNT; wait++) {
            ServiceSound();
            DelayMilli(MUSIC_STOP_WAIT_MILLISECONDS);
        }
        m_musicStreamOpen = 0;
        if (m_midiFile != NULL)
            fclose(m_midiFile);
        m_midiFile = NULL;
    }
    if (track >= 0) {
        loop = 0;
        if (track < MUSIC_POSITION_TRACK_END
            || (track >= CD_NOTIFY_FIRST && track <= CD_NOTIFY_LAST)
            || (track >= CD_NOTIFY_EXTRA_FIRST && track <= CD_NOTIFY_EXTRA_LAST)
            || track == MUSIC_POSITION_TRACK_1 || track == MUSIC_POSITION_TRACK_2
            || track == MUSIC_POSITION_TRACK_3
            || (track >= CD_NOTIFY_SCENARIO_FIRST && track <= CD_NOTIFY_SCENARIO_LAST))
            loop = 1;
        if (track == MUSIC_POSITION_TRACK_1 || gConfig.musicSource == 0)
            sprintf(filename, "heroes%02d.82m", track);
        else if (gConfig.musicSource == 1)
            sprintf(filename, "heroes%02d.82s", track);
        else
            sprintf(filename, "heroes%02d.62s", track);
        if (volume == -1) {
            if (gConfig.musicVolume != 0) {
                if (m_fadeSteps == 0)
                    volume = SAMPLE_VOLUME_MAX;
                else
                    volume = 1;
            } else {
                volume = 0;
            }
        }
        m_activeSample = StartSample(filename, &data, 1, loop, volume, 0, resume);
    }
    m_currentTrack = static_cast<char>(track);
}

// donor PoL RVA 0x000cd320; preferred Buka symbol ?PollSound@soundManager@@QAEXXZ
// donor Buka TU BASE/soundmgr; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void soundManager::PollSound(void)@0x000cd320
VA(0x00478940, 0x489)
void soundManager::PollSound(void) {
    int volume;
    int buffer;
    long delta;
    int musicFadeStep;

    if (gbNoSound != 0)
        return;
    if (m_pollRequested == 0 && m_fadeSteps == 0)
        return;
    if (gConfig.musicVolume == 0)
        return;

    if (m_fadeSteps > 0) {
        Process1WindowsMessage();
        if (m_currentTrack >= MUSIC_POSITION_TRACK_END || m_currentTrack < 0)
            gMusicFadeTimer = KBTickCount();
        delta = gMusicFadeTimer - KBTickCount();
        m_fadeSteps = delta / MUSIC_FADE_STEP_TICKS;
        if (m_fadeSteps < 1)
            m_fadeSteps = 0;

        if (m_fadeSteps <= MUSIC_FADE_HOLD_LAST && m_currentTrack != m_fadeTargetTrack) {
            if (m_midiFile != 0
                && ((m_currentTrack >= 0 && m_currentTrack < MUSIC_POSITION_TRACK_END)
                    || m_currentTrack == MUSIC_POSITION_TRACK_1
                    || m_currentTrack == MUSIC_POSITION_TRACK_2
                    || m_currentTrack == MUSIC_POSITION_TRACK_3)) {
                if (m_cdReady == 0) {
                    ProcessAssert(
                        reinterpret_cast<int>(m_midiFile), // byte-evidenced
                        gSoundManagerAssertFile1,
                        gSoundManagerAssertLine + 42
                    );
                    m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
                }
            } else {
                gMusicFadeTimer = KBTickCount();
            }
            m_fading = 1;
            if ((m_fadeTargetTrack >= 0 && m_fadeTargetTrack < MUSIC_POSITION_TRACK_END)
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_1
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_2
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_3)
                PlayAmbientMusic(m_fadeTargetTrack, m_savedTrackPositions[m_fadeTargetTrack], -1);
            else
                PlayAmbientMusic(m_fadeTargetTrack, 0, -1);
            delta = gMusicFadeTimer - KBTickCount();
            m_fadeSteps = delta / MUSIC_FADE_STEP_TICKS;
            if (m_fadeSteps < 1)
                m_fadeSteps = 0;
            m_currentTrack = static_cast<char>(m_fadeTargetTrack);
        }

        musicFadeStep = m_fadeSteps;
        if (m_fadeSteps <= MUSIC_FADE_HOLD_LAST)
            volume =
                (MUSIC_FADE_TOTAL_STEPS - m_fadeSteps) * SAMPLE_VOLUME_MAX / MUSIC_FADE_TOTAL_STEPS;
        else
            volume =
                (m_fadeSteps - MUSIC_FADE_HOLD_LAST) * SAMPLE_VOLUME_MAX / MUSIC_FADE_RISE_STEPS;
        if (volume > SAMPLE_VOLUME_MAX)
            volume = SAMPLE_VOLUME_MAX;
        if (volume < 0)
            volume = 0;

        struct _SAMPLE* sample = m_sampleHandles[0];
        if (m_cdReady != 0) {
            volume = (MUSIC_FADE_TOTAL_STEPS - gConfig.musicVolume) * volume * MIDI_VOLUME_MAX
                     / CD_VOLUME_SCALE_DIVISOR;
            if (volume > MIDI_VOLUME_MAX)
                volume = MIDI_VOLUME_MAX;
            if (volume < 0)
                volume = 0;

            if (gbNoSound == 0 && m_auxDevice != -1) {
                if (volume == -1)
                    volume = gConfig.musicVolume;
                unsigned long stereoVolume;
                if (volume != 0) {
                    volume = volume / 12 + 1;
                    volume <<= 12;
                    stereoVolume = volume << 16 | volume;
                } else
                    stereoVolume = 0;
                auxSetVolume(m_auxDevice, stereoVolume);
            }
        } else {
            ModifySample(sample, SOUND_OPERATION_MUSIC_VOLUME, volume);
        }
    }

    if (m_musicStreamOpen != 0 && m_cdReady == 0) {
        Process1WindowsMessage();
        buffer = AIL_sample_buffer_ready(m_musicSample);
        if (buffer != -1) {
            Process1WindowsMessage();
            ProcessAssert(
                reinterpret_cast<int>(m_midiFile), // byte-evidenced
                gSoundManagerAssertFile2,
                gSoundManagerAssertLine + 103
            );
            unsigned long bytesRead =
                fread(m_musicBuffers[buffer], 1, MUSIC_STREAM_BUFFER_SIZE, m_midiFile);
            AIL_load_sample_buffer(m_musicSample, buffer, m_musicBuffers[buffer], bytesRead);
        }

        Process1WindowsMessage();
        if (AIL_sample_status(m_musicSample) != SAMPLE_STATUS_PLAYING) {
            if (m_musicStreamRestart != 0) {
                if (m_fading == 0) {
                    ProcessAssert(
                        reinterpret_cast<int>(m_midiFile), // byte-evidenced
                        gSoundManagerAssertFile3,
                        gSoundManagerAssertLine + 116
                    );
                    rewind(m_midiFile);
                } else {
                    m_fading = 0;
                }

                Process1WindowsMessage();
                AIL_init_sample(m_musicSample);
                int format;
                if (m_currentTrack == MUSIC_POSITION_TRACK_1 || gConfig.musicSource == 0)
                    format = 0;
                else if (gConfig.musicSource == 1)
                    format = 2;
                else
                    format = 3;
                AIL_set_sample_type(m_musicSample, format, 1);
                AIL_set_sample_playback_rate(m_musicSample, MUSIC_STREAM_RATE);

                volume = 0;
                if (gConfig.musicVolume >= 1 && gConfig.musicVolume <= 10) {
                    volume =
                        (MUSIC_FADE_TOTAL_STEPS - gConfig.musicVolume) * SAMPLE_VOLUME_MAX / 10;
                    if (volume < 1)
                        volume = 1;
                }
                if (volume < 0)
                    volume = 0;
                if (volume > MIDI_VOLUME_MAX)
                    volume = MIDI_VOLUME_MAX;
                AIL_set_sample_volume(m_musicSample, volume);

                Process1WindowsMessage();
                buffer = AIL_sample_buffer_ready(m_musicSample);
                if (buffer != -1) {
                    Process1WindowsMessage();
                    ProcessAssert(
                        reinterpret_cast<int>(m_midiFile), // byte-evidenced
                        gSoundManagerAssertFile4,
                        gSoundManagerAssertLine + 145
                    );
                    unsigned long bytesRead =
                        fread(m_musicBuffers[buffer], 1, MUSIC_STREAM_BUFFER_SIZE, m_midiFile);
                    AIL_load_sample_buffer(
                        m_musicSample,
                        buffer,
                        m_musicBuffers[buffer],
                        bytesRead
                    );
                }
            } else {
                m_musicStreamOpen = 0;
                ProcessAssert(
                    reinterpret_cast<int>(m_midiFile), // byte-evidenced
                    gSoundManagerAssertFile5,
                    gSoundManagerAssertLine + 153
                );
                fclose(m_midiFile);
                m_midiFile = 0;
            }
        }
    }
    m_pollRequested = 0;
}

VA(0x00478dd0, 0x1ad)
void soundManager::SwitchAmbientMusic(int track) {
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (gConfig.musicVolume == 0) {
        m_currentTrack = static_cast<char>(track);
        return;
    }
    if (MusicPlaying() == 0) {
        PlayAmbientMusic(track, 0, -1);
        return;
    }
    Process1WindowsMessage();
    if ((m_fadeSteps != 0 && m_fadeTargetTrack != track)
        || (m_fadeSteps == 0 && m_currentTrack != track)) {
        if (m_fadeSteps <= MUSIC_FADE_HOLD_LAST) {
            m_fadeSteps = MUSIC_FADE_TOTAL_STEPS;
            gMusicFadeTimer = KBTickCount() + AMBIENT_FADE_DELAY_TICKS;
        }
        m_fadeTargetTrack = track;
        PollSound();
    }
}

VA(0x00478f80, 0x1ea)
struct _SAMPLE* soundManager::MemorySample(sample* sampleResource) {
    struct _SAMPLE* handle;
    short channel;
    SampleChannelStruct* channels;
    SamplePlaybackData* playback;
    if (gbNoSound != 0)
        return NULL;
    if (m_musicReady == 0)
        return NULL;
    if (gConfig.soundVolume == 0)
        return NULL;
    playback = &sampleResource->m_playbackData;
    if (m_samplesReady == 0 || playback->volume == 0)
        return NULL;
    channels = &SCS[playback->channelType];
    for (channel = static_cast<short>(channels->startChannel); channel < channels->endChannel;
         channel++) {
        if (AIL_sample_status(m_sampleHandles[channel]) == SAMPLE_STATUS_DONE)
            break;
    }
    if (channel == channels->endChannel) {
        if (playback->channelType == SAMPLE_PLAYBACK_CHANNEL_NONE)
            return NULL;
        channel = static_cast<short>(channels->currentChannel);
        channels->currentChannel++;
        if (channels->endChannel <= channels->currentChannel) {
            channels->currentChannel = channels->startChannel;
            channel = static_cast<short>(channels->currentChannel);
        }
        StopSample(m_sampleHandles[channel]);
    }
    handle = m_sampleHandles[channel];
    m_channelVolumes[channel] = static_cast<char>(playback->volume);
    gSampleVolumes[channel] = static_cast<short>(playback->volume);
    AIL_init_sample(handle);
    AIL_set_sample_type(handle, playback->format, 1);
    AIL_set_sample_playback_rate(handle, playback->sampleRate);
    AIL_set_sample_loop_count(handle, playback->loopCount);
    AIL_set_sample_address(handle, playback->data, playback->size);
    if (gConfig.soundVolume != 0)
        AIL_set_sample_volume(handle, ConvertVolume(playback->volume, SOUND_VOLUME_EFFECT));
    else
        AIL_set_sample_volume(handle, 0);
    AIL_start_sample(handle);
    playback->activeSample = handle;
    m_channelSamples[channel] = handle;
    m_channelSampleData[channel] = playback->data;
    m_channelSampleSizes[channel] = playback->size;
    return handle;
}

VA(0x00479170, 0x10)
void soundManager::ServiceSound(void) {
    if (gbNoSound == 0)
        AIL_serve();
}

VA(0x00479180, 0xfe)
int soundManager::MusicPlaying(void) {
    if (gbNoSound != 0)
        return 0;
    if (m_cdReady != 0)
        return CDIsPlaying();
    return DigitalReport(m_musicSample, SAMPLE_REPORT_PLAYING);
}
