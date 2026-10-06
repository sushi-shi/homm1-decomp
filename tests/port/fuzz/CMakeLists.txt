# Fuzz harnesses for the parsers of untrusted file data. Each harness is a
# LLVMFuzzerTestOneInput in <name>.cpp, built
#
#   - always as <name>_replay (ReplayMain.cpp), which ctest runs over the
#     checked-in regression inputs in regressions/<name> and, with
#     $HOMM1_DATA, over the shipped files the harness must accept;
#   - with -DHOMM1_FUZZERS=ON (Clang) as the libFuzzer program <name>.
#
# See docs/port/README.md for seeding and running them.

# The game's error exits throw instead of ending the process (FuzzWrap.cpp).
set(HOMM1_FUZZ_WRAPS
  -Wl,--wrap=_Z9FileErrorPc -Wl,--wrap=_Z8ShutDownPc -Wl,--wrap=_Z8MemErrorv)

# homm1_fuzz_harness(name SOURCES ... [GAME_SOURCES ...] [UNITS target]
#                    [REGRESSIONS] [SHIPPED pattern...] [INCLUDES dir...])
function(homm1_fuzz_harness name)
  cmake_parse_arguments(FUZZ "" "UNITS" "SOURCES;GAME_SOURCES;SHIPPED;INCLUDES" ${ARGN})
  homm1_localized(localized ${FUZZ_GAME_SOURCES})
  set_source_files_properties(${localized} PROPERTIES GENERATED TRUE)
  set(variants ${name}_replay)
  if(HOMM1_FUZZERS)
    list(APPEND variants ${name})
  endif()
  foreach(target ${variants})
    set(sources ${FUZZ_SOURCES} ${localized} FuzzSupport.cpp)
    if(target STREQUAL "${name}_replay")
      list(APPEND sources ReplayMain.cpp)
    endif()
    if(FUZZ_UNITS)
      list(APPEND sources FuzzWrap.cpp)
    endif()
    add_executable(${target} ${sources})
    add_dependencies(${target} homm1_localize)
    target_include_directories(${target} PRIVATE ${HOMM1_LOCALIZED}/include ${FUZZ_INCLUDES})
    target_compile_definitions(${target} PRIVATE HOMM1_PORT=1)
    if(NOT WIN32)
      target_compile_definitions(${target} PRIVATE __cdecl= __stdcall=)
    endif()
    target_compile_options(${target} PRIVATE ${HOMM1_SEMANTICS} ${HOMM1_SANITIZE_FLAGS}
      -include ${HOMM1_LOCALIZED}/include/PLATFORM/MsvcRuntime.h ${HOMM1_GAME_WARNINGS})
    if(FUZZ_UNITS)
      target_link_libraries(${target} PRIVATE ${FUZZ_UNITS})
      target_link_options(${target} PRIVATE ${HOMM1_FUZZ_WRAPS})
    else()
      target_link_libraries(${target} PRIVATE homm1_platform)
    endif()
    if(target STREQUAL "${name}")
      target_link_options(${target} PRIVATE -fsanitize=fuzzer)
    endif()
  endforeach()

  set(arguments ${CMAKE_CURRENT_SOURCE_DIR}/regressions/${name})
  if(FUZZ_SHIPPED)
    list(APPEND arguments --shipped ${FUZZ_SHIPPED})
  endif()
  add_test(NAME ${name}_replay COMMAND ${name}_replay ${arguments})
  set_tests_properties(${name}_replay PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endfunction()

# The record codecs alone: every record type, decoded, re-encoded and decoded
# again; the high score table and map header as the game reads them.
homm1_fuzz_harness(fuzz_records
  SOURCES fuzz_records.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/SOURCE/SAVEREC.cpp)

# The network save compressor's decoder on arbitrary streams, and the
# encoder and decoder round trip.
homm1_fuzz_harness(fuzz_lzhuf
  SOURCES fuzz_lzhuf.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/vendor/lzhuf/encoder.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/LZHUFDEC.cpp
  INCLUDES ${HOMM1_LOCALIZED}/vendor/lzhuf)

# Saved games through game::LoadGame in the game's own units.
homm1_fuzz_harness(fuzz_savegame
  SOURCES fuzz_savegame.cpp
  UNITS homm1_units
  SHIPPED "GAMES\\*.GM*@00-03" "DATA\\REMOTE.GAM@00-03")

# Maps and campaign maps through the file requester's header scan and
# game::NewMap (game::LoadMap and the new game set-up that interprets it).
# Every shipped map is started under every standard game set-up; the
# campaign maps, which only start as their campaign scenario, are read with
# game::LoadMap alone.
homm1_fuzz_harness(fuzz_map
  SOURCES fuzz_map.cpp
  UNITS homm1_units
  SHIPPED "MAPS\\*.MAP@00-0f")
add_test(NAME fuzz_map_campaign_replay
  COMMAND fuzz_map_replay --shipped "MAPS\\*.CMP@80-a3")
set_tests_properties(fuzz_map_campaign_replay PROPERTIES
  SKIP_RETURN_CODE 77 TIMEOUT 600 ENVIRONMENT HOMM1_FUZZ_LOAD_ONLY=1)

# Maps through the editor's reader, its checks and its writer.
homm1_fuzz_harness(fuzz_editor_map
  SOURCES fuzz_editor_map.cpp
  UNITS homm1_editor_units
  SHIPPED "MAPS\\*.MAP")

# The resource archive and the decoders that read their resource in place,
# with draws of what they decode into exactly-sized buffers.
homm1_fuzz_harness(fuzz_resources
  SOURCES fuzz_resources.cpp
  UNITS homm1_units
  SHIPPED "DATA\\HEROES.AGG@06")

# The high score tables through the high score screen's reader and display.
homm1_fuzz_harness(fuzz_highscore
  SOURCES fuzz_highscore.cpp
  UNITS homm1_units
  SHIPPED "DATA\\*.HS")
