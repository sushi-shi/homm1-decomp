// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <BASE/soundmgr.h>

// MSS precedes windows.h and Misc.h as in Buka's soundmgr include list. Its
// C1 handles set the /O2 range ids (C1 handle & 31) of the AIL import pointers
// against the C2 loop counters: SetMusicQuality's inlined StopSample wait loop
// needs AIL_serve's bucket below the counter's, and Open's inlined
// AllocateSampleHandles needs AIL_allocate_sample_handle ahead of its nodes
// (docs/patterns/vc4-register-tie-order-is-the-range-id.md).
#include <mss.h>
#include <windows.h>

#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/sample.h>
#include <BASE/soundManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Retail compiled Soundmgr.cpp incrementally (/Gi): every assertion's __LINE__ is
// read from a per-function static (?__LINE__Var@...) holding the function's
// original line, plus the assertion's offset (`movsx reg, word [var]; add reg, n`).
// The #line directives restore the original file name and the retail line values
// (52, 605, 740, 808, 900, 1008, 1118) so VC4 emits the same statics; see
// docs/patterns/vc4-gi-line-var.md. PoL's CD/sample helpers are ordinary member
// functions: /Ob2 expands them and the linker drops the unreferenced copies, and
// ValidatePreviousPosition's single line static (52) is shared by CDStop and CDPlay.

// PoL preserves this helper family; retail expands these helpers in CDPlay.
void HandleMCIError(i32 errorCode, char* command) {
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
    gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
    WritePrefs();
    ShutDown(gText);
}

#line 52 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::ValidatePreviousPosition(i32 track) {
    char buffer[CD_POSITION_BUFFER_SIZE];
    char* separator;
#line 56
    ProcessAssert(track >= 0 && track < MUSIC_TRACK_COUNT, __FILE__, __LINE__);
    if (CDPreviousPosition[track][0] == 0)
        return;
    strcpy(buffer, CDPreviousPosition[track]);
    separator = FindToken(buffer, ':');
    if (separator != NULL)
        *separator = 0;
    if (atoi(buffer) != track)
        CDPreviousPosition[track][0] = 0;
}

void soundManager::CDSetVolume(i32 volume, i32 fadeScale) {
    i32 level;
    u32 stereoVolume;
    if (gbNoSound != 0 || m_auxDevice == CD_AUX_DEVICE_NONE)
        return;
    if (volume == SOUND_VOLUME_FROM_CONFIG)
        level = gConfig.musicVolume;
    else
        level = volume;
    if (level != 0) {
        i32 channel;
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
    if (nMCIError != MMSYSERR_NOERROR)
        HandleMCIError(nMCIError, CommandString);
    if (_stricmp(lpszReturnString, "stopped") != 0) {
        wsprintfA(CommandString, "status CD position");
        nMCIError = mciSendStringA(CommandString, position, sizeof(position), NULL);
        if (nMCIError != MMSYSERR_NOERROR)
            HandleMCIError(nMCIError, CommandString);
        strcpy(CDPreviousPosition[CDTrackMap[m_currentTrack]], position);
        ValidatePreviousPosition(CDTrackMap[m_currentTrack]);
    }
    CDPlaying = 0;
}

// PoL keeps this out of line; HoMM1 retains only its expansions.
i32 soundManager::CDIsPlaying(void) {
    if (gbNoSound != 0)
        return 0;
    wsprintfA(CommandString, "status CD mode");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != MMSYSERR_NOERROR)
        HandleMCIError(nMCIError, CommandString);
    return _stricmp(lpszReturnString, "playing") == 0;
}

VA(0x00477110, 0xd7)
u32 soundManager::CDStartup(void) {
    i32 device;
    i32 numDevices;
    if (gbNoSound != 0)
        return 0;
    wsprintfA(CommandString, "open %c: type cdaudio alias CD shareable", gSoundPath[0]);
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != MMSYSERR_NOERROR) {
        m_cdStarted = 0;
        gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
        m_cdReady = 0;
        WritePrefs();
        return 0;
    }
    m_cdStarted = 1;
    numDevices = auxGetNumDevs();
    m_auxDevice = CD_AUX_DEVICE_NONE;
    for (device = 0; device < numDevices; device++) {
        memset(&gAuxCaps, 0, sizeof(gAuxCaps));
        auxGetDevCapsA(device, &gAuxCaps, sizeof(gAuxCaps));
        if (gAuxCaps.wTechnology == AUXCAPS_CDAUDIO) {
            m_auxDevice = static_cast<i16>(device);
            break;
        }
    }
    return nMCIError;
}

// PoL declares t1..t3 for KBTickCount timings. HoMM1 emits no timing calls, but the three
// C1 handles are needed by later functions' register ties (WAVE_init_driver).
VA(0x004771f0, 0x5b7)
void soundManager::CDPlay(i32 track, i32 resume, i32 volume, i32 restart) {
    i32 t1;
    i32 t2;
    i32 t3;
    char buffer[CD_POSITION_BUFFER_SIZE];
    u8 notify;
    i32 cdTrack;
    HWND window;
    HWND newWindow;
    if (gbNoSound != 0 || gConfig.musicVolume == SOUND_VOLUME_OFF)
        return;
    if (track == MUSIC_TRACK_NONE) {
        CDStop();
        return;
    }
    if (m_currentTrack == track && CDPlaying != 0 && restart == 0)
        return;
    Process1WindowsMessage();
    ServiceSound();
    if (volume == SOUND_VOLUME_FROM_CONFIG) {
        if (gConfig.musicVolume != SOUND_VOLUME_OFF) {
            if (m_fadeSteps == 0)
                volume = gConfig.musicVolume;
            else
                volume = SOUND_VOLUME_FIRST;
        } else {
            volume = SOUND_VOLUME_OFF;
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
    if (nMCIError != MMSYSERR_NOERROR)
        HandleMCIError(nMCIError, CommandString);
    wsprintfA(CommandString, "status CD mode");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != MMSYSERR_NOERROR)
        HandleMCIError(nMCIError, CommandString);
    if (_stricmp(lpszReturnString, "stopped") != 0) {
        wsprintfA(CommandString, "status CD position");
        nMCIError = mciSendStringA(CommandString, buffer, sizeof(buffer), NULL);
        if (nMCIError != MMSYSERR_NOERROR)
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
        if (nMCIError != MMSYSERR_NOERROR)
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
        if (nMCIError != MMSYSERR_NOERROR)
            HandleMCIError(nMCIError, CommandString);
    }
    CDPlaying = 1;
    CDPlayOnce = 1 - notify;
    Process1WindowsMessage();
    ServiceSound();
    if (m_fadeSteps > 0) {
        m_fadeSteps = MUSIC_FADE_TOTAL_STEPS;
        glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] = KBTickCount() + CD_FADE_DELAY_TICKS;
        CDSetVolume(SOUND_VOLUME_LAST, 0);
    } else {
        CDSetVolume(volume, 0);
    }
    m_currentTrack = track;
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
struct _DIG_DRIVER*
WAVE_init_driver(u32 sampleRate, u16 bitsPerSample, u16 channels, u16 showErrors) {
    u32 numDevs;
    struct _DIG_DRIVER* drvr;
    WAVEOUTCAPSA caps;
    i32 rc;
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
            MB_OK
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
            MessageBoxA(hwndApp, AIL_last_error(), "Sound initialization error!", MB_OK);
        drvr = NULL;
        return NULL;
    }
    return drvr;
}

void soundManager::AllocateSampleHandles(void) {
    i32 sampleIndex;
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

// PoL keeps the gbNoSound bypass as `goto managerReady`; the label also places the
// inlined AllocateSampleHandles handles where retail colours them.
VA(0x00477970, 0x1bc)
i16 soundManager::Open(i16) {
    i32 keyState;
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
    m_cdReady = gConfig.musicSource == SOUND_MUSIC_SOURCE_CD;
    m_currentTrack = MUSIC_TRACK_NONE;
    if (gbNoSound != 0)
        goto managerReady;
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
managerReady:
    m_messageMask = BASE_MANAGER_ACCEPT_LEFT_BUTTON_UP;
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_active = 1;
    strcpy(m_name, "soundManager");
    return 0;
}

void soundManager::CDShutdown(void) {
    if (gbNoSound != 0)
        return;
    if (m_cdStarted == 0)
        return;
    wsprintfA(CommandString, "stop CD");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != MMSYSERR_NOERROR)
        HandleMCIError(nMCIError, CommandString);
    wsprintfA(CommandString, "close CD");
    nMCIError = mciSendStringA(CommandString, lpszReturnString, CD_MCI_RESULT_LAST, NULL);
    if (nMCIError != MMSYSERR_NOERROR)
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
i16 soundManager::Main(tag_message&) {
    return 0;
}

i32 soundManager::ConvertVolume(i32 volume, i32 soundType) {
    i32 result = 0;
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

// Retail shares one `return NULL` tail between the missing-file and fopen-failure exits.
VA(0x00477cc0, 0x331)
struct _SAMPLE* soundManager::StartSample(
    char* name,
    char**,
    i16,
    i16 loop,
    i32 volume,
    i32 channelType,
    i32 resume
#line 605 "D:\\Heroes\\Base\\Soundmgr.cpp"
) {
    i16 channel;
    struct _SAMPLE* sample;
    i32 sampleRate;
    i32 stereo;
    i32 sampleType;
    i16 index;
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
    if (channelType == SAMPLE_PLAYBACK_CHANNEL_MUSIC) {
        channel = 0;
        if (m_musicStreamOpen != 0) {
            StopSample(m_musicSample);
            m_musicStreamOpen = 0;
            // byte-evidenced: retail passes its FILE pointer to the integer assertion API.
#line 642
            ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
            fclose(m_midiFile);
            m_midiFile = NULL;
        }
    }
    Process1WindowsMessage();
    sample = m_sampleHandles[channel];
    gSampleVolumes[channel] = static_cast<i16>(volume);
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
    AIL_set_sample_type(sample, sampleType, DIG_PCM_SIGN);
    AIL_set_sample_playback_rate(sample, sampleRate);
    Process1WindowsMessage();
    sprintf(path, "%s%s", gSoundPath, filename);
    if (_access(path, 0) == -1) {
        if (_access(path, 0) == -1) {
            sprintf(path, "%s%s", gDataPath, filename);
            if (_access(path, 0) == -1)
                goto notFound;
        }
    }
    file = fopen(path, "rb");
    fseek(file, resume, SEEK_SET);
    if (file == NULL)
        goto notFound;
    AIL_set_sample_volume(sample, ConvertVolume(volume, SOUND_VOLUME_MUSIC));
    m_midiFile = file;
    m_musicSample = sample;
    m_musicStreamOpen = 1;
    m_musicStreamRestart = static_cast<char>(loop);
    Process1WindowsMessage();
    return sample;
notFound:
    return NULL;
}

VA(0x00478000, 0xe1)
#line 740 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::StopAllSamples(void) {
    i16 sampleIndex;
    i32 wait;
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
        // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 767
        ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
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
    i32 wait;
    u8 music;
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
#line 808 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::ModifySample(struct _SAMPLE* sampleHandle, i16 operation, i32 value) {
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (m_samplesReady == 0)
        return;
    i32 foundChannel = -1;
    for (i32 sampleIndex = 0; sampleIndex < m_numSampleHandles; sampleIndex++) {
        if (m_sampleHandles[sampleIndex] == sampleHandle)
            foundChannel = sampleIndex;
    }
    switch (operation) {
        case SOUND_OPERATION_VOLUME:
        case SOUND_OPERATION_EFFECT_VOLUME:
            AIL_set_sample_volume(sampleHandle, ConvertVolume(value, SOUND_VOLUME_EFFECT));
            if (foundChannel >= 0)
                gSampleVolumes[foundChannel] = static_cast<i16>(value);
            break;
        case SOUND_OPERATION_MUSIC_VOLUME:
#line 835
            ProcessAssert(m_cdReady == 0, __FILE__, __LINE__);
            AIL_set_sample_volume(sampleHandle, ConvertVolume(value, SOUND_VOLUME_MUSIC));
            if (foundChannel >= 0)
                gSampleVolumes[foundChannel] = static_cast<i16>(value);
            break;
        case SOUND_OPERATION_START:
            AIL_start_sample(sampleHandle);
            break;
    }
    Process1WindowsMessage();
}

// The retail volume updater expands this same query helper.
VA(0x00478340, 0x4d)
i32 soundManager::DigitalReport(struct _SAMPLE* sample, i16 reportType) {
    i32 sampleStatus;
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
    i32 sampleIndex;
    struct _SAMPLE* sampleHandle;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    for (sampleIndex = 1; sampleIndex < m_numSampleHandles; sampleIndex++) {
        sampleHandle = m_sampleHandles[sampleIndex];
        if (gConfig.soundVolume != SOUND_VOLUME_OFF) {
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
#line 900 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::AdjustMusicVolumes(void) {
    u8 savePosition;
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
    if (gConfig.musicVolume != SOUND_VOLUME_OFF) {
        if (m_cdReady != 0)
            CDSetVolume(SOUND_VOLUME_FROM_CONFIG, 0);
        else
            ModifySample(m_sampleHandles[0], SOUND_OPERATION_MUSIC_VOLUME, SAMPLE_VOLUME_MAX);
        PlayAmbientMusic(
            m_currentTrack,
            savePosition ? m_savedTrackPositions[m_currentTrack] : 0,
            SOUND_VOLUME_FROM_CONFIG
        );
    } else {
        if (m_cdReady != 0) {
            CDSetVolume(SOUND_VOLUME_FROM_CONFIG, 0);
        } else {
            if (savePosition != 0) {
                // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 940
                ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
                m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
            }
            ModifySample(m_sampleHandles[0], SOUND_OPERATION_VOLUME, 0);
        }
    }
}

VA(0x004785c0, 0x104)
void soundManager::SetMusicQuality(i32 musicSource) {
    i32 track;
    if (gbNoSound != 0)
        return;
    if (m_samplesReady == 0)
        return;
    if (gConfig.musicVolume == SOUND_VOLUME_OFF)
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
        track = MUSIC_TRACK_NONE;
    }
    memset(m_savedTrackPositions, 0, sizeof(m_savedTrackPositions));
    gConfig.musicSource = musicSource;
    m_cdReady = musicSource == SOUND_MUSIC_SOURCE_CD;
    if (track >= 0)
        PlayAmbientMusic(track, 0, SOUND_VOLUME_FROM_CONFIG);
}

VA(0x004786d0, 0x26e)
#line 1008 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::PlayAmbientMusic(i32 track, i32 resume, i32 volume) {
    char filename[MUSIC_FILENAME_CAPACITY];
    char* data;
    u8 loop;
    i32 wait;
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (m_samplesReady == 0)
        return;
    if (m_cdReady != 0) {
        if (gConfig.musicVolume == SOUND_VOLUME_OFF) {
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
        // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1045
        ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
        m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
    }
    m_currentTrack = static_cast<char>(track);
    if (gConfig.musicVolume == SOUND_VOLUME_OFF)
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
        if (track == MUSIC_POSITION_TRACK_1 || gConfig.musicSource == SOUND_MUSIC_SOURCE_DIGITAL)
            sprintf(filename, "heroes%02d.82m", track);
        else if (gConfig.musicSource == SOUND_MUSIC_SOURCE_DIGITAL_STEREO)
            sprintf(filename, "heroes%02d.82s", track);
        else
            sprintf(filename, "heroes%02d.62s", track);
        if (volume == SOUND_VOLUME_FROM_CONFIG) {
            if (gConfig.musicVolume != SOUND_VOLUME_OFF) {
                if (m_fadeSteps == 0)
                    volume = SAMPLE_VOLUME_MAX;
                else
                    volume = 1;
            } else {
                volume = 0;
            }
        }
        m_activeSample =
            StartSample(filename, &data, 1, loop, volume, SAMPLE_PLAYBACK_CHANNEL_MUSIC, resume);
    }
    m_currentTrack = static_cast<char>(track);
}

// donor PoL RVA 0x000cd320; preferred Buka symbol ?PollSound@soundManager@@QAEXXZ
// donor Buka TU BASE/soundmgr; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:void soundManager::PollSound(void)@0x000cd320
VA(0x00478940, 0x489)
#line 1118 "D:\\Heroes\\Base\\Soundmgr.cpp"
void soundManager::PollSound(void) {
    i32 volume;
    i32 buffer;
    i32 delta;
    i32 musicFadeStep;

    if (gbNoSound != 0)
        return;
    if (m_pollRequested == 0 && m_fadeSteps == 0)
        return;
    if (gConfig.musicVolume == SOUND_VOLUME_OFF)
        return;

    if (m_fadeSteps > 0) {
        Process1WindowsMessage();
        if (m_currentTrack >= MUSIC_POSITION_TRACK_END || m_currentTrack < 0)
            glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] = KBTickCount();
        delta = glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] - KBTickCount();
        m_fadeSteps = delta / MUSIC_FADE_STEP_TICKS;
        if (m_fadeSteps < 1)
            m_fadeSteps = 0;

        if (m_fadeSteps <= MUSIC_FADE_HOLD_LAST && m_currentTrack != m_fadeTargetTrack) {
            if (m_midiFile != NULL
                && ((m_currentTrack >= 0 && m_currentTrack < MUSIC_POSITION_TRACK_END)
                    || m_currentTrack == MUSIC_POSITION_TRACK_1
                    || m_currentTrack == MUSIC_POSITION_TRACK_2
                    || m_currentTrack == MUSIC_POSITION_TRACK_3)) {
                if (m_cdReady == 0) {
                    // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1160
                    ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
                    m_savedTrackPositions[m_currentTrack] = ftell(m_midiFile);
                }
            } else {
                glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] = KBTickCount();
            }
            m_fading = 1;
            if ((m_fadeTargetTrack >= 0 && m_fadeTargetTrack < MUSIC_POSITION_TRACK_END)
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_1
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_2
                || m_fadeTargetTrack == MUSIC_POSITION_TRACK_3)
                PlayAmbientMusic(
                    m_fadeTargetTrack,
                    m_savedTrackPositions[m_fadeTargetTrack],
                    SOUND_VOLUME_FROM_CONFIG
                );
            else
                PlayAmbientMusic(m_fadeTargetTrack, 0, SOUND_VOLUME_FROM_CONFIG);
            delta = glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] - KBTickCount();
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

            if (gbNoSound == 0 && m_auxDevice != CD_AUX_DEVICE_NONE) {
                if (volume == SOUND_VOLUME_FROM_CONFIG)
                    volume = gConfig.musicVolume;
                u32 stereoVolume;
                if (volume != 0) {
                    volume = volume / CD_VOLUME_LEVEL_COUNT + 1;
                    volume <<= CD_VOLUME_LEVEL_SHIFT;
                    stereoVolume = volume << CD_STEREO_CHANNEL_SHIFT | volume;
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
            // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1221
            ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
            u32 bytesRead = fread(m_musicBuffers[buffer], 1, MUSIC_STREAM_BUFFER_SIZE, m_midiFile);
            AIL_load_sample_buffer(m_musicSample, buffer, m_musicBuffers[buffer], bytesRead);
        }

        Process1WindowsMessage();
        if (AIL_sample_status(m_musicSample) != SAMPLE_STATUS_PLAYING) {
            if (m_musicStreamRestart != 0) {
                if (m_fading == 0) {
                    // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1234
                    ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
                    rewind(m_midiFile);
                } else {
                    m_fading = 0;
                }

                Process1WindowsMessage();
                AIL_init_sample(m_musicSample);
                i32 format;
                if (m_currentTrack == MUSIC_POSITION_TRACK_1
                    || gConfig.musicSource == SOUND_MUSIC_SOURCE_DIGITAL)
                    format = DIG_F_MONO_8;
                else if (gConfig.musicSource == SOUND_MUSIC_SOURCE_DIGITAL_STEREO)
                    format = DIG_F_STEREO_8;
                else
                    format = DIG_F_STEREO_16;
                AIL_set_sample_type(m_musicSample, format, DIG_PCM_SIGN);
                AIL_set_sample_playback_rate(m_musicSample, MUSIC_STREAM_RATE);

                volume = 0;
                if (gConfig.musicVolume >= SOUND_VOLUME_FIRST
                    && gConfig.musicVolume <= SOUND_VOLUME_LAST) {
                    volume = (MUSIC_FADE_TOTAL_STEPS - gConfig.musicVolume) * SAMPLE_VOLUME_MAX
                             / SOUND_VOLUME_LAST;
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
                    // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1263
                    ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
                    u32 bytesRead =
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
                // byte-evidenced: retail passes the FILE pointer as its assertion condition.
#line 1271
                ProcessAssert(reinterpret_cast<i32>(m_midiFile), __FILE__, __LINE__);
                fclose(m_midiFile);
                m_midiFile = NULL;
            }
        }
    }
    m_pollRequested = 0;
}

VA(0x00478dd0, 0x1ad)
void soundManager::SwitchAmbientMusic(i32 track) {
    if (gbNoSound != 0)
        return;
    if (m_musicReady == 0)
        return;
    if (gConfig.musicVolume == SOUND_VOLUME_OFF) {
        m_currentTrack = static_cast<char>(track);
        return;
    }
    if (MusicPlaying() == 0) {
        PlayAmbientMusic(track, 0, SOUND_VOLUME_FROM_CONFIG);
        return;
    }
    Process1WindowsMessage();
    if ((m_fadeSteps != 0 && m_fadeTargetTrack != track)
        || (m_fadeSteps == 0 && m_currentTrack != track)) {
        if (m_fadeSteps <= MUSIC_FADE_HOLD_LAST) {
            m_fadeSteps = MUSIC_FADE_TOTAL_STEPS;
            glTimers[GLOBAL_MUSIC_FADE_TIMER_SLOT] = KBTickCount() + AMBIENT_FADE_DELAY_TICKS;
        }
        m_fadeTargetTrack = track;
        PollSound();
    }
}

VA(0x00478f80, 0x1ea)
struct _SAMPLE* soundManager::MemorySample(sample* sampleResource) {
    struct _SAMPLE* handle;
    i16 channel;
    SampleChannelStruct* channels;
    SamplePlaybackData* playback;
    if (gbNoSound != 0)
        return NULL;
    if (m_musicReady == 0)
        return NULL;
    if (gConfig.soundVolume == SOUND_VOLUME_OFF)
        return NULL;
    playback = &sampleResource->m_playbackData;
    if (m_samplesReady == 0 || playback->volume == 0)
        return NULL;
    channels = &SCS[playback->channelType];
    for (channel = static_cast<i16>(channels->startChannel); channel < channels->endChannel;
         channel++) {
        if (AIL_sample_status(m_sampleHandles[channel]) == SAMPLE_STATUS_DONE)
            break;
    }
    if (channel == channels->endChannel) {
        if (playback->channelType == SAMPLE_PLAYBACK_CHANNEL_NONE)
            return NULL;
        channel = static_cast<i16>(channels->currentChannel);
        channels->currentChannel++;
        if (channels->endChannel <= channels->currentChannel) {
            channels->currentChannel = channels->startChannel;
            channel = static_cast<i16>(channels->currentChannel);
        }
        StopSample(m_sampleHandles[channel]);
    }
    handle = m_sampleHandles[channel];
    m_channelVolumes[channel] = static_cast<char>(playback->volume);
    gSampleVolumes[channel] = static_cast<i16>(playback->volume);
    AIL_init_sample(handle);
    AIL_set_sample_type(handle, playback->format, DIG_PCM_SIGN);
    AIL_set_sample_playback_rate(handle, playback->sampleRate);
    AIL_set_sample_loop_count(handle, playback->loopCount);
    AIL_set_sample_address(handle, playback->data, playback->size);
    if (gConfig.soundVolume != SOUND_VOLUME_OFF)
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
i32 soundManager::MusicPlaying(void) {
    if (gbNoSound != 0)
        return 0;
    if (m_cdReady != 0)
        return CDIsPlaying();
    return DigitalReport(m_musicSample, SAMPLE_REPORT_PLAYING);
}

// Sound-manager data, initialized from retail .data (0x004a0fd0..) and
// zero-filled MCI/AIL work storage (0x004cc668..).
DATA(0x004a0fd0)
SampleChannelStruct SCS[4] = {{0, 1, 0}, {1, 2, 1}, {2, 6, 2}, {6, 16, 6}};
DATA(0x004a1000)
char CDPreviousPosition[60][CD_POSITION_CAPACITY] = {0};
DATA(0x004a1384)
i32 CDPlayOnce = 0;
DATA(0x004a138c)
i32 CDPlaying = 0;
DATA(0x004a1390)
i8 CDTrackMap[100] = {2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18,
                      19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 99,
                      99, 99, 99, 99, 99, 99, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,
                      46, 47, 48, 49, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                      99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
                      99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 50};
DATA(0x004a1620)
i32 gCDDrive = 0;
DATA(0x004cc668)
char lpszReturnString[CD_MCI_RESULT_LAST + 1];
DATA(0x004cc768)
u32 nMCIError;
DATA(0x004cc770)
i16 gSampleVolumes[SAMPLE_VOLUME_TABLE_BYTES / sizeof(i16)];
DATA(0x004cc7b0)
char CommandString[256];
DATA(0x004cc8b0)
AUXCAPSA gAuxCaps;
DATA(0x004cc8e0)
PCMWAVEFORMAT gWaveFormat;
