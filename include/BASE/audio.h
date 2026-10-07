#ifndef HOMM1_BASE_AUDIO_H
#define HOMM1_BASE_AUDIO_H

#include <BASE/audioTypes.h>

// Buka replaces soundManager with free functions.
bool InitAudio();
void ShutdownAudio();
float VolumeLevel(i32 level);
float GetEffectsVolume();
float ScaleSampleVolume(i32 volume);
float GetMusicVolume();
void StopAllAudio();
void SetEffectsVolume(i32 level);
void SetMusicVolume(i32 level);
void SetVolumes(i32 effects, i32 music);

bool ShouldRepeatMusic(i32 track);
void PlayMusic(H1_ENUM_PARAM(MusicTrack, i32) track);
H1_ENUM_RETURN(MusicTrack, i32) GetCurrentTrack();
void StopMusic();
void UpdateMusicVolume();
void SetMusicSource(i32 source);
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
