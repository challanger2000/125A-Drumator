cmake_minimum_required(VERSION 3.25)
project(Drumator VERSION 0.1.0 LANGUAGES CXX)
# Milestone 1: host-independent timing core. SDK integration is separate.
add_library(drumator_core INTERFACE)
target_compile_features(drumator_core INTERFACE cxx_std_23)
target_include_directories(drumator_core INTERFACE src)
include(CTest)
if(BUILD_TESTING)
  add_executable(drumator_song_chain_tests tests/SongChainTests.cpp)
  target_include_directories(drumator_song_chain_tests PRIVATE src)
  add_test(NAME drumator_song_chain_tests COMMAND drumator_song_chain_tests)
  # Release-mode safety: always-active checks, including an intentionally
  # failing test whose nonzero exit must be observed by CTest.
  add_executable(drumator_song_chain_negative_tests tests/SongChainTests.cpp)
  target_link_libraries(drumator_song_chain_negative_tests PRIVATE drumator_core)
  target_compile_definitions(drumator_song_chain_negative_tests PRIVATE DRUMATOR_TEST_EXPECTED_FAILURE)
  add_test(NAME drumator_song_chain_negative_tests COMMAND drumator_song_chain_negative_tests)
  set_tests_properties(drumator_song_chain_negative_tests PROPERTIES WILL_FAIL TRUE)

  add_executable(drumator_ui_hit_tests tests/UiHeaderHitZonesTests.cpp)
  target_link_libraries(drumator_ui_hit_tests PRIVATE drumator_core)
  add_test(NAME drumator_ui_hit_tests COMMAND drumator_ui_hit_tests)
  add_executable(drumator_pattern_timeline_tests tests/PatternEventTimelineTests.cpp)
  target_link_libraries(drumator_pattern_timeline_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_pattern_timeline_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_pattern_timeline_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_pattern_timeline_tests COMMAND drumator_pattern_timeline_tests)
  add_executable(drumator_overlay_hit_tests tests/UiOverlayHitZonesTests.cpp)
  target_link_libraries(drumator_overlay_hit_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_overlay_hit_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_overlay_hit_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_overlay_hit_tests COMMAND drumator_overlay_hit_tests)
  if(MSVC)
    target_compile_options(drumator_song_chain_tests PRIVATE /W4 /WX)
    target_compile_options(drumator_song_chain_negative_tests PRIVATE /W4 /WX)
    target_compile_options(drumator_ui_hit_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_song_chain_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(drumator_song_chain_negative_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
    target_compile_options(drumator_ui_hit_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_executable(drumator_pattern_undo_tests tests/PatternUndoTests.cpp)
  target_link_libraries(drumator_pattern_undo_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_pattern_undo_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_pattern_undo_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_pattern_undo_tests COMMAND drumator_pattern_undo_tests)
  add_executable(drumator_velocity_lane_tests tests/VelocityLaneTests.cpp)
  target_link_libraries(drumator_velocity_lane_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_velocity_lane_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_velocity_lane_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_velocity_lane_tests COMMAND drumator_velocity_lane_tests)
  add_executable(drumator_recall_ui_tests tests/RecallUIContractTests.cpp)
  target_link_libraries(drumator_recall_ui_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_recall_ui_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_recall_ui_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_recall_ui_tests COMMAND drumator_recall_ui_tests)
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
  add_executable(drumator_wav_tests tests/WavDecoderTests.cpp)
  target_link_libraries(drumator_wav_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_wav_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_wav_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_wav_tests COMMAND drumator_wav_tests)
  add_executable(drumator_exchange_tests tests/SampleExchangeTests.cpp)
  target_link_libraries(drumator_exchange_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_exchange_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_exchange_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_exchange_tests COMMAND drumator_exchange_tests)
  add_executable(drumator_trigger_tests tests/TriggerBatchTests.cpp)
  target_link_libraries(drumator_trigger_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_trigger_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_trigger_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_trigger_tests COMMAND drumator_trigger_tests)
  add_executable(drumator_atomic_tests tests/AtomicPatternTests.cpp)
  target_link_libraries(drumator_atomic_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_atomic_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_atomic_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_atomic_tests COMMAND drumator_atomic_tests)
  add_executable(drumator_visible_rows_tests tests/VisibleRowsTests.cpp)
  target_link_libraries(drumator_visible_rows_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_visible_rows_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_visible_rows_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_visible_rows_tests COMMAND drumator_visible_rows_tests)
  add_executable(drumator_kit_tests tests/KitStateTests.cpp)
  target_link_libraries(drumator_kit_tests PRIVATE drumator_core)
  if(MSVC)
    target_compile_options(drumator_kit_tests PRIVATE /W4 /WX)
  else()
    target_compile_options(drumator_kit_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
  add_test(NAME drumator_kit_tests COMMAND drumator_kit_tests)

  # Build ALL local CTest executables before invoking ctest. A stale
  # explicit GitHub Actions target list previously omitted the WAV test.
  add_custom_target(drumator_test_suite)
  get_property(_drumator_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
  foreach(_drumator_target IN LISTS _drumator_targets)
    if(_drumator_target MATCHES "^drumator_.*_tests$")
      add_dependencies(drumator_test_suite ${_drumator_target})
    endif()
  endforeach()
endif()

option(DRUMATOR_BUILD_VST3 "Build SDK-pinned VST3 integration prototype" OFF)
if(DRUMATOR_BUILD_VST3)
  if(NOT DEFINED VST3_SDK_ROOT OR NOT EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR "VST3_SDK_ROOT must be the pinned recursive SDK checkout")
  endif()
  set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(SMTG_ENABLE_VSTGUI_SUPPORT ON CACHE BOOL "" FORCE)
  # Official SDK option: CI runner has no Administrator symlink rights.
  # Plugin bundle is built in-tree and published as a workflow artifact.
  set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)
  # Integration-only: SDK-supported legacy Windows single-file VST3.
  # Avoid bundle icon/metadata hooks until 125A assets are added and verified.
  set(SMTG_CREATE_BUNDLE_FOR_WINDOWS OFF CACHE BOOL "" FORCE)
  # Execute official Steinberg Validator AFTER constructing the standard
  # Windows bundle directory in CI, not on the transient single-file binary.
  set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
  set(SMTG_CREATE_MODULE_INFO OFF CACHE BOOL "" FORCE)
  add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/steinberg_sdk")
  # GitHub Windows GITHUB_WORKSPACE may use backslashes. The Steinberg
  # cmake macro interpolates source paths into a CMake parse expression;
  # normalize the official SDK source location first.
  file(TO_CMAKE_PATH "${VST3_SDK_ROOT}" _DrumatorSdkRoot)
  if(WIN32)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/PatchVstguiWin32Drop.cmake")
    phraseator_patch_vstgui_windows_drop("${_DrumatorSdkRoot}/vstgui4/vstgui/lib/platform/win32/win32datapackage.cpp")
  endif()
  # Decoder pinned to an immutable upstream commit, used only at import time.
  set(_drflac_dir "${CMAKE_BINARY_DIR}/third_party")
  file(MAKE_DIRECTORY "${_drflac_dir}")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/mackron/dr_libs/dfe8377631000664666519fdb83da193fd8037f4/dr_flac.h"
    "${_drflac_dir}/dr_flac.h"
    STATUS _drflac_status TLS_VERIFY ON)
  list(GET _drflac_status 0 _drflac_code)
  if(NOT _drflac_code EQUAL 0)
    message(FATAL_ERROR "Failed to obtain pinned dr_flac decoder: ${_drflac_status}")
  endif()
  smtg_add_vst3plugin(Drumator
    ${_DrumatorSdkRoot}/public.sdk/source/main/dllmain.cpp
    src/vst3/PluginIDs.h
    src/vst3/DrumatorProcessor.h
    src/vst3/DrumatorProcessor.cpp
    src/vst3/DrumatorController.h
    src/vst3/DrumatorController.cpp
    src/vst3/FlacImport.cpp
    src/core/FlacImport.h
    src/vst3/DrumatorView.h
    src/vst3/DrumatorView.cpp
    src/core/AtomicPatternBank.h
    src/core/VisibleRows.h
    src/core/KitState.h
    resource/Drumator.uidesc
    src/vst3/Factory.cpp
    src/core/SequencerCore.h
    src/core/SampleVoiceEngine.h
    src/core/TriggerBatch.h
    src/core/SampleExchange.h
    src/core/WavPcm16Decoder.h
  )
  target_link_libraries(Drumator PRIVATE sdk drumator_core vstgui_support)
  target_include_directories(Drumator PRIVATE "${_drflac_dir}")
  smtg_target_add_plugin_resources(Drumator
      RESOURCES resource/Drumator.uidesc)
  # Steinberg official Win32 bundle resource setup, even though CI assembles
  # the final bundle manually to avoid administrator symlink privileges.
  # The official helper macro is evaluated in this parent project scope.
  # Explicitly supply its official public.sdk root as in Steinberg examples.
  set(public_sdk_SOURCE_DIR "${_DrumatorSdkRoot}/public.sdk")
  smtg_target_prepare_bundle(Drumator)
  # With the SDK's legacy single-file compilation plus manual assembly,
  # SMTG_MODULE_IS_BUNDLE must ALSO be defined for the separately compiled
  # vstgui_support static library. Otherwise vstguieditor.cpp never calls
  # setupVSTGUIBundleSupport, the .uidesc cannot be found on Win32,
  # and VST3Editor::init() leaves the requested GUI rectangle at 0x0.
  target_compile_definitions(vstgui_support PRIVATE SMTG_MODULE_IS_BUNDLE=1)
  target_compile_features(Drumator PRIVATE cxx_std_23)
  # Build the official Steinberg hosting helpers for a full in-process
  # test of the packaged plugin and actual output buffers.
  add_library(drumator_host_fixture STATIC
    ${_DrumatorSdkRoot}/public.sdk/source/vst/hosting/hostclasses.cpp
    ${_DrumatorSdkRoot}/public.sdk/source/vst/hosting/pluginterfacesupport.cpp
    ${_DrumatorSdkRoot}/public.sdk/source/vst/hosting/eventlist.cpp
    ${_DrumatorSdkRoot}/public.sdk/source/vst/hosting/parameterchanges.cpp)
  target_link_libraries(drumator_host_fixture PRIVATE sdk)
  add_executable(drumator_audio_host_smoke tests/PackagedAudioHostSmoke.cpp
      src/vst3/FlacImport.cpp)
  target_link_libraries(drumator_audio_host_smoke PRIVATE
      sdk drumator_core drumator_host_fixture)
  target_include_directories(drumator_audio_host_smoke PRIVATE "${_drflac_dir}")
  target_compile_features(drumator_audio_host_smoke PRIVATE cxx_std_23)
  if(MSVC)
    target_compile_options(drumator_audio_host_smoke PRIVATE /W4 /WX)
  endif()
  add_executable(drumator_editor_smoke tests/EditorGeometrySmoke.cpp)
  target_link_libraries(drumator_editor_smoke PRIVATE sdk drumator_core)
  target_compile_features(drumator_editor_smoke PRIVATE cxx_std_23)
  if(MSVC)
    target_compile_options(drumator_editor_smoke PRIVATE /W4 /WX)
  endif()
  if(MSVC)
    target_compile_options(Drumator PRIVATE /W4)
  endif()
endif()
