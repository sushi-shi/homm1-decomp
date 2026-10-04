// Buka 2003 Ogg music playback. English game text remains in the catalogs.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/audiereBackend.h>
#include <SOURCE/KB.h>

#include <stdio.h>

// Buka retail VA 0x004cddec.
audiere::OutputStreamPtr AudiereMusic::stream;
// Buka retail VA 0x004cdf6c.
audiere::SampleSourcePtr AudiereMusic::source;
// Buka retail VA 0x004cdde8.
static int gMusicSuspensions;
// Buka retail VA 0x004a0d70.
static int gCurrentTrack = -1;
// Buka retail VA 0x004a0d74.
static int gCDTrackMap[100] = {
    2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
    22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, -1, -1, -1, -1, -1, -1, -1,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 50,
};

// Buka retail VA 0x004cddf0.
static char gMusicFilename[352];
// Buka retail VA 0x004cdf74.
static int gMusicPositions[100];
// Buka retail VA 0x004ce10c.
static int gMusicSource;

// Buka retail VA 0x004692b6, size 0x47.
bool ShouldRepeatMusic(int track) {
    if (track < 7 || (track >= 40 && track <= 42) || track == 53 || track == 54 || track == 47
        || track == 48 || track == 49 || (track >= 29 && track <= 32))
        return true;
    return false;
}

// Buka retail VA 0x004692fd, size 0x43b.
void PlayMusic(int track) {
    if (!GetAudioDevice())
        return;
    if (MusicSuspended())
        return;
    if (track == gCurrentTrack) {
        if (AudiereMusic::stream) {
            if (!AudiereMusic::stream->isPlaying())
                AudiereMusic::stream->play();
        }
        return;
    }
    if (track < 0) {
        StopMusic();
        return;
    }
    if (gMusicSource == 0) {
        sprintf(gMusicFilename, "%sHeroes%02d.ogg", gSoundPath, track);
    } else {
        int discTrack = gCDTrackMap[track];
        sprintf(
            gMusicFilename,
            "%s%s%02d-AudioTrack %02d.ogg",
            gcRegCDRomPath,
            "\\TRACKS\\",
            discTrack,
            discTrack
        );
    }
    audiere::SampleSourcePtr source = audiere::OpenSampleSource(gMusicFilename);
    if (source) {
        audiere::OutputStreamPtr stream = GetAudioDevice()->openStream(source.get());
        if (stream) {
            stream->setVolume(GetMusicVolume());
            stream->setRepeat(ShouldRepeatMusic(track));
            if (gMusicPositions[track] > 0 && stream->isSeekable())
                stream->setPosition(gMusicPositions[track]);
            stream->play();
            if (AudiereMusic::stream) {
                if (gCurrentTrack >= 0) {
                    if (AudiereMusic::stream->isSeekable() && ShouldRepeatMusic(gCurrentTrack))
                        gMusicPositions[gCurrentTrack] = AudiereMusic::stream->getPosition();
                    else
                        gMusicPositions[gCurrentTrack] = 0;
                }
                AudiereMusic::stream->stop();
            }
            AudiereMusic::stream = stream;
            AudiereMusic::source = source;
            gCurrentTrack = track;
        } else {
            StopMusic();
        }
    } else {
        StopMusic();
    }
}

// Buka retail VA 0x00469738, size 0xa.
int GetCurrentTrack() {
    return gCurrentTrack;
}

// Buka retail VA 0x00469742, size 0x164.
void StopMusic() {
    if (MusicSuspended())
        return;
    if (AudiereMusic::stream) {
        if (gCurrentTrack >= 0) {
            if (AudiereMusic::stream->isSeekable() && ShouldRepeatMusic(gCurrentTrack))
                gMusicPositions[gCurrentTrack] = AudiereMusic::stream->getPosition();
            else
                gMusicPositions[gCurrentTrack] = 0;
        }
        AudiereMusic::stream->stop();
        AudiereMusic::stream = NULL;
    }
    AudiereMusic::source = NULL;
    gCurrentTrack = -1;
}

// Buka retail VA 0x004698a6, size 0x55.
void UpdateMusicVolume() {
    if (MusicSuspended())
        return;
    if (!AudiereMusic::stream)
        return;
    AudiereMusic::stream->setVolume(GetMusicVolume());
}

// Buka retail VA 0x004698fb, size 0x128.
void SetMusicSource(int source) {
    if (AudiereMusic::stream) {
        AudiereMusic::stream->stop();
        AudiereMusic::stream = NULL;
    }
    AudiereMusic::source = NULL;
    for (int track = 0; track <= 99; ++track)
        gMusicPositions[track] = 0;
    gMusicSource = source;
    if (gCurrentTrack >= 0) {
        int oldTrack = gCurrentTrack;
        gCurrentTrack = -1;
        PlayMusic(oldTrack);
    }
}

// Buka retail VA 0x00469a23, size 0x36.
bool MusicPlaying() {
    if (!AudiereMusic::stream)
        return false;
    return AudiereMusic::stream->isPlaying();
}

// Buka retail VA 0x00469a59, size 0x12.
void SuspendMusic() {
    ++gMusicSuspensions;
}

// Buka retail VA 0x00469a6b, size 0x12.
void ResumeMusic() {
    --gMusicSuspensions;
}

// Buka retail VA 0x00469a7d, size 0x11.
bool MusicSuspended() {
    return gMusicSuspensions > 0;
}
