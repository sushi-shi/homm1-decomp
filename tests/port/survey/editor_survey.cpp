// The editor's part of the sanitizer survey (opt-in: -DHOMM1_SURVEY=ON;
// driven by tools/port/survey.py). It starts the scenario editor headless
// and generates random maps:
//
//   editor_survey random SEED COUNT [OUTDIR]
//
// Each map is generated with random terrain shares, densities and town
// placement (the new map dialog is answered by the "key return" input
// replay the runner passes in), saved, read back and saved again; the two
// files must match. With OUTDIR the maps are copied there, so that the game
// part of the survey can play them.

#include <H1/Ints.h>

#include <BASE/executive.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/editManager.h>
#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <string>
#include <vector>

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);

namespace {

std::mt19937 gRandom;

double Share(double low, double high) {
    return std::uniform_real_distribution<double>(low, high)(gRandom);
}

void Progress(const char* format, ...) __attribute__((format(printf, 1, 2)));
void Progress(const char* format, ...) {
    va_list arguments;
    va_start(arguments, format);
    std::vprintf(format, arguments);
    va_end(arguments);
    std::putchar('\n');
    std::fflush(stdout);
}

std::vector<u8> ReadGameFile(const char* gamePath) {
    char resolved[FILE_PATH_CAPACITY];
    std::vector<u8> bytes;
    if (!FileResolve(gamePath, FILE_OPEN_READ, resolved, sizeof(resolved)))
        return bytes;
    std::FILE* file = std::fopen(resolved, "rb");
    if (file == nullptr)
        return bytes;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<u8>(c));
    std::fclose(file);
    return bytes;
}

// The generator's settings, as a player could set them in the dialog.
void RandomSettings() {
    for (int i = 0; i < EDITOR_TERRAIN_COUNT; i++)
        gTerrainPercent[i] = std::uniform_int_distribution<int>(0, 4)(gRandom) == 0 ? 0.0 : Share(0, 60);
    for (int i = 0; i < EDITOR_GENERATOR_DENSITY_COUNT; i++)
        gDensityPercent[i] = Share(0, 100);
    gScatterTowns = std::uniform_int_distribution<int>(0, 1)(gRandom);
    gSaveUnseen = 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 4 || std::strcmp(argv[1], "random") != 0) {
        std::fprintf(stderr, "usage: editor_survey random SEED COUNT [OUTDIR]\n");
        return 2;
    }
    int seed = std::atoi(argv[2]);
    int count = std::atoi(argv[3]);
    std::string outDirectory = argc > 4 ? argv[4] : "";
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    setenv("SDL_AUDIO_DRIVER", "dummy", 0);
    setenv("HOMM1_NO_DIALOGS", "1", 1);
    if (!KBStartHost(nullptr, "", 0))
        return 2;
    if (gExec->InitSystem() != 0)
        return 2;
    if (gExec->AddManager(gEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != 0)
        return 2;
    gRandom.seed(static_cast<u32>(seed));
    srand(static_cast<u32>(seed));
    int failures = 0;
    for (int index = 0; index < count; index++) {
        RandomSettings();
        Progress("map %d: terrain %.0f %.0f %.0f %.0f %.0f %.0f %.0f, density %.0f %.0f %.0f %.0f %.0f, "
                 "%s towns",
                 index, gTerrainPercent[0], gTerrainPercent[1], gTerrainPercent[2], gTerrainPercent[3],
                 gTerrainPercent[4], gTerrainPercent[5], gTerrainPercent[6], gDensityPercent[0],
                 gDensityPercent[1], gDensityPercent[2], gDensityPercent[3], gDensityPercent[4],
                 gScatterTowns ? "scattered" : "central");
        gEditManager->GenerateRandomMap();
        char name[32];
        std::snprintf(name, sizeof(name), "R%03d%03d1.MAP", seed % 1000, index % 1000);
        if (gEditManager->SaveMap(name) != BASE_MANAGER_SUCCESS) {
            Progress("FINDING editor-save: map %d: could not be saved", index);
            failures++;
            continue;
        }
        char path[64];
        std::snprintf(path, sizeof(path), "MAPS\\%s", name);
        std::vector<u8> first = ReadGameFile(path);
        RecordReader reader;
        if (!reader.LoadFile(path) || !gEditManager->ReadMapFile(reader)) {
            Progress("FINDING editor-read: map %d: the editor cannot read the map it saved", index);
            failures++;
            continue;
        }
        gEditManager->SaveMap(name);
        std::vector<u8> second = ReadGameFile(path);
        if (first.empty() || first != second) {
            Progress("FINDING editor-roundtrip: map %d: saving the read map gives a different file "
                     "(%zu vs %zu bytes)",
                     index, first.size(), second.size());
            failures++;
        } else {
            Progress("map %d: saved, read and saved again identically (%zu bytes)", index, first.size());
        }
        if (!outDirectory.empty()) {
            std::FILE* out = std::fopen((outDirectory + "/" + name).c_str(), "wb");
            if (out != nullptr) {
                std::fwrite(first.data(), 1, first.size(), out);
                std::fclose(out);
            }
        }
    }
    Progress("survey done");
    std::fflush(stdout);
    std::_Exit(failures != 0 ? 1 : 0);
}
