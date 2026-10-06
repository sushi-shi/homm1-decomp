cmake_minimum_required(VERSION 3.20)
project(homm1 CXX)

# Native build of the game and the scenario editor on the SDL3 platform
# layer, for Linux and (cross-compiled with MinGW-w64, nix/windows.nix)
# Windows. The Visual C++ 6 build (build.py) is
# separate and unaffected.

include(CTest)

set(HOMM1_LOCALE "ru" CACHE STRING "Language compiled into the program (locales/<LANG>.json)")
option(HOMM1_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(HOMM1_SANITIZERS_RECOVER "Report undefined behaviour and continue (for surveys)" OFF)
option(HOMM1_WARNINGS_AS_ERRORS "Treat warnings in port and platform code as errors" ON)
option(HOMM1_FUZZERS "Build the libFuzzer harnesses of the file parsers (Clang; implies HOMM1_SANITIZERS)" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_BYTE_ORDER STREQUAL "BIG_ENDIAN")
  message(FATAL_ERROR "The game's data files are little-endian; big-endian hosts are not supported.")
endif()

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(SDL3 REQUIRED CONFIG)
# SDL marks its headers non-system, so the strict warnings below would apply
# to its macros (old-style casts) wherever the compiler wrapper does not
# already pass them as system headers, as Nix's does natively.
if(TARGET SDL3::Headers AND NOT CMAKE_VERSION VERSION_LESS "3.25")
  set_property(TARGET SDL3::Headers PROPERTY SYSTEM 1)
endif()
pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET libavformat libavcodec libavutil libswresample)

# --------------------------------------------------------------------------
# Localized sources. Game text is resolved at build time, as in the Visual
# C++ build: tools/port/localize.py writes a copy of src/, include/ and
# vendor/ with every localization::Tr("id") replaced by a literal in the
# language's code page. Only changed files are rewritten.
# --------------------------------------------------------------------------

set(HOMM1_LOCALIZED ${CMAKE_BINARY_DIR}/localized)
file(GLOB_RECURSE HOMM1_AUTHORED CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.h
  ${CMAKE_SOURCE_DIR}/include/*.h ${CMAKE_SOURCE_DIR}/vendor/*.cpp
  ${CMAKE_SOURCE_DIR}/vendor/*.h)
set(HOMM1_LOCALIZE_STAMP ${CMAKE_BINARY_DIR}/localized.stamp)
set(HOMM1_LOCALIZE_COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/port/localize.py
  --locale ${HOMM1_LOCALE} --out ${HOMM1_LOCALIZED} --stamp ${HOMM1_LOCALIZE_STAMP})
execute_process(COMMAND ${HOMM1_LOCALIZE_COMMAND} RESULT_VARIABLE HOMM1_LOCALIZE_RESULT)
if(NOT HOMM1_LOCALIZE_RESULT EQUAL 0)
  message(FATAL_ERROR "tools/port/localize.py failed")
endif()
# The localized copies are byproducts: Ninja re-reads their timestamps after
# the step runs, so a changed source recompiles in the same build.
set(HOMM1_LOCALIZED_FILES)
foreach(source ${HOMM1_AUTHORED})
  file(RELATIVE_PATH relative ${CMAKE_SOURCE_DIR} ${source})
  list(APPEND HOMM1_LOCALIZED_FILES ${HOMM1_LOCALIZED}/${relative})
endforeach()
add_custom_command(
  OUTPUT ${HOMM1_LOCALIZE_STAMP}
  BYPRODUCTS ${HOMM1_LOCALIZED_FILES}
  COMMAND ${HOMM1_LOCALIZE_COMMAND}
  DEPENDS ${HOMM1_AUTHORED} ${CMAKE_SOURCE_DIR}/tools/port/localize.py
    ${CMAKE_SOURCE_DIR}/catalog.py ${CMAKE_SOURCE_DIR}/locales/${HOMM1_LOCALE}.po
    ${CMAKE_SOURCE_DIR}/locales/${HOMM1_LOCALE}.json
  COMMENT "Localizing sources (${HOMM1_LOCALE})"
  VERBATIM)
add_custom_target(homm1_localize DEPENDS ${HOMM1_LOCALIZE_STAMP})

function(homm1_localized out)
  set(result)
  foreach(source ${ARGN})
    file(RELATIVE_PATH relative ${CMAKE_SOURCE_DIR} ${source})
    list(APPEND result ${HOMM1_LOCALIZED}/${relative})
  endforeach()
  set(${out} ${result} PARENT_SCOPE)
endfunction()

file(READ ${CMAKE_SOURCE_DIR}/locales/${HOMM1_LOCALE}.json HOMM1_LOCALE_JSON)
string(REGEX MATCH "\"codepage\": *([0-9]+)" HOMM1_CODEPAGE_FIELD "${HOMM1_LOCALE_JSON}")
set(HOMM1_CODEPAGE ${CMAKE_MATCH_1})

# --------------------------------------------------------------------------
# Compiler settings shared by every native target.
# --------------------------------------------------------------------------

# The original compiler's semantics the game relies on: plain char is signed,
# signed arithmetic wraps, and no type-based alias analysis.
set(HOMM1_SEMANTICS -fsigned-char -fwrapv -fno-strict-aliasing)
set(HOMM1_SANITIZE_FLAGS)
if(HOMM1_FUZZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "HOMM1_FUZZERS needs Clang (libFuzzer): -DCMAKE_CXX_COMPILER=clang++")
  endif()
  set(HOMM1_SANITIZERS ON)
endif()
if(HOMM1_SANITIZERS)
  set(HOMM1_SANITIZE_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
  if(NOT HOMM1_SANITIZERS_RECOVER)
    list(APPEND HOMM1_SANITIZE_FLAGS -fno-sanitize-recover=undefined)
  endif()
endif()
# Coverage instrumentation for libFuzzer everywhere; only the fuzzers link
# the fuzzing engine itself (tests/port/fuzz).
if(HOMM1_FUZZERS)
  list(APPEND HOMM1_SANITIZE_FLAGS -fsanitize=fuzzer-no-link)
endif()

set(HOMM1_STRICT_WARNINGS
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wnon-virtual-dtor
  -Wold-style-cast -Wcast-align -Wformat=2)
if(HOMM1_WARNINGS_AS_ERRORS)
  list(APPEND HOMM1_STRICT_WARNINGS -Werror)
endif()

# The reconstructed game sources: warnings that flag real portability defects
# are errors; the rest of the legacy style is left alone.
set(HOMM1_GAME_WARNINGS
  -Wall
  -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-label
  -Wno-unused-function -Wno-parentheses -Wno-misleading-indentation
  -Wno-char-subscripts -Wno-unknown-pragmas -Wno-sign-compare
  -Wno-class-memaccess -Wno-address-of-packed-member -Wno-unused-value
  -Wno-write-strings -Wno-format-overflow -Wno-format-truncation
  -Wno-stringop-truncation -Wno-stringop-overflow -Wno-array-bounds
  -Wno-maybe-uninitialized -Wno-uninitialized -Wno-dangling-pointer
  -Wno-restrict -Wno-nonnull -Wno-unused-result -Wno-narrowing -Wno-register
  -Werror=return-type -Werror=int-to-pointer-cast -Werror=mismatched-new-delete
  -Wformat-security -Wno-error=format-security -Wdelete-non-virtual-dtor)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # Some of the options above are GCC's alone.
  list(APPEND HOMM1_GAME_WARNINGS -Wno-unknown-warning-option)
endif()

# --------------------------------------------------------------------------
# The browser build (Emscripten). The game's own loops block - menus,
# dialogs, combat and the adventure map each run a loop that polls for input
# - so the program cannot return to the browser between frames. ASYNCIFY
# lets those loops yield where they already wait: SDL_Delay, presenting a
# frame and the event poll unwind the WebAssembly stack to the browser and
# rewind it afterwards. JSPI would do the same with less code, but is not
# available in every browser yet (docs/port/README.md).
# --------------------------------------------------------------------------

if(EMSCRIPTEN)
  set(HOMM1_WEB_PRE_JS ${CMAKE_SOURCE_DIR}/src/PLATFORM/Web/pre.js)
  set(HOMM1_WEB_LINK_OPTIONS
    -sASYNCIFY=1 -sASYNCIFY_STACK_SIZE=262144
    -sSTACK_SIZE=8MB -sINITIAL_MEMORY=64MB -sALLOW_MEMORY_GROWTH=1
    -sFORCE_FILESYSTEM=1 -lidbfs.js
    -sINVOKE_RUN=0 -sEXIT_RUNTIME=1 -sENVIRONMENT=web
    "-sEXPORTED_RUNTIME_METHODS=FS,callMain,ENV"
    --pre-js ${HOMM1_WEB_PRE_JS})
endif()

# --------------------------------------------------------------------------
# Platform layer: host services behind include/PLATFORM interfaces.
# --------------------------------------------------------------------------

file(GLOB HOMM1_PLATFORM_COMMON CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/PLATFORM/*.cpp)
file(GLOB HOMM1_PLATFORM_SDL3 CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/PLATFORM/SDL3/*.cpp)
add_library(homm1_platform STATIC ${HOMM1_PLATFORM_COMMON} ${HOMM1_PLATFORM_SDL3})
target_include_directories(homm1_platform PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_compile_options(homm1_platform PRIVATE ${HOMM1_SEMANTICS} ${HOMM1_STRICT_WARNINGS}
  ${HOMM1_SANITIZE_FLAGS})
target_link_options(homm1_platform PUBLIC ${HOMM1_SANITIZE_FLAGS})
target_compile_definitions(homm1_platform PRIVATE HOMM1_PORT=1 HOMM1_CODEPAGE=${HOMM1_CODEPAGE})
target_link_libraries(homm1_platform PUBLIC SDL3::SDL3 PkgConfig::FFMPEG)
if(WIN32)
  # MinGW: the C++ runtime is linked in; the programs then need only SDL3,
  # FFmpeg and the compiler's thread library beside them.
  target_link_options(homm1_platform PUBLIC -static-libgcc -static-libstdc++)
  # C99 formatting (%zu, %lld) for the log, as on other hosts.
  target_compile_definitions(homm1_platform PRIVATE __USE_MINGW_ANSI_STDIO=1)
endif()

# --------------------------------------------------------------------------
# The programs: the game (heroes) and the scenario editor (heroes-editor).
# Each compiles the units its Visual C++ build lists in build.json, with the
# Win32-bound units (window and message loop, DirectDraw/WinG, Miles/Audiere
# sound, NetBIOS and serial transports) and the assembly routines replaced by
# their counterparts under src/PORT/ (tools/port/units.py).
# --------------------------------------------------------------------------

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/build.json ${CMAKE_SOURCE_DIR}/tools/port/units.py)

function(homm1_program name target output rc)
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/port/units.py --target ${target}
    OUTPUT_VARIABLE units RESULT_VARIABLE result OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(NOT result EQUAL 0)
    message(FATAL_ERROR "tools/port/units.py --target ${target} failed")
  endif()
  string(REPLACE "\n" ";" units "${units}")
  set(sources)
  foreach(unit ${units})
    list(APPEND sources ${HOMM1_LOCALIZED}/${unit})
  endforeach()
  set_source_files_properties(${sources} PROPERTIES GENERATED TRUE)

  # The menus and About box from the program's resource script.
  set(generated ${CMAKE_BINARY_DIR}/generated/${target})
  set(menus ${generated}/menus_${target}.inc)
  add_custom_command(
    OUTPUT ${menus}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/port/menus.py
      --rc ${CMAKE_SOURCE_DIR}/${rc} --locale ${HOMM1_LOCALE} --out ${menus}
    DEPENDS ${CMAKE_SOURCE_DIR}/${rc} ${CMAKE_SOURCE_DIR}/tools/port/menus.py
      ${CMAKE_SOURCE_DIR}/catalog.py ${CMAKE_SOURCE_DIR}/locales/${HOMM1_LOCALE}.po
    COMMENT "Generating the ${target} menus"
    VERBATIM)

  # The program's units without its entry point, for the program and for
  # tests that drive it.
  add_library(${name}_units OBJECT ${sources} ${menus})
  add_dependencies(${name}_units homm1_localize)
  target_include_directories(${name}_units PUBLIC
    ${HOMM1_LOCALIZED}/include ${HOMM1_LOCALIZED}/vendor/lzhuf ${generated})
  target_compile_definitions(${name}_units PUBLIC HOMM1_PORT=1)
  if(target STREQUAL "editor")
    target_compile_definitions(${name}_units PUBLIC HOMM1_EDITOR=1)
  endif()
  if(NOT WIN32)
    target_compile_definitions(${name}_units PUBLIC __cdecl= __stdcall=)
  endif()
  target_compile_options(${name}_units PUBLIC
    ${HOMM1_SEMANTICS} ${HOMM1_SANITIZE_FLAGS} ${HOMM1_GAME_WARNINGS}
    -include ${HOMM1_LOCALIZED}/include/PLATFORM/MsvcRuntime.h)
  target_link_libraries(${name}_units PUBLIC homm1_platform)

  set_source_files_properties(${HOMM1_LOCALIZED}/src/PORT/SOURCE/Main.cpp PROPERTIES GENERATED TRUE)
  add_executable(${name} ${HOMM1_LOCALIZED}/src/PORT/SOURCE/Main.cpp)
  # A Windows GUI program: no console window (the log is routed in Host.cpp).
  set_target_properties(${name} PROPERTIES OUTPUT_NAME ${output} WIN32_EXECUTABLE TRUE)
  target_link_libraries(${name} PRIVATE ${name}_units)
  if(EMSCRIPTEN)
    target_link_options(${name} PRIVATE ${HOMM1_WEB_LINK_OPTIONS})
    set_property(TARGET ${name} APPEND PROPERTY LINK_DEPENDS ${HOMM1_WEB_PRE_JS})
  endif()
endfunction()

homm1_program(homm1 game heroes src/SOURCE/Heroes.rc)
homm1_program(homm1_editor editor heroes-editor src/EDITOR/Editor.rc)

# The help converter as a command-line tool (tools/help).
add_executable(homm1_hlp2html ${CMAKE_SOURCE_DIR}/tools/help/hlp2html.cpp)
set_target_properties(homm1_hlp2html PROPERTIES OUTPUT_NAME homm1-hlp2html)
target_compile_options(homm1_hlp2html PRIVATE ${HOMM1_SEMANTICS} ${HOMM1_STRICT_WARNINGS}
  ${HOMM1_SANITIZE_FLAGS})
target_link_libraries(homm1_hlp2html PRIVATE homm1_platform)

# --------------------------------------------------------------------------
# Tests.
# --------------------------------------------------------------------------

if(BUILD_TESTING)
  add_subdirectory(tests/port)
endif()
