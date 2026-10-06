# Native port tests. Tests that need the game data read it from $HOMM1_DATA
# and skip those checks without it.

function(homm1_test name)
  cmake_parse_arguments(TEST "" "" "SOURCES;GAME_SOURCES" ${ARGN})
  homm1_localized(localized ${TEST_GAME_SOURCES})
  set_source_files_properties(${localized} PROPERTIES GENERATED TRUE)
  add_executable(${name} ${TEST_SOURCES} ${localized})
  add_dependencies(${name} homm1_localize)
  target_include_directories(${name} PRIVATE ${HOMM1_LOCALIZED}/include)
  target_compile_definitions(${name} PRIVATE HOMM1_PORT=1)
  if(NOT WIN32)
    target_compile_definitions(${name} PRIVATE __cdecl= __stdcall=)
  endif()
  target_compile_options(${name} PRIVATE ${HOMM1_SEMANTICS} ${HOMM1_SANITIZE_FLAGS}
    -include ${HOMM1_LOCALIZED}/include/PLATFORM/MsvcRuntime.h ${HOMM1_GAME_WARNINGS})
  target_link_libraries(${name} PRIVATE homm1_platform)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

homm1_test(records_test
  SOURCES records_test.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/SOURCE/SAVEREC.cpp)

# The network and serial message codecs, with golden packets.
homm1_test(remote_records_test
  SOURCES remote_records_test.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/src/SOURCE/REMOTEREC.cpp ${CMAKE_SOURCE_DIR}/src/SOURCE/SAVEREC.cpp)

homm1_test(file_test
  SOURCES file_test.cpp)

# Finding the game folder as players set it up: quoted and trailing-separator
# HOMM1_DATA, the program's folder from anywhere, non-ASCII folders, file
# names in another case.
homm1_test(data_root_test
  SOURCES data_root_test.cpp)

# The movie and music decoders on the shipped files (needs HOMM1_DATA).
homm1_test(media_test
  SOURCES media_test.cpp)
set_tests_properties(media_test PROPERTIES SKIP_RETURN_CODE 77)

# The portable drawing routines (the Windows build's assembly) on synthetic
# images in exactly-sized buffers.
homm1_test(blit_test
  SOURCES blit_test.cpp
  GAME_SOURCES
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/BITS.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/BMAP2.cpp
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/Icon2b.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/Icon2bc.cpp
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/TILE.cpp)

# The WinHelp converter: a help file built by the test, and with HOMM1_HELP
# or HOMM1_DATA the game's help file.
homm1_test(help_test
  SOURCES help_test.cpp)
add_test(NAME help_game_test COMMAND help_test --game)
set_tests_properties(help_game_test PROPERTIES SKIP_RETURN_CODE 77)

# The network save compressor: the encoder and the portable decoder.
homm1_test(lzhuf_test
  SOURCES lzhuf_roundtrip_test.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/vendor/lzhuf/encoder.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/LZHUFDEC.cpp)
target_include_directories(lzhuf_test PRIVATE ${HOMM1_LOCALIZED}/vendor/lzhuf)

# Needs the game data at test time (it runs on SDL's dummy video driver); skipped otherwise.
add_test(NAME save_roundtrip
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/save_roundtrip.py $<TARGET_FILE:homm1>)
set_tests_properties(save_roundtrip PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400)

# A save on a full disk is reported and the game goes on (needs the game data,
# /dev/full).
add_test(NAME save_diskfull
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/save_diskfull.py $<TARGET_FILE:homm1>)
set_tests_properties(save_diskfull PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400)

# Every shipped map through the editor's reader and writer (needs HOMM1_DATA).
add_executable(editor_maps_test editor_maps_test.cpp)
target_link_libraries(editor_maps_test PRIVATE homm1_editor_units)
add_test(NAME editor_maps_test COMMAND editor_maps_test)
set_tests_properties(editor_maps_test PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)

# The retail out-of-bounds defects fixed in the shared game units (needs
# HOMM1_DATA; most useful in a sanitizer build).
add_executable(game_regressions_test game_regressions_test.cpp)
target_link_libraries(game_regressions_test PRIVATE homm1_units)
add_test(NAME game_regressions_test COMMAND game_regressions_test)
set_tests_properties(game_regressions_test PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

# The retail defects fixed in the scenario editor (needs HOMM1_DATA).
add_executable(editor_regressions_test editor_regressions_test.cpp)
target_link_libraries(editor_regressions_test PRIVATE homm1_editor_units)
add_test(NAME editor_regressions_test COMMAND editor_regressions_test)
set_tests_properties(editor_regressions_test PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)

# The long-running sanitizer survey (docs/port/README.md): opt-in, not a test.
option(HOMM1_SURVEY "Build the survey program that plays the game headless (tools/port/survey.py)" OFF)
if(HOMM1_SURVEY)
  add_executable(homm1_survey survey/survey.cpp)
  target_link_libraries(homm1_survey PRIVATE homm1_units)
  add_executable(homm1_editor_survey survey/editor_survey.cpp)
  target_link_libraries(homm1_editor_survey PRIVATE homm1_editor_units)
endif()

# Two instances play network, direct-connection and modem games on localhost
# with scripted input, through a battle, comparing state hashes at every
# hand-off (needs HOMM1_DATA and xvfb-run).
add_test(NAME net_game_test
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/net_game_test.py $<TARGET_FILE:homm1>)
set_tests_properties(net_game_test PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)

# The file parsers' fuzz harnesses: replayed over their checked-in
# regression inputs by default, built as libFuzzer programs with
# -DHOMM1_FUZZERS=ON.
add_subdirectory(fuzz)

# The text catalog: the templates are current and the sources carry no inline
# non-ASCII text or numeric escapes (catalog.py).
add_test(NAME catalog_check
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/catalog.py check --root ${CMAKE_SOURCE_DIR})
add_test(NAME catalog_update_check
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/catalog.py update --check --root ${CMAKE_SOURCE_DIR})
