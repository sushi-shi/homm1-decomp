#ifndef HOMM1_BASE_AUDIO_H
#define HOMM1_BASE_AUDIO_H

#include <BASE/audioTypes.h>

// Buka replaces soundManager with free functions.
bool InitAudio();
void ShutdownAudio();
float VolumeLevel(int level);
float GetEffectsVolume();
float ScaleSampleVolume(int volume);
float GetMusicVolume();
void StopAllAudio();
void SetEffectsVolume(int level);
void SetMusicVolume(int level);
void SetVolumes(int effects, int music);

bool ShouldRepeatMusic(int track);
void PlayMusic(H1_ENUM_PARAM(MusicTrack, int) track);
int GetCurrentTrack();
void StopMusic();
void UpdateMusicVolume();
void SetMusicSource(int source);
bool MusicPlaying();
void SuspendMusic();
void ResumeMusic();
bool MusicSuspended();

class sample;
void PlaySample(sample* resource);
sample* LoadPlaySample(char* name);
void StopSample(sample* resource);
void WaitSample(sample* resource);
void UpdateSampleVolume(sample* resource);
void StopAllSamples();
void UpdateAllSampleVolumes();
void SuspendSamples();
void ResumeSamples();
bool SamplesSuspended();

#endif
