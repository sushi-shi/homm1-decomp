// Maps through the scenario editor's own reader, in the editor's units
// started headless: editManager::ReadMapFile, then what the editor's Save
// does with a map it has read (CheckObjects, UpdateTriggers, WriteMapFile),
// as editor_maps_test does for the shipped maps.

#include "FuzzSupport.h"

#include <BASE/executive.h>
#include <EDITOR/editManager.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_editor_map");
    std::string root = FuzzScratchGame();
    FuzzStartHost(root);
    bool opened = false;
    RunGame([&] { opened = gExec->AddManager(gEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED) == 0; });
    if (!opened) {
        std::fprintf(stderr, "the editor did not open\n");
        std::_Exit(1);
    }
    gFuzzRejected = false;
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 20))
        return 0;
    RunGame([&] {
        RecordReader reader(data, static_cast<i32>(size));
        // As in an editor started afresh for the map.
        gEditManager->FreeMapExtras();
        std::memset(gEditManager->m_map.cellPairs, 0, sizeof(gEditManager->m_map.cellPairs));
        if (!gEditManager->ReadMapFile(reader)) {
            gFuzzRejected = true;
            return;
        }
        gEditManager->ClearErrors();
        gEditManager->CheckObjects();
        gEditManager->UpdateTriggers();
        RecordWriter writer;
        gEditManager->WriteMapFile(writer);
        gEditManager->ClearErrors();
    });
    return 0;
}
