# Instrument exact current production sources for deterministic interleavings.
# No source-tree file is rewritten. Fail closed if the expected anchor changes.
set(_qa "${CMAKE_CURRENT_BINARY_DIR}/qa-source")
file(MAKE_DIRECTORY "${_qa}/core" "${_qa}/vst3")
function(qa_replace variable anchor replacement)
  string(FIND "${${variable}}" "${anchor}" _first)
  if(_first EQUAL -1)
    message(FATAL_ERROR "QA instrumentation anchor missing: ${anchor}")
  endif()
  string(LENGTH "${anchor}" _length)
  math(EXPR _next "${_first}+${_length}")
  string(SUBSTRING "${${variable}}" ${_next} -1 _tail)
  string(FIND "${_tail}" "${anchor}" _second)
  if(NOT _second EQUAL -1)
    message(FATAL_ERROR "QA instrumentation anchor ambiguous: ${anchor}")
  endif()
  string(REPLACE "${anchor}" "${replacement}" _updated "${${variable}}")
  set(${variable} "${_updated}" PARENT_SCOPE)
endfunction()
foreach(_name DrumatorController DrumatorView DrumatorProcessor)
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/vst3/${_name}.h" _header)
  qa_replace(_header "private:\n" "private:\n friend struct IntegrationAccess; // test observation only\n")
  if(_name STREQUAL "DrumatorProcessor")
    qa_replace(_header "count_.store(static_cast<std::uint8_t>(chain.count()),std::memory_order_relaxed);"
      "count_.store(static_cast<std::uint8_t>(chain.count()),std::memory_order_relaxed);\n  drumator::qa::probe(drumator::qa::songMidPublish);")
  endif()
  file(WRITE "${_qa}/vst3/${_name}.h" "${_header}")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/vst3/${_name}.cpp" _source)
  if(_name STREQUAL "DrumatorProcessor")
    qa_replace(_source "std::int32_t cursor=0;"
      "std::int32_t cursor=0;\n   drumator::qa::probe(drumator::qa::auditAfterLengthRead);")
    qa_replace(_source "const auto selectionAtStart=selected_.snapshot();"
      "const auto selectionAtStart=selected_.snapshot();\n    drumator::qa::probe(drumator::qa::restoreSnapshot);")
    qa_replace(_source "previewPending_.store(0,std::memory_order_release);\n    kit_=std::move(candidate);"
      "drumator::qa::probe(drumator::qa::restoreMidCommit);\n    previewPending_.store(0,std::memory_order_release);\n    kit_=std::move(candidate);")
    qa_replace(_source "selected_.restoreIfUnchanged(selectionAtStart,kit_.selected);"
      "drumator::qa::probe(drumator::qa::restoreBeforeSelection);\n    selected_.restoreIfUnchanged(selectionAtStart,kit_.selected);")
  endif()
  file(WRITE "${_qa}/vst3/${_name}.cpp" "#include \"Probe.h\"\n${_source}")
endforeach()
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/core/AtomicPatternBank.h" _bank)
qa_replace(_bank "std::array<std::uint8_t,kRows*kSteps> candidate{};"
  "drumator::qa::probe(drumator::qa::patternReadStarted);\n        std::array<std::uint8_t,kRows*kSteps> candidate{};")
qa_replace(_bank "for(std::size_t i=0;i<count;++i)\n            grid_[p*kRows*kSteps+i].store(values[i],std::memory_order_relaxed);"
  "for(std::size_t i=0;i<count;++i) {\n            grid_[p*kRows*kSteps+i].store(values[i],std::memory_order_relaxed);\n            if(i==count/2)drumator::qa::probe(drumator::qa::patternMidWrite);\n        }")
file(WRITE "${_qa}/core/AtomicPatternBank.h" "#include \"Probe.h\"\n${_bank}")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/src/core/TriggerBatch.h" _triggers)
qa_replace(_triggers "void sort() noexcept {"
  "void sort() noexcept {\n        if(drumator::qa::legacyTriggerOrdering) {\n            std::sort(events_.begin(),events_.begin()+size_,[](const Trigger& a,const Trigger& b){\n                if(a.sampleOffset!=b.sampleOffset)return a.sampleOffset<b.sampleOffset;\n                return a.row<b.row;\n            });\n            return;\n        }")
file(WRITE "${_qa}/core/TriggerBatch.h" "#include \"Probe.h\"\n${_triggers}")
add_executable(drumator_integration_tests tests/integration/WindowsIntegration.cpp
  "${_qa}/vst3/DrumatorController.cpp" "${_qa}/vst3/DrumatorView.cpp"
  "${_qa}/vst3/DrumatorProcessor.cpp" src/vst3/FlacImport.cpp)
# The shared vstgui_support library is built with SMTG_MODULE_IS_BUNDLE=1.
# Drumator receives this implementation via smtg_target_prepare_bundle, but
# this standalone EXE must also link the real SDK definition (no stub).
target_sources(drumator_integration_tests PRIVATE
  "${_DrumatorSdkRoot}/public.sdk/source/vst/vstgui_win32_bundle_support.cpp")
target_include_directories(drumator_integration_tests BEFORE PRIVATE
  "${_qa}" tests/integration src/vst3 src/core "${_drflac_dir}")
# Generated core headers retain quoted sibling includes (SequencerCore.h).
# Keep project/generated headers strict; classify only pinned upstream SDK
# paths as external. CMake/MSVC emits /external:I for these SYSTEM paths.
target_include_directories(drumator_integration_tests SYSTEM PRIVATE
  "${_DrumatorSdkRoot}" "${_DrumatorSdkRoot}/vstgui4")
target_link_libraries(drumator_integration_tests PRIVATE
  sdk vstgui_support drumator_core drumator_host_fixture)
target_compile_features(drumator_integration_tests PRIVATE cxx_std_23)
target_compile_options(drumator_integration_tests PRIVATE /W4 /WX /external:W0)
add_dependencies(drumator_test_suite drumator_integration_tests)
add_test(NAME drumator_integration_tests COMMAND drumator_integration_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/resource")
set_tests_properties(drumator_integration_tests PROPERTIES TIMEOUT 300)
