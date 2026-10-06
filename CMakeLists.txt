cmake_minimum_required(VERSION 3.20)
project(homm1 CXX)

# Native build of the game (and, once its sources are present, the scenario
# editor) on the SDL3 platform layer. The Visual C++ 6 build (build.py) is
# separate and unaffected.

include(CTest)

set(HOMM1_LOCALE "ru" CACHE STRING "Language compiled into the program (locales/<LANG>.json)")
option(HOMM1_SANITIZERS "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(HOMM1_SANITIZERS_RECOVER "Report undefined behaviour and continue (for surveys)" OFF)
option(HOMM1_WARNINGS_AS_ERRORS "Treat warnings in port and platform code as errors" ON)

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
if(HOMM1_SANITIZERS)
  set(HOMM1_SANITIZE_FLAGS -fsanitize=address,undefined -fno-omit-frame-pointer)
  if(NOT HOMM1_SANITIZERS_RECOVER)
    list(APPEND HOMM1_SANITIZE_FLAGS -fno-sanitize-recover=undefined)
  endif()
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
target_compile_definitions(homm1_platform PRIVATE HOMM1_CODEPAGE=${HOMM1_CODEPAGE})
target_link_libraries(homm1_platform PUBLIC SDL3::SDL3 PkgConfig::FFMPEG)

# --------------------------------------------------------------------------
# The game. The Win32-bound units (window and message loop, DirectDraw/WinG,
# Miles/Audiere sound, NetBIOS and serial transports) and the assembly
# routines are replaced by the units under src/PORT/.
# --------------------------------------------------------------------------

file(GLOB HOMM1_GAME_AUTHORED CONFIGURE_DEPENDS
  ${CMAKE_SOURCE_DIR}/src/BASE/*.cpp
  ${CMAKE_SOURCE_DIR}/src/SOURCE/*.cpp)
set(HOMM1_HOST_UNITS
  ${CMAKE_SOURCE_DIR}/src/BASE/Audio.cpp
  ${CMAKE_SOURCE_DIR}/src/SOURCE/kbwin.cpp
  ${CMAKE_SOURCE_DIR}/src/SOURCE/wingraph.cpp
  ${CMAKE_SOURCE_DIR}/src/SOURCE/netwin.cpp
  ${CMAKE_SOURCE_DIR}/src/SOURCE/comwin.cpp)
list(REMOVE_ITEM HOMM1_GAME_AUTHORED ${HOMM1_HOST_UNITS})
file(GLOB_RECURSE HOMM1_PORT_AUTHORED CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/PORT/*.cpp)
list(APPEND HOMM1_GAME_AUTHORED ${CMAKE_SOURCE_DIR}/vendor/lzhuf/encoder.cpp)
homm1_localized(HOMM1_GAME_SOURCES ${HOMM1_GAME_AUTHORED})
homm1_localized(HOMM1_PORT_SOURCES ${HOMM1_PORT_AUTHORED})
set_source_files_properties(${HOMM1_GAME_SOURCES} ${HOMM1_PORT_SOURCES} PROPERTIES GENERATED TRUE)

add_library(homm1_game OBJECT ${HOMM1_GAME_SOURCES})
add_dependencies(homm1_game homm1_localize)
add_library(homm1_port OBJECT ${HOMM1_PORT_SOURCES})
add_dependencies(homm1_port homm1_localize)

foreach(target homm1_game homm1_port)
  target_include_directories(${target} PRIVATE
    ${HOMM1_LOCALIZED}/include
    ${HOMM1_LOCALIZED}/vendor/lzhuf)
  target_compile_definitions(${target} PRIVATE HOMM1_PORT=1)
  if(NOT WIN32)
    target_compile_definitions(${target} PRIVATE __cdecl= __stdcall=)
  endif()
  target_compile_options(${target} PRIVATE
    ${HOMM1_SEMANTICS} ${HOMM1_SANITIZE_FLAGS}
    -include ${HOMM1_LOCALIZED}/include/PLATFORM/MsvcRuntime.h)
  target_link_libraries(${target} PRIVATE homm1_platform)
endforeach()
target_compile_options(homm1_game PRIVATE ${HOMM1_GAME_WARNINGS})
target_compile_options(homm1_port PRIVATE ${HOMM1_GAME_WARNINGS} -Wextra -Wno-unused-parameter)

add_executable(homm1 $<TARGET_OBJECTS:homm1_game> $<TARGET_OBJECTS:homm1_port>)
set_target_properties(homm1 PROPERTIES OUTPUT_NAME heroes)
target_link_libraries(homm1 PRIVATE homm1_platform)

# --------------------------------------------------------------------------
# Tests.
# --------------------------------------------------------------------------

if(BUILD_TESTING)
  add_subdirectory(tests/port)
endif()
