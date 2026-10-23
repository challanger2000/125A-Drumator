# Audit-only mutations in generated test copies. No product source is edited.
# Every replacement uses the existing unique-anchor, fail-closed helper.
function(reaudit_instrument name variable)
  set(_code "${${variable}}")
  if(name STREQUAL "DrumatorProcessor")
    qa_replace(_code "return attributes->setInt(\"Selected\","
      "if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::patternReplyOOM)throw std::bad_alloc{};\n     return attributes->setInt(\"Selected\",")
    qa_replace(_code "if(std::strcmp(id,\"Drumator.EditorPreviewStop\")==0){"
      "if(std::strcmp(id,\"Drumator.EditorPreviewStop\")==0){\n   if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::rejectPreviewStop)\n     return Steinberg::kResultFalse;")
    qa_replace(_code "const bool fireAudition=auditionStart_.exchange(false,std::memory_order_acq_rel);"
      "const bool fireAudition=auditionStart_.exchange(false,std::memory_order_acq_rel);\n drumator::qa::probe(drumator::qa::auditAfterAuditionStartTaken);")
    qa_replace(_code "std::int32_t cursor=0;"
      "std::int32_t cursor=0;\n   drumator::qa::probe(drumator::qa::auditAfterLengthRead);")
    qa_replace(_code "lengths[activePattern]=audioPatternLengthCache_[activePattern];"
      "if(drumator::qa::auditMutation.load()!=drumator::qa::AuditMutation::staleDuration)\n       lengths[activePattern]=audioPatternLengthCache_[activePattern];")
    qa_replace(_code "if(data.numSamples==0&&data.processContext&&wasPlaying_&&"
      "if(drumator::qa::auditMutation.load()!=drumator::qa::AuditMutation::suppressZeroStop&&\n    data.numSamples==0&&data.processContext&&wasPlaying_&&")
    qa_replace(_code "const void* bytes=nullptr;\n Steinberg::uint32 size=0;"
      "drumator::qa::auditImportTarget.store(this);\n ++drumator::qa::auditImportCalls;\n if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::skipImport)\n   return Steinberg::kResultOk;\n const void* bytes=nullptr;\n Steinberg::uint32 size=0;")
    set(_hit "triggers_.appendNoteOn(event.noteOn.pitch,event.noteOn.velocity,\n                            event.sampleOffset,\n                            static_cast<std::uint32_t>(data.numSamples));")
    qa_replace(_code "${_hit}"
      "if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::dropMidi)continue;\n     triggers_.appendNoteOn(event.noteOn.pitch,event.noteOn.velocity,\n         event.sampleOffset+(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::delayMidiOne?1:0),\n         static_cast<std::uint32_t>(data.numSamples));\n     if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::doubleMidi)\n       ${_hit}")
    qa_replace(_code "if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;"
      "if(rendered.status!=RenderStatus::Ok)return Steinberg::kResultFalse;\n if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::zeroPrefix||\n    (drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::muteImportedProcessor&&\n     drumator::qa::auditImportTarget.load()==this)) {\n   const auto auditMutationFrames=drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::zeroPrefix?\n       std::min(Steinberg::int32{32},data.numSamples):data.numSamples;\n   if(left)std::fill_n(left,auditMutationFrames,0.f);if(right)std::fill_n(right,auditMutationFrames,0.f);\n }\n if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::delayImportedProcessor&&\n    drumator::qa::auditImportTarget.load()==this&&drumator::qa::auditDelayCalls.fetch_add(1)<8u)\n   std::this_thread::sleep_for(std::chrono::milliseconds(10));")
    set(_code "#include <chrono>\n${_code}")
  elseif(name STREQUAL "DrumatorController")
    qa_replace(_code "msg->setMessageID(\"Drumator.QueryPattern\");"
      "if(drumator::qa::auditMutation.load()==drumator::qa::AuditMutation::patternQueryOOM)throw std::bad_alloc{};\n   msg->setMessageID(\"Drumator.QueryPattern\");")
    qa_replace(_code "if(strokePattern_<kPatterns&&strokeOwner_!=owner)"
      "if(drumator::qa::auditMutation.load()!=drumator::qa::AuditMutation::skipPeerCommit&&\n    strokePattern_<kPatterns&&strokeOwner_!=owner)")
    qa_replace(_code "if(!sameName||(!pathsRestoredBeforeComponent_&&"
      "if(!sameName||(drumator::qa::auditMutation.load()!=drumator::qa::AuditMutation::basenameOnly&&\n                     !pathsRestoredBeforeComponent_&&")
  endif()
  set(${variable} "${_code}" PARENT_SCOPE)
endfunction()
