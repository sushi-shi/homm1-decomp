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

homm1_test(file_test
  SOURCES file_test.cpp)

# The portable drawing routines (the Windows build's assembly) on synthetic
# images in exactly-sized buffers.
homm1_test(blit_test
  SOURCES blit_test.cpp
  GAME_SOURCES
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/BITS.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/BMAP2.cpp
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/Icon2b.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/Icon2bc.cpp
    ${CMAKE_SOURCE_DIR}/src/PORT/BASE/TILE.cpp)

# The network save compressor: the encoder and the portable decoder.
homm1_test(lzhuf_test
  SOURCES lzhuf_roundtrip_test.cpp
  GAME_SOURCES ${CMAKE_SOURCE_DIR}/vendor/lzhuf/encoder.cpp ${CMAKE_SOURCE_DIR}/src/PORT/BASE/LZHUFDEC.cpp)
target_include_directories(lzhuf_test PRIVATE ${HOMM1_LOCALIZED}/vendor/lzhuf)

# Needs the game data and xvfb-run at test time; skipped otherwise.
add_test(NAME save_roundtrip
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/save_roundtrip.py $<TARGET_FILE:homm1>)
set_tests_properties(save_roundtrip PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400)
