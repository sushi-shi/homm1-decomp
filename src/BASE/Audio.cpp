// Buka 2003 audio device and volume controls.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/audiereBackend.h>

DATA(0x004cdf50)
audiere::AudioDevicePtr AudiereDevice::device;
DATA(0x004a0f04)
static float gEffectsVolume = 1.0f;
DATA(0x004a0f08)
static float gMusicVolume = 1.0f;
DATA(0x004a0f0c)
static float gVolumeLevels[11] =
    {0.0f, 1.0f, 0.8f, 0.65f, 0.5f, 0.4f, 0.3f, 0.2f, 0.15f, 0.1f, 0.05f};

VA(0x00469b56, 0xf)
float VolumeLevel(int level) {
    return gVolumeLevels[level];
}

VA(0x00469b65, 0x70)
audiere::AudioDevicePtr GetAudioDevice() {
    return AudiereDevice::device;
}

VA(0x00469bd5, 0x132)
bool InitAudio() {
    audiere::AudioDevice* device = audiere::OpenDevice("winmm", NULL);
    if (device) {
        AudiereDevice::device = device;
    } else {
        AudiereDevice::device = audiere::OpenDevice("null", NULL);
        if (!AudiereDevice::device)
            return false;
        // Retail releases even a successful fallback device (RVA 0x69caf).
        AudiereDevice::device = NULL;
        return true;
    }
    return true;
}

VA(0x00469d07, 0x5c)
void ShutdownAudio() {
    StopAllAudio();
    AudiereDevice::device = NULL;
}

VA(0x00469d63, 0xb)
float GetEffectsVolume() {
    return gEffectsVolume;
}

VA(0x00469d6e, 0x15)
float ScaleSampleVolume(int volume) {
    return GetEffectsVolume() * (static_cast<float>(volume)) / 127.0f;
}

VA(0x00469d83, 0xb)
float GetMusicVolume() {
    return gMusicVolume;
}

VA(0x00469d8e, 0xf)
void StopAllAudio() {
    StopAllSamples();
    StopMusic();
}

VA(0x00469d9d, 0x1c)
void SetEffectsVolume(int level) {
    gEffectsVolume = VolumeLevel(level);
    UpdateAllSampleVolumes();
}

VA(0x00469db9, 0x1c)
void SetMusicVolume(int level) {
    gMusicVolume = VolumeLevel(level);
    UpdateMusicVolume();
}

VA(0x00469dd5, 0x1d)
void SetVolumes(int effects, int music) {
    SetEffectsVolume(effects);
    SetMusicVolume(music);
}
