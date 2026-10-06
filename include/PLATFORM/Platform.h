#ifndef HOMM1_PLATFORM_PLATFORM_H
#define HOMM1_PLATFORM_PLATFORM_H

// The host services of the native port. The game never includes this header:
// the port's replacements of the Windows units (src/PORT) translate between
// the game's host functions (kbwin.h, wingraph.h, audio.h, smack.h) and these
// interfaces. A backend is a set of source files implementing them; the one
// shipped is SDL3 with FFmpeg for compressed media (src/PLATFORM/SDL3).

#include <H1/Ints.h>

#include <cstddef>
#include <string>

namespace platform {

// ---------------------------------------------------------------- startup

struct StartupOptions {
    const char* title = "Heroes of Might and Magic";
    bool fullscreen = false;
    // Integer window scale for windowed play; 0 picks one that fits the desktop.
    int scale = 0;
};

bool Startup(const StartupOptions& options);
void Shutdown();

// The directory holding the running executable, with a trailing separator.
std::string ExecutableDirectory();
// Where the port keeps its own settings: $XDG_CONFIG_HOME/homm1 or the
// platform's equivalent, created on demand, with a trailing separator.
std::string ConfigDirectory();
// A variable from the environment, or empty.
std::string Environment(const char* name);

// ---------------------------------------------------------------- time

// Milliseconds on a monotonic clock. The epoch is arbitrary but never small:
// the game asserts on deadlines below 10000 and compares ticks as signed
// 32-bit values, so the count starts at 1,000,000 and stays below 2^31 for
// 24 days of play.
u32 Ticks();
void Sleep(u32 milliseconds);

// ---------------------------------------------------------------- text

// The game's text is single-byte in the language's Windows code page (the
// build defines HOMM1_CODEPAGE); the host wants UTF-8.
std::string ToUtf8(const char* text);

void ShowMessage(const char* title, const char* text);
#if defined(__GNUC__)
__attribute__((format(printf, 1, 2)))
#endif
void Log(const char* format, ...);

// ---------------------------------------------------------------- video

struct Color {
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
};

enum DisplayConstant {
    DISPLAY_WIDTH = 640,
    DISPLAY_HEIGHT = 480,
    PALETTE_SIZE = 256,
    CURSOR_SIZE = 32
};

// The display shows a 640x480 8-bit image through a 256-color palette, as the
// original's DirectDraw primary surface did. The game draws into its own
// buffer and copies finished rectangles here.
bool OpenDisplay();
void CloseDisplay();
bool DisplayOpen();

// Copies a rectangle of an 8-bit image (pitch bytes per row) to the display
// image at (destinationX, destinationY). Clipped to the display.
void UpdateDisplay(
    const u8* pixels,
    int pitch,
    int sourceX,
    int sourceY,
    int width,
    int height,
    int destinationX,
    int destinationY
);
// Replaces palette entries; the display changes at once, as a hardware
// palette would.
void SetPalette(const Color* colors, int first, int count);
// Shows pending display changes. Without force, presentation is limited to
// the display's refresh so that many small updates make one frame.
void Present(bool force);

void SetFullscreen(bool fullscreen);
bool Fullscreen();

// The display image as 24-bit RGB rows (for screenshots and tests).
void CaptureDisplay(u8* rgb);
bool SaveDisplayBmp(const char* hostPath);

// ---------------------------------------------------------------- cursor

// A 32x32 pointer image in the Windows cursor model. color holds a palette
// index per pixel (used where colorCursor is true). andMask and xorMask hold
// one bit per pixel, most significant bit first, 4 bytes per row; a pixel
// with its AND bit set is transparent (or inverts the screen when its XOR bit
// is also set); otherwise it shows the color index, or for a monochrome
// cursor black (XOR clear) or white (XOR set).
struct CursorImage {
    u8 color[CURSOR_SIZE * CURSOR_SIZE] = {};
    u8 andMask[CURSOR_SIZE * 4] = {};
    u8 xorMask[CURSOR_SIZE * 4] = {};
    bool colorCursor = true;
    int hotX = 0;
    int hotY = 0;
};

void SetCursorImage(const CursorImage* image);  // nullptr: the host's arrow
// Windows' display counter: the cursor shows while the count is >= 0.
void ShowCursor(bool show);

// ---------------------------------------------------------------- input

// Keys carry the PC keyboard's Set 1 scan code, as the Windows original saw
// them in WM_KEYDOWN: the extended keys (arrows, Home, End, Page Up/Down,
// Insert, Delete, right Ctrl/Alt, keypad Enter and divide) report the code of
// their non-extended twin, and keypad keys report keypad codes whatever the
// Num Lock state.
struct Event {
    enum Type {
        NONE,
        QUIT,
        KEY_DOWN,
        KEY_UP,
        MOUSE_MOVE,
        MOUSE_DOWN,
        MOUSE_UP,
        FOCUS_GAINED,
        FOCUS_LOST
    };
    enum Button {
        BUTTON_LEFT,
        BUTTON_RIGHT
    };
    Type type = NONE;
    int scanCode = 0;
    bool returnKey = false;  // the main or keypad Enter key
    int x = 0;  // display coordinates
    int y = 0;
    Button button = BUTTON_LEFT;
    bool doubleClick = false;
};

// Returns the next event; false when none is waiting. With a timeout, waits
// up to that many milliseconds for one.
bool PollEvent(Event& event, u32 timeoutMilliseconds = 0);
void MousePosition(int& x, int& y);
void CaptureMouse(bool capture);

// Scripted input for unattended runs: when $HOMM1_INPUT_REPLAY names a file,
// its timed actions are delivered as events (see docs/port/README.md).
void LoadInputReplay(const char* hostPath);

// ---------------------------------------------------------------- audio

bool OpenAudio();
void CloseAudio();
bool AudioOpen();

enum SampleFormat {
    SAMPLE_U8,
    SAMPLE_S16
};

using Voice = int;
constexpr Voice NO_VOICE = 0;

// Plays PCM from memory the caller keeps alive until the voice stops.
Voice PlaySample(
    const void* data,
    std::size_t bytes,
    int rate,
    int channels,
    SampleFormat format,
    float volume,
    bool loop
);
void StopVoice(Voice voice);
bool VoicePlaying(Voice voice);
void SetVoiceVolume(Voice voice, float volume);

// A compressed music file (Ogg Vorbis in the shipped game), streamed.
bool PlayMusic(const char* hostPath, bool loop, double startSeconds, float volume);
void StopMusic();
bool MusicPlaying();
double MusicPosition();
void SetMusicVolume(float volume);

// ---------------------------------------------------------------- movies

// A Smacker movie decoded frame by frame. Audio, when requested, plays on the
// audio device in step with Advance.
struct Movie;

struct MovieInfo {
    int width = 0;
    int height = 0;
    int frames = 0;
    double framesPerSecond = 15.0;
};

Movie* OpenMovie(const char* hostPath, bool withAudio, float volume);
void CloseMovie(Movie* movie);
const MovieInfo& MovieDetails(const Movie* movie);
// Decodes the next frame into the movie's frame buffer. False at the end.
bool DecodeMovieFrame(Movie* movie);
// The decoded frame: width*height palette indices, and its 256 RGB colors
// with full 8-bit components.
const u8* MovieFramePixels(const Movie* movie);
const u8* MovieFramePalette(const Movie* movie);

}  // namespace platform

#endif
