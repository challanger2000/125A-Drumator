cmake_minimum_required(VERSION 3.25)
project(Drumator VERSION 0.1.0 LANGUAGES CXX)
# Milestone 1: host-independent timing core. SDK integration is separate.
add_library(drumator_core INTERFACE)
target_compile_features(drumator_core INTERFACE cxx_std_17)
target_include_directories(drumator_core INTERFACE src)
include(CTest)
if(BUILD_TESTING)
  add_executable(drumator_core_tests tests/CoreTests.cpp)
  target_link_libraries(drumator_core_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_core_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_core_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_core_tests COMMAND drumator_core_tests)
  add_executable(drumator_voice_tests tests/SampleVoiceTests.cpp)
  target_link_libraries(drumator_voice_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_voice_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_voice_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_voice_tests COMMAND drumator_voice_tests)
endif()

option(DRUMATOR_BUILD_VST3 "Build SDK-pinned VST3 integration prototype" OFF)
if(DRUMATOR_BUILD_VST3)
  if(NOT DEFINED VST3_SDK_ROOT OR NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "VST3_SDK_ROOT must be the pinned recursive SDK checkout")
  endif()
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
  # Official SDK option: CI runner has no Administrator symlink rights.
  # Plugin bundle is built in-tree and published as a workflow artifact.
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/steinberg_sdk")
  smtg_add_vst3plugin(Drumator
    src/vst3/PluginIDs.h
    src/vst3/DrumatorProcessor.h
    src/vst3/DrumatorProcessor.cpp
    src/vst3/DrumatorController.h
    src/vst3/DrumatorController.cpp
    src/vst3/Factory.cpp
    src/core/SequencerCore.h
    src/core/SampleVoiceEngine.h
  )
  target_link_libraries(Drumator PRIVATE sdk drumator_core)
  target_compile_features(Drumator PRIVATE cxx_std_17)
  if(MSVC)
    target_compile_options(Drumator PRIVATE /W4)
  endif()
endif()
