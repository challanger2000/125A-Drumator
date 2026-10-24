// Independent 2026-10-23 source-audit regressions. NOT Windows-run in this audit.
// Uses the actual Controller/View/Processor; no replacement state algorithms.
Fixture& fa23Kit(Fixture& f){
 KitState kit;kit.grid.fill(0);kit.visible=4;kit.names[0]="source.wav";
 kit.wav[0]=constantStereo(.25f,-.5f);f.kit(kit);prepareAudio(*f.p);return f;
}
DrumatorView& fa23Open(Fixture& f){
 f.open();auto* view=IntegrationAccess::view(*f.c,0);
 // Direct row EDIT action opens the WAV modal without opening the sample popup.
 click(*view,240.,128.);
 check(IntegrationAccess::sampleEditorOpen(*view),"FA23 actual WAV editor did not open");
 return *view;
}
void testFa23ClearEditedState(){
 Fixture f;fa23Kit(f);
 check(f.c->commitSampleEdit(0,{0,1024,0,0,.5f}),"FA23 clear fixture commit");
 check(auditReturnedKit(*f.p).sampleEdits[0].gain==.5f,"FA23 edited control not saved");
 check(f.c->clearSample(0),"FA23 actual ClearSample rejected");
 ResizableMemoryIBStream stream;
 check(f.p->getState(&stream)==kResultOk,"FA23 BLOCKER ClearSample retained edit and made project unsavable");
 KitState restored;
 check(KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
       static_cast<std::size_t>(stream.getCursor()),restored)&&restored.wav[0].empty()&&
       restored.sampleEdits[0].end==0,"FA23 cleared DM16 row not canonical/recallable");
}
void testFa23PreviewRetrigger(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;const auto edit=SampleEdit::reset(1024);
 check(f.c->previewSampleEdit(0,edit)&&f.p->process(audio.data)==kResultOk,"FA23 first preview");
 check(audio.left[80]==.25f&&audio.right[80]==-.5f,"FA23 preview control PCM");
 check(f.c->previewSampleEdit(0,edit)&&f.p->process(audio.data)==kResultOk,"FA23 second preview");
 check(audio.left[80]==.25f&&audio.right[80]==-.5f,
       "FA23 VORHOEREN layered previous voice instead of restarting one-shot");
}
void testFa23PreviewPublication(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;arm(qa::auditAfterAuditionAdopt);
 tresult result=kResultFalse;
 std::jthread actor([&]{result=f.p->process(audio.data);});waitEntered();
 const bool accepted=f.c->previewSampleEdit(0,SampleEdit::reset(1024));
 releaseProbe();actor.join();
 check(accepted&&result==kResultOk&&!probeTimeout,"FA23 actual preview interleave not reached");
 const bool first=audio.left[80]==.25f&&audio.right[80]==-.5f;
 check(f.p->process(audio.data)==kResultOk,"FA23 next preview block");
 const bool second=audio.left[80]==.25f&&audio.right[80]==-.5f;
 // A request after the block boundary may defer one block; it must not vanish.
 check(first||second,"FA23 accepted preview consumed Start before adopting its prepared asset");
}
void testFa23ModalDrop(){
 // An empty row must not enable direct EDIT or open a modal.
 { Fixture emptyFixture;KitState emptyKit;emptyKit.grid.fill(0);emptyKit.visible=4;
   emptyFixture.kit(emptyKit);emptyFixture.open();
   auto* emptyView=IntegrationAccess::view(*emptyFixture.c,0);
   check(!emptyFixture.c->hasSample(0),"UI empty row falsely enables EDIT");
   click(*emptyView,240.,128.);
   check(!IntegrationAccess::sampleEditorOpen(*emptyView),"UI EDIT opened an empty row");
 }
 FinalAuditFiles files;
 // Positive control: the same row coordinate must accept a drop without a modal.
 // Without this assertion a broken modal guard can leave the negative test green.
 { Fixture positive;fa23Kit(positive);positive.open();
   auto* target=IntegrationAccess::view(*positive.c,0);
   const auto replacement=constantStereo(-.125f,.375f);
   AuditFilePackage source(files.write("positive-drop.wav",replacement));
   VSTGUI::DragEventData drop{};drop.drag=&source;drop.pos={150.,128.};
   const auto allowed=target->onDragEnter(drop);
   const bool changed=target->onDrop(drop);
   check(allowed!=VSTGUI::DragOperation::None&&changed&&
         auditReturnedKit(*positive.p).wav[0]==replacement,
         "FA23 positive control: valid sample target did not accept unblocked drop");
 }
 Fixture f;fa23Kit(f);auto& view=fa23Open(f);
 const auto before=auditReturnedKit(*f.p).wav[0];
 AuditFilePackage package(files.write("replacement.wav",constantStereo(-.125f,.375f)));
 VSTGUI::DragEventData drag{};drag.drag=&package;drag.pos={150.,128.};
 const auto operation=view.onDragEnter(drag);const bool accepted=view.onDrop(drag);
 check(operation==VSTGUI::DragOperation::None&&!accepted&&auditReturnedKit(*f.p).wav[0]==before,
       "FA23 modal WAV editor admitted a file drop into the covered sample row");
}
void testFa23StaleEditor(){
 FinalAuditFiles files;Fixture f;fa23Kit(f);auto& view=fa23Open(f);
 click(view,650.625,420.);VSTGUI::CPoint point(650.625,420.);view.onMouseUp(point,{});
 check(IntegrationAccess::sampleEditorDraft(view).gain==.5f,"FA23 actual gain control fixture");
 const auto replacement=constantStereo(-.125f,.375f);
 check(f.c->loadWav(0,files.write("new-source.wav",replacement))==Controller::LoadStatus::Accepted,
       "FA23 peer replacement rejected");
 check(auditReturnedKit(*f.p).sampleEdits[0].end==0,"FA23 replacement did not reset edit");
 click(view,990.,470.); // New editor APPLY after peer source change.
 const auto saved=auditReturnedKit(*f.p);
 check(saved.wav[0]==replacement&&saved.sampleEdits[0].end==0,
       "FA23 stale draft was silently committed to a different source asset");
}
void testFa23OriginCycles(){
 FinalAuditFiles files;Fixture f;fa23Kit(f);
 const auto path=files.write("kick.wav",constantStereo(.25f,-.5f));
 check(f.c->loadWav(0,path)==Controller::LoadStatus::Accepted,"FA23 X import");
 const auto x=Fixture::encode(auditReturnedKit(*f.p));const auto control=finalAuditControllerBytes(*f.c);
 KitState y;y.grid.fill(0);y.names[0]="kick.wav";y.wav[0]=constantStereo(-.125f,.375f);
 const auto encodedY=Fixture::encode(y);
 qa::ByteStream py(encodedY.data(),encodedY.size());
 check(f.p->setState(&py)==kResultOk&&f.recall(encodedY)==kResultOk,"FA23 unpaired Y Component recall");
 check(finalAuditControllerRecall(*f.c,control)==kResultOk,"FA23 next complete X Controller-first");
 qa::ByteStream px(x.data(),x.size());
 check(f.p->setState(&px)==kResultOk&&f.recall(x)==kResultOk,"FA23 next complete X Component");
 check(IntegrationAccess::paths(*f.c)[0]==path,
       "FA23 old unpaired Component marker discarded explicit origin in next complete Controller-first recall");
}
void testFa23UndoDuringStroke(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.grid[0]=60;
 kit.patternLengths[1]=16;kit.grid[kRows*kSteps]=90;f.kit(kit);
 check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&f.c->copyCurrentPattern()&&
       f.c->setParamNormalized(kPatternParam,0.)==kResultOk&&f.c->pasteCurrentPattern(),"FA23 32-to16 paste");
 check(f.c->patternLength(0)==16&&f.c->canUndoCurrentPattern(),"FA23 duration Undo setup");
 int owner=0;f.c->beginVelocityStroke(0,&owner);
 check(f.c->setStepVelocity(0,0,0,80,&owner),"FA23 live gesture");
 check(f.c->undoCurrentPattern(),"FA23 Undo rejected");
 check(f.c->patternLength(0)==16&&f.c->velocity(0,0,0)==90&&
       auditReturnedKit(*f.p).patternLengths[0]==16,
       "FA23 Undo combines current gesture snapshot with old clipboard duration");
}
void testFa23TrimInterpolation(){
 for(std::size_t impulse:{63u,128u}){
   std::array<float,256> pcm{};pcm[impulse]=1.f;
   SincKernel kernel;check(kernel.prepare(2.),"FA23 trim SRC kernel");
   SampleVoiceEngine engine;check(engine.setOutputRate(48000.),"FA23 trim SRC rate");
   RowSound row;row.sample={pcm.data(),nullptr,pcm.size()};row.kernel=&kernel;
   row.edit={64,128,0,0,1.f};
   check(engine.setRow(0,row),"FA23 trim silent region");
   std::array<float,32> left{},right{};const Trigger hit{0,0,0,127,0};
   check(engine.render(&hit,1,left.data(),right.data(),32).status==RenderStatus::Ok,"FA23 trim render");
   for(std::size_t sample=0;sample<left.size();++sample)
     check(std::abs(left[sample])<1e-7f&&std::abs(right[sample])<1e-7f,
           "FA23 resampler reads an impulse outside the selected silent START/END interval");
 }
}
void testFa23AuditionDeactivate(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&f.p->process(audio.data)==kResultOk,
       "FA23 initial audition");
 check(audio.left[80]==.25f,"FA23 initial audition control");
 check(f.p->setProcessing(false)==kResultOk&&f.p->setActive(false)==kResultOk&&
       f.p->setActive(true)==kResultOk&&f.p->setProcessing(true)==kResultOk,"FA23 host lifecycle");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f&&audio.right[80]==0.f,
       "FA23 old audition resumed after deactivate/reactivate without new request");
}
void testFa23QueryOOM(){
 Fixture f;fa23Kit(f);HostMessage query;query.setMessageID("Drumator.SampleEditorQuery");
 check(query.getAttributes()->setInt("Row",0)==kResultOk,"FA23 query fixture");
 allocationFail=0;bool escaped=false;tresult result=kResultFalse;
 try {result=f.p->notify(&query);}catch(const std::bad_alloc&){escaped=true;}
 allocationFail=-1;
 check(!escaped&&result==kOutOfMemory,
       "FA23 Wave attribute copy OOM escaped Processor::notify VST3 ABI");
 check(!auditReturnedKit(*f.p).wav[0].empty(),"FA23 query OOM damaged source");
}

void testFr01StopAfterStartConsumed(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)),"FR01 preview setup rejected");
 arm(qa::auditAfterAuditionStartTaken);
 tresult result=kResultFalse;
 std::jthread actor([&]{result=f.p->process(audio.data);});
 waitEntered();
 f.c->stopSampleEditPreview();
 releaseProbe();actor.join();
 check(result==kResultOk&&!probeTimeout,"FR01 stop race did not complete");
 check(audio.left[80]==0.f&&audio.right[80]==0.f,
       "FR01 STOP was consumed but stale start still fired");
 check(f.p->process(audio.data)==kResultOk&&
       audio.left[80]==0.f&&audio.right[80]==0.f,
       "FR01 STOP left audition voice alive on following block");
}
void testRv01RestartAfterStopDuringAudioStart(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 const auto edit=SampleEdit::reset(1024);
 check(f.c->previewSampleEdit(0,edit),"RV01 initial preview rejected");
 arm(qa::auditAfterAuditionStartTaken);
 tresult result=kResultFalse;
 std::jthread actor([&]{result=f.p->process(audio.data);});
 waitEntered();
 check(f.c->stopSampleEditPreview(),"RV01 STOP not delivered");
 auto editB=edit;
 editB.gain=.5f; // distinguish the new request from the superseded source
 check(f.c->previewSampleEdit(0,editB),"RV01 newer preview rejected");
 releaseProbe();actor.join();
 check(result==kResultOk&&!probeTimeout,"RV01 competing preview interleave did not finish");
 const bool bNow=audio.left[80]==.125f&&audio.right[80]==-.25f;
 check(f.p->process(audio.data)==kResultOk,"RV01 followup process rejected");
 const bool bNext=audio.left[80]==.125f&&audio.right[80]==-.25f;
 check(bNow||bNext,
       "RV01 superseded audio A was played or newer source B was lost");
}
void testRv01AssetCommandPublication(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 const auto editA=SampleEdit::reset(1024);
 auto editB=editA;editB.gain=.5f;
 check(f.c->previewSampleEdit(0,editA),"RV01 asset pairing A rejected");
 arm(qa::auditAfterAuditionAssetPublished);
 bool preparedB=false;
 std::jthread publisher([&]{preparedB=f.c->previewSampleEdit(0,editB);});
 waitEntered(); // B published but not yet announced by START command
 check(f.p->process(audio.data)==kResultOk&&
       audio.left[80]==0.f&&audio.right[80]==0.f,
       "RV01 older START fired against newly published B asset");
 releaseProbe();publisher.join();
 check(preparedB&&!probeTimeout,"RV01 publisher did not finish");
 check(f.p->process(audio.data)==kResultOk&&
       audio.left[80]==.125f&&audio.right[80]==-.25f,
       "RV01 B preview did not fire with its matching source");
}
void testRv01OutputlessPreviewRetirement(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)),
       "RV01 outputless initial preview rejected");
 audio.data.numOutputs=0;audio.data.outputs=nullptr;
 check(f.p->process(audio.data)==kResultOk,
       "RV01 outputless callback rejected");
 audio.data.numOutputs=1;audio.data.outputs=&audio.output;
 check(f.p->process(audio.data)==kResultOk&&
       audio.left[80]==0.f&&audio.right[80]==0.f,
       "RV01 preview restarted after outputless interval");
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&
       f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "RV01 fresh preview failed after outputless interval");
}
void testA10MissingContextAudioContinuity(){
 // True VST3 integration: compare all 256 PCM samples over a missing
 // ProcessContext block against the same uninterrupted real host clock.
 constexpr auto playing=ProcessContext::kPlaying|
      ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 for(const bool newHit:{false,true})for(const bool partial:{false,true}){
   Fixture reference,interrupted;
   KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.names[0]="host-gap.wav";
   kit.wav[0]=constantStereo(.25f,-.5f);
   kit.grid[newHit?1:0]=127;
   reference.kit(kit);interrupted.kit(kit);
   prepareAudio(*reference.p);prepareAudio(*interrupted.p);
   AuditAudioBlock standard,missing;
   const double initialPpq=newHit?.25-256.0/24000.0:0.0;
   for(auto* b:{&standard,&missing}){
     b->clock.state=playing;
     b->clock.tempo=120.;
     b->clock.projectTimeMusic=initialPpq;
   }
   check(reference.p->process(standard.data)==kResultOk&&
         interrupted.p->process(missing.data)==kResultOk,
         "A10 initial host clock rejected");
   for(std::size_t i=0;i<256;++i)
     check(standard.left[i]==missing.left[i]&&
           standard.right[i]==missing.right[i],
           "A10 pre-gap PCM mismatch");
   for(int block=1;block<=3;++block){
     standard.clock.state=playing;missing.clock.state=playing;
     const double ppq=initialPpq+block*256.0/24000.0;
     standard.clock.projectTimeMusic=ppq;
     missing.clock.projectTimeMusic=ppq;
     missing.data.processContext=&missing.clock;
     if(block==1) {
       if(partial)missing.clock.state=ProcessContext::kPlaying;
       else missing.data.processContext=nullptr;
     }
     check(reference.p->process(standard.data)==kResultOk&&
           interrupted.p->process(missing.data)==kResultOk,
           "A10 live host clock lost during context gap");
     for(std::size_t i=0;i<256;++i)
       check(standard.left[i]==missing.left[i]&&
             standard.right[i]==missing.right[i],
             "A10 optional context omission cut, shifted or duplicated PCM");
     if(block==1)
       check(standard.left[80]==.25f&&standard.right[80]==-.5f,
             "A10 silent positive control for missing clock");
   }
   // Genuine host STOP must still silence outstanding sequencer voices.
   missing.clock.state=0;missing.data.processContext=&missing.clock;
   check(interrupted.p->process(missing.data)==kResultOk&&
         missing.left[80]==0.f&&missing.right[80]==0.f,
         "A10 explicit STOP ignored after fallback");
 }
}

void testA11CycleWrapPreservesSampleTails(){
 // 2048-frame stereo 48k FLOAT32 WAV outlasts this deliberately short loop.
 auto wav=constantStereo(.125f,-.25f);
 const auto original=wav;
 wav.insert(wav.end(),original.begin()+44,original.end());
 auto put32=[&](std::size_t at,std::uint32_t n){
   for(unsigned i=0;i<4;++i)wav[at+i]=static_cast<std::uint8_t>(n>>(i*8));
 };
 put32(4,static_cast<std::uint32_t>(wav.size()-8));
 put32(40,static_cast<std::uint32_t>(wav.size()-44));
 constexpr auto playing=ProcessContext::kPlaying|
     ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 constexpr auto cycle=ProcessContext::kCycleActive|ProcessContext::kCycleValid;
 for(unsigned mode=0;mode<3;++mode){
   // 0: unflagged Locate, 1: confirmed loop, 2: wrong Cycle end.
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.grid[0]=127;kit.names[0]="cycle-tail.wav";kit.wav[0]=wav;
   f.kit(kit);prepareAudio(*f.p);
   AuditAudioBlock audio;
   audio.clock.tempo=120.;
   audio.clock.cycleStartMusic=0.;
   audio.clock.cycleEndMusic=static_cast<double>(mode==2?7:5)*256./24000.;
   audio.clock.state=playing|(mode==0?0:cycle);
   for(unsigned block=0;block<5;++block){
     audio.clock.projectTimeMusic=static_cast<double>(block)*256./24000.;
     check(f.p->process(audio.data)==kResultOk,"A11 pre-loop render");
     check(audio.left[80]==.125f&&audio.right[80]==-.25f,
           "A11 silent/doubled pre-loop positive control");
   }
   check(IntegrationAccess::activeAudioVoices(*f.p)==1,
         "A11 expected tail missing before loop");
   audio.clock.projectTimeMusic=0.;
   check(f.p->process(audio.data)==kResultOk,"A11 jump processing");
   // The new beat overlays its original one-shot tail only for real Cycle.
   const bool confirmed=mode==1;
   check(audio.left[80]==(confirmed?.25f:.125f)&&
         audio.right[80]==(confirmed?-.5f:-.25f),
         "A11 valid Cycle cut tail or Locate retained stale tail");
   check(IntegrationAccess::activeAudioVoices(*f.p)==(confirmed?2u:1u),
         "A11 active voices inconsistent with measured PCM");
 }
}

void testA12CycleWrapMidBlockAndNegativeControls(){
 // A 2048-frame WAV rings over a 1152-frame loop ending HALF-WAY
 // through a 256-frame callback. The next real host PPQ must include
 // the 128-frame overshoot rather than equal the raw cycle start.
 auto wav=constantStereo(.125f,-.25f);
 const auto original=wav;
 wav.insert(wav.end(),original.begin()+44,original.end());
 auto put32=[&](std::size_t at,std::uint32_t value){
   for(unsigned i=0;i<4;++i)
     wav[at+i]=static_cast<std::uint8_t>(value>>(i*8));
 };
 put32(4,static_cast<std::uint32_t>(wav.size()-8));
 put32(40,static_cast<std::uint32_t>(wav.size()-44));
 constexpr auto playing=ProcessContext::kPlaying|
     ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 constexpr auto cycle=ProcessContext::kCycleActive|ProcessContext::kCycleValid;
 constexpr double cycleStart=4.0; // bar two / nonzero project position
 const double trueCycleEnd=cycleStart+1152.0/24000.0;
 for(unsigned mode=0;mode<4;++mode){
   // 0: valid cycle with overshoot, 1: cycle flag without valid bounds,
   // 2: incorrect cycle end, 3: unflagged locate.
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.grid[0]=127;kit.names[0]="cycle-offset.wav";kit.wav[0]=wav;
   f.kit(kit);prepareAudio(*f.p);
   AuditAudioBlock audio;
   audio.clock.tempo=120.;
   audio.clock.cycleStartMusic=cycleStart;
   audio.clock.cycleEndMusic=mode==2?
       std::numeric_limits<double>::quiet_NaN():trueCycleEnd;
   audio.clock.state=playing|(mode==0||mode==2?cycle:
                                (mode==1?ProcessContext::kCycleActive:0));
   for(unsigned block=0;block<5;++block){
     audio.clock.projectTimeMusic=cycleStart+block*256.0/24000.0;
     check(f.p->process(audio.data)==kResultOk,"A12 before loop PCM");
     for(unsigned frame=0;frame<256;++frame){
       const bool wrapped=mode==0&&block==4&&frame>=128;
       check(audio.left[frame]==(wrapped?.25f:.125f)&&
             audio.right[frame]==(wrapped?-.5f:-.25f),
             "A12 old tail or in-block sequencer restart invalid");
     }
   }
   check(IntegrationAccess::activeAudioVoices(*f.p)==(mode==0?2u:1u),
         "A12 actual voices inconsistent with in-block PCM");
   // The loop began in the previous block, so no grid hit starts at the
   // next block's slightly offset PPQ. Trigger the new beat via real MIDI
   // to prove that the old tail truly survives beside the new one.
   EventList notes;Event hit{};
   hit.type=Event::kNoteOnEvent;hit.sampleOffset=0;hit.noteOn.pitch=36;
   hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
   check(notes.addEvent(hit)==kResultOk,"A12 MIDI hit fixture");
   audio.data.inputEvents=&notes;
   audio.clock.projectTimeMusic=cycleStart+128.0/24000.0;
   check(f.p->process(audio.data)==kResultOk,"A12 wrapped block");
   const bool confirmed=mode==0;
   for(unsigned frame=0;frame<256;++frame)
     check(audio.left[frame]==(confirmed?.375f:.125f)&&
           audio.right[frame]==(confirmed?-.75f:-.25f),
           "A12 in-block sequencer, old tail or MIDI retrigger vanished");
   check(IntegrationAccess::activeAudioVoices(*f.p)==(confirmed?3u:1u),
         "A12 PCM and active voices disagree");
   audio.data.inputEvents=nullptr;
   audio.clock.state=cycle; // no kPlaying = explicit STOP, even in Cycle
   check(f.p->process(audio.data)==kResultOk,"A12 explicit STOP");
   for(unsigned frame=0;frame<256;++frame)
     check(audio.left[frame]==0.f&&audio.right[frame]==0.f,
           "A12 actual STOP left a cycle tail audible");
   check(IntegrationAccess::activeAudioVoices(*f.p)==0,
         "A12 explicit STOP retained voice state");
 }
}


void testA13PureSequencerCyclePcm(){
 // No external MIDI. Grid Step 0 must fire again at sample 128 inside
 // block five, or the full stereo PCM oracle must fail.
 auto wav=constantStereo(.125f,-.25f);
 const auto first=wav;
 wav.insert(wav.end(),first.begin()+44,first.end());
 const auto set32=[&](std::size_t at,std::uint32_t value){
   for(unsigned i=0;i<4;++i)wav[at+i]=static_cast<std::uint8_t>(value>>(8*i));
 };
 set32(4,static_cast<std::uint32_t>(wav.size()-8));
 set32(40,static_cast<std::uint32_t>(wav.size()-44));
 constexpr auto playing=ProcessContext::kPlaying|
     ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 constexpr auto cycle=ProcessContext::kCycleActive|ProcessContext::kCycleValid;
 for(unsigned mode=0;mode<3;++mode){
   // Valid Cycle, absent kCycleValid, and bogus NaN end.
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.grid[0]=127;kit.names[0]="sequencer-seam.wav";kit.wav[0]=wav;
   f.kit(kit);prepareAudio(*f.p);AuditAudioBlock audio;
   audio.clock.tempo=120.;
   audio.clock.state=playing|(mode==1?ProcessContext::kCycleActive:cycle);
   audio.clock.cycleStartMusic=4.;
   audio.clock.cycleEndMusic=mode==2?
       std::numeric_limits<double>::quiet_NaN():4.+1152./24000.;
   for(unsigned b=0;b<5;++b){
     audio.clock.projectTimeMusic=4.+static_cast<double>(b)*256./24000.;
     check(f.p->process(audio.data)==kResultOk,"A13 native cycle block");
     for(unsigned i=0;i<256;++i){
       const bool newHit=mode==0&&b==4&&i>=128;
       check(audio.left[i]==(newHit?.25f:.125f)&&
             audio.right[i]==(newHit?-.5f:-.25f),
             "A13 host cycle seam skipped, anticipated or doubled programmed hit");
     }
   }
   check(IntegrationAccess::activeAudioVoices(*f.p)==(mode==0?2u:1u),
         "A13 expected real overlapping PCM sample voices");
   audio.clock.projectTimeMusic=4.+128./24000.;
   check(f.p->process(audio.data)==kResultOk,"A13 next host block");
   for(unsigned i=0;i<256;++i)
     check(audio.left[i]==(mode==0?.25f:0.f)&&
           audio.right[i]==(mode==0?-.5f:0.f),
           "A13 false locate or lost old tails after mid-block cycle");
 }
}


std::vector<std::uint8_t> a14LongConstant(float left,float right){
 auto wav=constantStereo(left,right);
 const auto original=wav;
 wav.insert(wav.end(),original.begin()+44,original.end());
 for(unsigned i=0;i<4;++i){
   wav[4+i]=static_cast<std::uint8_t>((wav.size()-8)>>(i*8));
   wav[40+i]=static_cast<std::uint8_t>((wav.size()-44)>>(i*8));
 }
 return wav;
}
void testA14PatternAndSongAcrossCycleSeam(){
 constexpr auto playing=ProcessContext::kPlaying|
     ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 constexpr auto cycle=ProcessContext::kCycleActive|ProcessContext::kCycleValid;
 for(unsigned mode=0;mode<3;++mode){
   // Pattern B switches 64 frames before / precisely at the seam;
   // Song mode selects B independently of the host pattern parameter.
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=2;
   kit.names[0]="pattern-a.wav";kit.wav[0]=a14LongConstant(.125f,-.25f);
   kit.names[1]="pattern-b.wav";kit.wav[1]=a14LongConstant(.375f,-.125f);
   kit.output[1]=0;
   kit.grid[0]=127;
   kit.grid[(kRows+1)*kSteps]=127;
   if(mode==2){
     kit.songEnabled=1;
     check(kit.chain.setCount(1)&&kit.chain.configure(0,1,16),
           "A14 song fixture invalid");
   }
   f.kit(kit);prepareAudio(*f.p);AuditAudioBlock audio;
   audio.clock.state=playing|cycle;audio.clock.tempo=120.;
   audio.clock.cycleStartMusic=4.;
   audio.clock.cycleEndMusic=4.+1152./24000.;
   EventList patternEvents;
   if(mode!=2){
     Event change{};change.type=Event::kNoteOnEvent;
     change.sampleOffset=mode==0?64:128;
     change.noteOn.pitch=25;change.noteOn.velocity=1.f;
     change.noteOn.noteId=-1;
     check(patternEvents.addEvent(change)==kResultOk,
           "A14 real MIDI A-H pattern change setup");
   }
   for(unsigned b=0;b<5;++b){
     audio.clock.projectTimeMusic=4.+static_cast<double>(b)*256./24000.;
     audio.data.inputEvents=b==4&&mode!=2?&patternEvents:nullptr;
     check(f.p->process(audio.data)==kResultOk,
           "A14 real pattern/song seam processing");
     for(unsigned i=0;i<256;++i){
       const bool afterSeam=b==4&&i>=128;
       const float expectedLeft=mode==2?
           (afterSeam?.75f:.375f):(afterSeam?.5f:.125f);
       const float expectedRight=mode==2?
           (afterSeam?-.25f:-.125f):(afterSeam?-.375f:-.25f);
       check(audio.left[i]==expectedLeft&&audio.right[i]==expectedRight,
             "A14 pattern/Song seam selected wrong program or onset");
     }
   }
   check(IntegrationAccess::activeAudioVoices(*f.p)==2,
         "A14 pattern or song seam missed overlapping sample voice");
 }
}
void testA15CycleRandomVelocityReproducible(){
 // Same musical Step 0 on the first and second loop should have the
 // SAME velocity even if a 6000-frame cycle wraps at sample 112 in
 // callback 24. Without wrapped PPQ, randomVelocity hashes Step 17
 // instead of repeated Step 16 (the test's deliberately distinct values).
 Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
 kit.names[0]="random-velocity.wav";kit.wav[0]=constantStereo(.125f,-.25f);
 kit.grid[0]=64;kit.randomVelocity[0]=100;
 f.kit(kit);prepareAudio(*f.p);AuditAudioBlock audio;
 audio.clock.tempo=120.;
 audio.clock.state=ProcessContext::kPlaying|
     ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid|
     ProcessContext::kCycleActive|ProcessContext::kCycleValid;
 audio.clock.cycleStartMusic=4.;audio.clock.cycleEndMusic=4.25;
 std::array<float,1024> firstL{},firstR{},repeatL{},repeatR{};
 for(unsigned b=0;b<28;++b){
   const unsigned start=b*256;
   audio.clock.projectTimeMusic=4.+static_cast<double>(start%6000)/24000.;
   check(f.p->process(audio.data)==kResultOk,"A15 render wrapped velocity");
   for(unsigned i=0;i<256;++i){
     const unsigned at=start+i;
     if(at<1024){firstL[at]=audio.left[i];firstR[at]=audio.right[i];}
     if(at>=6000&&at<7024){
       repeatL[at-6000]=audio.left[i];
       repeatR[at-6000]=audio.right[i];
     }
   }
 }
 check(firstL[40]>0.f&&firstR[40]<0.f,
       "A15 positive random-velocity audio reference silent");
 for(unsigned i=0;i<1024;++i)
   check(firstL[i]==repeatL[i]&&firstR[i]==repeatR[i],
         "A15 Cycle step velocity depended on callback partition instead of PPQ");
}
void testA16MultiCyclePerCallback(){
 // Four musical loops inside a 256-frame callback and four more in
 // the following callback. Old voices must not be retired on callback 2.
 for(unsigned mode=0;mode<2;++mode){
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.names[0]="multi-cycle.wav";kit.wav[0]=a14LongConstant(.125f,-.25f);
   kit.grid[0]=127;f.kit(kit);prepareAudio(*f.p);AuditAudioBlock audio;
   audio.clock.tempo=120.;
   audio.clock.state=ProcessContext::kPlaying|
       ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid|
       ProcessContext::kCycleActive|ProcessContext::kCycleValid;
   audio.clock.cycleStartMusic=4.;
   audio.clock.cycleEndMusic=mode==0?4.+64./24000.:
       std::numeric_limits<double>::quiet_NaN();
   for(unsigned b=0;b<2;++b){
     audio.clock.projectTimeMusic=4.;
     check(f.p->process(audio.data)==kResultOk,"A16 short cycle block");
     for(unsigned i=0;i<256;++i){
       const unsigned voices=mode==0?b*4+i/64+1:1;
       const float expectedLeft=static_cast<float>(voices)*.125f;
       const float expectedRight=-static_cast<float>(voices)*.25f;
       check(audio.left[i]==expectedLeft&&audio.right[i]==expectedRight,
             "A16 short multi-wrap voice reset, missed/doubled Step or false Cycle");
     }
   }
   check(IntegrationAccess::activeAudioVoices(*f.p)==(mode==0?8u:1u),
         "A16 multi-wrap post-host live voice count mismatch");
 }
}

void testA09WaveformExtremaAgainstNativeReference(){
 constexpr std::size_t count=131089u;
 auto wav=constantStereo(0.f,0.f);
 wav.resize(44u+count*8u);
 const auto put32=[&](std::size_t offset,std::uint32_t value){
   for(unsigned j=0;j<4u;++j)
     wav[offset+j]=static_cast<std::uint8_t>(value>>(j*8u));
 };
 put32(4u,static_cast<std::uint32_t>(wav.size()-8u));
 put32(40u,static_cast<std::uint32_t>(count*8u));
 std::vector<float> left(count),right(count);
 for(std::size_t i=0;i<count;++i) {
   left[i]=static_cast<float>(std::sin(static_cast<double>(i)*.01417)*.6);
   right[i]=static_cast<float>(std::cos(static_cast<double>(i)*.01231)*.4);
   if(i%256u==255u){left[i]=-.99f;right[i]=.95f;}
   if(i%1024u==0u){left[i]=.87f;right[i]=-.92f;}
   std::memcpy(wav.data()+44u+i*8u,&left[i],sizeof(float));
   std::memcpy(wav.data()+48u+i*8u,&right[i],sizeof(float));
 }
 Fixture f;KitState state;state.grid.fill(0);state.visible=4;
 state.names[0]="variable.wav";state.wav[0]=wav;
 f.kit(state);prepareAudio(*f.p);
 qa::auditWaveIndexedCalls=0;
 // Open the real native WAV editor, then exercise the very same
 // sampleEditorSnapshot path used by zoom, scroll and mouse drags.
 auto& view=fa23Open(f);
 check(IntegrationAccess::zoomSample(view,true)&&
       IntegrationAccess::scrollSample(view,825.),
       "A09 real View viewport controls failed");
 std::array<WaveformRange,256> bins{},other{};
 SampleEdit edit{};std::uint32_t frames=0,revision=0;bool stereo=false;
 const std::array<std::pair<std::size_t,std::size_t>,8> windows{{
     {0u,count},{1u,260u},{250u,260u},{255u,513u},
     {256u,129999u},{17u,2000u},{65535u,65600u},{count-1u,count}}};
 for(const auto bounds:windows) {
   check(f.c->sampleEditorSnapshot(0,edit,frames,bins,
           static_cast<std::uint32_t>(bounds.first),
           static_cast<std::uint32_t>(bounds.second),
           &revision,&other,&stereo)&&frames==count&&stereo,
         "A09 authentic Controller->Processor stereo waveform query rejected");
   for(std::size_t i=0;i<256u;++i) {
     const auto start=bounds.first+((bounds.second-bounds.first)*i)/256u;
     const auto rawEnd=bounds.first+((bounds.second-bounds.first)*(i+1u))/256u;
     const auto end=std::max(start+1u,rawEnd);
     const auto expectedLeft=waveformRange(left.data(),count,start,end);
     const auto expectedRight=waveformRange(right.data(),count,start,end);
     check(std::memcmp(&expectedLeft.minimum,&bins[i].minimum,sizeof(float))==0&&
           std::memcmp(&expectedLeft.maximum,&bins[i].maximum,sizeof(float))==0&&
           std::memcmp(&expectedRight.minimum,&other[i].minimum,sizeof(float))==0&&
           std::memcmp(&expectedRight.maximum,&other[i].maximum,sizeof(float))==0,
           "A09 displayed stereo bin does not match independent PCM oracle");
   }
 }
 check(qa::auditWaveIndexedCalls.load()>=8u*256u,
       "A09 real UI queries used old linear fallback instead of the index");
 check(auditReturnedKit(*f.p).wav[0]==wav,
       "A09 purely graphical operation changed embedded sample bytes");
}
void testA08WaveEditorRevisionCache(){
 FinalAuditFiles files;Fixture f;fa23Kit(f);
 qa::auditWaveDecodeMisses=0;qa::auditWaveFullReplies=0;
 auto& view=fa23Open(f); // Actual View OPEN invokes the public snapshot.
 check(qa::auditWaveDecodeMisses==1&&qa::auditWaveFullReplies==1,
       "A08 opening waveform did not exercise one real full WAV query");
 for(int i=0;i<4;++i) {
   check(IntegrationAccess::zoomSample(view,true),"A08 native View zoom rejected");
   check(IntegrationAccess::scrollSample(view,870.),"A08 native View scroll rejected");
 }
 check(IntegrationAccess::zoomSample(view,false,true),"A08 waveform FIT failed");
 check(qa::auditWaveDecodeMisses==1&&qa::auditWaveFullReplies==1,
       "A08 unchanged View zoom/scroll redundantly decodes or transmits full WAV");
 SampleEdit edit{};std::uint32_t frames=0,revision=0;
 std::array<WaveformRange,256> envelope{},right{};
 bool stereo=false;
 check(f.c->sampleEditorSnapshot(0,edit,frames,envelope,0,0,&revision,&right,&stereo)&&
       frames==1024&&stereo&&edit.gain==1.f,
       "A08 metadata-only snapshot corrupted sample or channel count");
 // Compare actual wave-envelope extrema against independently known
 // constant PCM. These assertions fail if cached waveform is stale/silent.
 for(std::size_t i=0;i<256;++i)
   check(envelope[i].minimum==.25f&&envelope[i].maximum==.25f&&
         right[i].minimum==-.5f&&right[i].maximum==-.5f,
         "A08 cached waveform PCM does not match original stereo source");
 check(f.c->commitSampleEdit(0,{0,1024,8,12,.75f},revision),
       "A08 edit commit refused");
 check(f.c->sampleEditorSnapshot(0,edit,frames,envelope,0,0,&revision,&right,&stereo)&&
       edit.gain==.75f&&edit.fadeIn==8&&edit.fadeOut==12&&
       qa::auditWaveDecodeMisses==2&&qa::auditWaveFullReplies==2,
       "A08 edited-source revision failed to invalidate cached decode");
 const auto other=constantStereo(-.125f,.375f);
 check(f.c->loadWav(0,files.write("cache-replacement.wav",other))==Controller::LoadStatus::Accepted,
       "A08 replacement WAV rejected");
 check(f.c->sampleEditorSnapshot(0,edit,frames,envelope,0,0,&revision,&right,&stereo)&&
       edit.gain==1.f&&envelope[0].minimum==-.125f&&right[0].maximum==.375f&&
       qa::auditWaveDecodeMisses==3&&qa::auditWaveFullReplies==3,
       "A08 peer WAV replacement incorrectly reused old PCM");
 // Component-only recall of a THIRD sample must never resurrect the
 // previous cached decode, even when the display name is identical.
 KitState recalled;recalled.grid.fill(0);recalled.visible=4;
 recalled.names[0]="cache-replacement.wav";
 recalled.wav[0]=constantStereo(.0625f,-.1875f);
 const auto bytes=Fixture::encode(recalled);
 qa::ByteStream memory(bytes.data(),bytes.size());
 check(f.p->setState(&memory)==Steinberg::kResultOk,
       "A08 processor project recall rejected");
 check(f.c->sampleEditorSnapshot(0,edit,frames,envelope,0,0,&revision,&right,&stereo)&&
       envelope[0].maximum==.0625f&&right[0].minimum==-.1875f&&
       qa::auditWaveDecodeMisses==4&&qa::auditWaveFullReplies==4,
       "A08 recall failed to invalidate decoded sample cache");
 check(f.c->clearSample(0)&&
       !f.c->sampleEditorSnapshot(0,edit,frames,envelope),
       "A08 clear did not invalidate editor source");
 f.closeLast();
 check(IntegrationAccess::waveCacheEmpty(*f.c),
       "A08 last View close retained decoded WAV in Controller");
}
void testA07NotificationFaultIsolation(){
 FinalAuditFiles files;
 Fixture f;KitState initial;initial.visible=2;initial.grid.fill(0);
 f.kit(initial);prepareAudio(*f.p);f.open();f.open();
 const auto file=files.write("notification-fault.wav",constantStereo(.125f,-.375f));
 qa::auditViewRefreshFaultHits=0;
 qa::auditMutation=qa::AuditMutation::refreshViewsOOM;
 Controller::LoadStatus loaded=Controller::LoadStatus::ReadError;
 bool escaped=false;
 try {loaded=f.c->loadWav(0,file);}catch(...){escaped=true;}
 qa::auditMutation=qa::AuditMutation::none;
 check(!escaped&&loaded==Controller::LoadStatus::Accepted,
       "A07 accepting the WAV escaped or lied about a failing peer-view refresh");
 check(qa::auditViewRefreshFaultHits.load()==2,
       "A07 peer-view repaint failure path was not exercised twice");
 check(auditReturnedKit(*f.p).wav[0]==constantStereo(.125f,-.375f)&&
       f.c->hasSample(0),
       "A07 accepted Sample/Controller state diverged");
 check(IntegrationAccess::pendingDirty(*f.c),
       "A07 host without dirty handler silently lost post-commit project-dirty request");
 AuditAudioBlock audio;
 check(f.c->previewSample(0)&&f.p->process(audio.data)==kResultOk&&
       audio.left[80]==.125f&&audio.right[80]==-.375f,
       "A07 accepted sample fails next playable callback after two view failures");
}
void testA06CommittedImportSurvivesRepaintFailure(){
 FinalAuditFiles files;
 Fixture f;KitState initial;initial.visible=2;initial.grid.fill(0);
 f.kit(initial);prepareAudio(*f.p);f.open();
 const auto wav=constantStereo(.375f,-.125f);
 const auto file=files.write("repaint-oom-source.wav",wav);
 qa::auditMutation=qa::AuditMutation::loadRepaintOOM;
 Controller::LoadStatus status=Controller::LoadStatus::ReadError;
 bool escaped=false;
 try{status=f.c->loadWav(0,file);}catch(...){escaped=true;}
 qa::auditMutation=qa::AuditMutation::none;
 check(!escaped&&status==Controller::LoadStatus::Accepted,
       "A06 host accepted WAV but Controller reported failure after repaint OOM");
 check(auditReturnedKit(*f.p).wav[0]==wav&&f.c->hasSample(0)&&
       IntegrationAccess::paths(*f.c)[0]==file,
       "A06 committed WAV/Controller path diverged on post-commit OOM");
 AuditAudioBlock audio;
 check(f.c->previewSample(0)&&f.p->process(audio.data)==kResultOk&&
       audio.left[80]==.375f&&audio.right[80]==-.125f,
       "A06 accepted WAV not playable after post-commit repaint failure");
}
void testA05DenseSongAutomationDoesNotReschedule(){
 auto initial=[]{
   KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.names[0]="song.wav";kit.wav[0]=constantStereo(.25f,-.5f);
   kit.grid[0]=127;kit.songEnabled=1;
   check(kit.chain.setCount(1)&&kit.chain.configure(0,0,32),
         "A05 valid Song chain");
   return kit;
 };
 Fixture reference,automated;const auto kit=initial();
 reference.kit(kit);automated.kit(kit);
 prepareAudio(*reference.p);prepareAudio(*automated.p);
 AuditAudioBlock plain,dense;
 for(auto* block:{&plain,&dense}){
   block->clock.state=ProcessContext::kPlaying|
       ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
   block->clock.projectTimeMusic=0.;
   block->clock.tempo=120.;
 }
 ParameterChanges changes;
 Steinberg::int32 queueIndex=0,pointIndex=0;
 auto* queue=changes.addParameterData(kPatternParam,queueIndex);
 check(queue!=nullptr,"A05 host automation parameter missing");
 for(Steinberg::int32 i=0;i<256;++i)
   check(queue->addPoint(i,static_cast<double>(i%kPatterns)/7.,pointIndex)==kResultOk,
         "A05 dense automation points rejected");
 dense.data.inputParameterChanges=&changes;
 qa::auditSongScheduleCalls.store(0);
 check(reference.p->process(plain.data)==kResultOk,
       "A05 control Song render rejected");
 const auto baselineCalls=qa::auditSongScheduleCalls.exchange(0);
 check(automated.p->process(dense.data)==kResultOk,
       "A05 dense Song render rejected");
 const auto denseCalls=qa::auditSongScheduleCalls.exchange(0);
 check(baselineCalls==1&&denseCalls==1,
       "A05 dense Song A-H automation triggered unnecessary per-point DSP schedules");
 for(std::size_t frame=0;frame<plain.left.size();++frame)
   check(plain.left[frame]==dense.left[frame]&&
         plain.right[frame]==dense.right[frame],
         "A05 song PCM differs with unrelated host A-H automation");
 check(plain.left[80]==.25f&&plain.right[80]==-.5f,
       "A05 Song reference not actually audible");
 check(auditReturnedKit(*automated.p).selected==7,
       "A05 host selected program did not persist after sample-accurate points");
}
void testA04PreviewRateLifecycle(){
 // Real processor/controller, including pending and previously audible
 // previews. Check both paths across two legal host-rate transitions.
 for(const bool adoptOld:{false,true}){
   Fixture f;fa23Kit(f);AuditAudioBlock audio;
   check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)),
        "A04 cannot publish preview at 48k");
   if(adoptOld) {
     check(f.p->process(audio.data)==kResultOk &&
           audio.left[80]==.25f && audio.right[80]==-.5f,
           "A04 initial 48k preview not audible");
   }
   check(f.p->setProcessing(false)==kResultOk &&
         f.p->setActive(false)==kResultOk,
         "A04 stop before reconfigure failed");
   ProcessSetup changed{};changed.sampleRate=44100.;
   changed.maxSamplesPerBlock=256;changed.symbolicSampleSize=kSample32;
   changed.processMode=kRealtime;
   check(f.p->setupProcessing(changed)==kResultOk &&
         f.p->setActive(true)==kResultOk &&
         f.p->setProcessing(true)==kResultOk,
         "A04 48k-to-44.1k processing restart rejected");
   check(f.p->process(audio.data)==kResultOk &&
         audio.left[80]==0.f && audio.right[80]==0.f,
         "A04 old preview sounded after 44.1k setup");
   check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)),
         "A04 new-rate preview rejected");
   check(f.p->process(audio.data)==kResultOk &&
         std::isfinite(audio.left[80]) &&
         audio.left[80]>.20f && audio.left[80]<.30f &&
         audio.right[80]<-.40f && audio.right[80]>-.60f,
         "A04 new-rate preview did not render fresh sinc kernel");
   check(f.p->setProcessing(false)==kResultOk &&
         f.p->setActive(false)==kResultOk,
         "A04 stop before second reconfigure rejected");
   changed.sampleRate=48000.;
   check(f.p->setupProcessing(changed)==kResultOk &&
         f.p->setActive(true)==kResultOk &&
         f.p->setProcessing(true)==kResultOk,
         "A04 44.1k-to-48k processing restart rejected");
   check(f.p->process(audio.data)==kResultOk &&
         audio.left[80]==0.f && audio.right[80]==0.f,
         "A04 preview from prior sample rate revived");
   check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)) &&
         f.p->process(audio.data)==kResultOk &&
         audio.left[80]==.25f && audio.right[80]==-.5f,
         "A04 fresh 48k preview after two reconfigurations failed");
 }
}
void testRv02AllocationFreeStopAndRetry(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)) &&
       f.p->process(audio.data)==kResultOk && audio.left[80]==.25f,
       "RV02 initial audition failed");
 // The stop is already reserved: fail the very next potential allocation.
 allocationFail=0;
 bool escaped=false,stopped=false;
 try {stopped=f.c->stopSampleEditPreview();}
 catch(const std::bad_alloc&){escaped=true;}
 allocationFail=-1;
 check(!escaped&&stopped,"RV02 STOP depended on late host allocation");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f,
       "RV02 allocation-free STOP did not silence active audition");
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024)) &&
       f.p->process(audio.data)==kResultOk && audio.left[80]==.25f,
       "RV02 restart audition failed");
 qa::auditMutation=qa::AuditMutation::rejectPreviewStop;
 const bool rejected=f.c->stopSampleEditPreview();
 qa::auditMutation=qa::AuditMutation::none;
 check(!rejected,"RV02 rejected STOP falsely reported success");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "RV02 rejected STOP not actually exercised against an active preview");
 check(f.c->stopSampleEditPreview(),"RV02 pending STOP could not be retried");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f,
       "RV02 retried STOP left audition audible");
}
void testRv02NativeEditorCloseTransientReject(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 auto& view=fa23Open(f);
 check(IntegrationAccess::sampleEditorOpen(view),
       "RV02 no native WAV editor to close");
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&
       f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "RV02 closing preview must first be audible");
 qa::auditMutation=qa::AuditMutation::rejectPreviewStopOnce;
 f.closeLast(); // real VSTGUI removed/destroy, no later view to retry
 check(qa::auditMutation.load()==qa::AuditMutation::none,
       "RV02 close did not exercise exactly one STOP rejection");
 check(f.p->process(audio.data)==kResultOk&&
       audio.left[80]==0.f&&audio.right[80]==0.f,
       "RV02 transient STOP rejection left audio alive after editor close");
}
void testRv02ApplyRejectKeepsEdit(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 auto& view=fa23Open(f);
 const SampleEdit proposal{20,900,0,0,.5f};
 check(proposal.valid(1024),"RV02 apply proposal invalid");
 const auto previous=auditReturnedKit(*f.p).sampleEdits[0];
 IntegrationAccess::setSampleEditorDraft(view,proposal);
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&
       f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "RV02 apply preview setup failed");
 qa::auditMutation=qa::AuditMutation::rejectPreviewStop;
 click(view,975.,475.);
 qa::auditMutation=qa::AuditMutation::none;
 const auto rejected=auditReturnedKit(*f.p).sampleEdits[0];
 check(IntegrationAccess::sampleEditorOpen(view)&&
       rejected.start==previous.start&&rejected.end==previous.end&&
       rejected.gain==previous.gain,
       "RV02 rejected STOP committed edit behind modal");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "RV02 rejected STOP did not preserve previous audible preview");
 click(view,975.,475.);
 const auto applied=auditReturnedKit(*f.p).sampleEdits[0];
 check(!IntegrationAccess::sampleEditorOpen(view)&&
       applied.start==proposal.start&&applied.end==proposal.end&&
       applied.gain==proposal.gain,
       "RV02 Apply retry did not commit exactly once after STOP");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f,
       "RV02 successful Apply left audible preview");
}
void testA03PersistentStopRejectionRecovery(){
 Fixture f;fa23Kit(f);AuditAudioBlock audio;
 (void)fa23Open(f);
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&
       f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "A03 initial audition absent");
 qa::auditMutation=qa::AuditMutation::rejectPreviewStop;
 f.closeLast(); // host closes Editor despite rejected STOP
 check(!f.c->previewSampleEdit(0,SampleEdit::reset(1024)),
       "A03 unresolved STOP allowed an overtaking new audition");
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "A03 persistent rejection did not exercise audible one-shot");
 for(int block=0;block<4;++block)
   check(f.p->process(audio.data)==kResultOk,
         "A03 one-shot processing after view close failed");
 check(audio.left[80]==0.f&&audio.right[80]==0.f,
       "A03 failed STOP left a completed one-shot audibly stuck");
 qa::auditMutation=qa::AuditMutation::none;
 check(f.c->previewSampleEdit(0,SampleEdit::reset(1024))&&
       f.p->process(audio.data)==kResultOk&&audio.left[80]==.25f,
       "A03 recovery did not retry the pending STOP before new preview");
 // A disconnected host cannot guarantee immediate silence; only natural
 // one-shot end and refusal of new START are proven by this case.
}
void testR05SampleMenuAllocationFailures(){
 Fixture f;fa23Kit(f);
 for(long failPoint=0;failPoint<14;++failPoint){
   bool escaped=false;
   allocationFail=failPoint;
   try {(void)f.c->previewSample(0);}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"R05 preview-row OOM escaped Controller boundary");
   KitState candidate;candidate.grid.fill(0);candidate.visible=4;
   candidate.names[0]="source.wav";candidate.wav[0]=constantStereo(.25f,-.5f);
   f.kit(candidate);
   bool accepted=false;escaped=false;
   allocationFail=failPoint;
   try {accepted=f.c->clearSample(0);}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"R05 clear-sample OOM escaped Controller boundary");
   const auto saved=auditReturnedKit(*f.p);
   check(accepted?saved.wav[0].empty():!saved.wav[0].empty(),
         "R05 clear rejection altered processor sample");
   check(f.c->hasSample(0)==!saved.wav[0].empty(),
         "R05 Controller and Processor disagree on sample presence");
 }
}
void testRv03RecallQueryOOM(){
 const std::array<qa::AuditMutation,2> faults{
     qa::AuditMutation::patternQueryOOM,qa::AuditMutation::patternReplyOOM};
 for(const auto fault:faults)for(const int variant:{0,1,2,3}){
   Fixture f;
   auto previous=namedKit('X'),incoming=namedKit('Y');
   incoming.selected=2;
   f.kit(previous);
   const auto encoded=Fixture::encode(incoming);
   qa::ByteStream processorStream(encoded.data(),encoded.size());
   check(f.p->setState(&processorStream)==kResultOk,
         "RV03 target Processor state rejected");
   check(f.c->setParamNormalized(kPatternParam,1.)==kResultOk,
         "RV03 host selection setup rejected");
   std::vector<std::uint8_t> component=encoded;
   if(variant>0){
     const std::uint32_t magic=variant==1?0x31524d44u:
       (variant==2?0x32524d44u:0x33524d44u);
     component.clear();
     for(auto number:{magic,std::uint32_t{2}})
       for(int i=0;i<4;++i)
         component.push_back(static_cast<std::uint8_t>(number>>(i*8)));
     if(variant==3)component.push_back(4);
     if(variant!=1)component.insert(component.end(),kCells,19);
   }
   qa::auditMutation=fault;
   bool escaped=false;tresult result=kResultFalse;
   try {result=f.recall(component);}
   catch(const std::bad_alloc&){escaped=true;}
   qa::auditMutation=qa::AuditMutation::none;
   check(!escaped&&result==kResultOk,
         "RV03 query failure escaped or falsely rejected committed recall");
   check(f.c->currentPattern()==2&&
         auditReturnedKit(*f.p).selected==2,
         "RV03 recall pattern diverged from restored Processor");
   if(variant==0) {
     check(f.c->velocity(0,0,0)==93&&
           f.c->label(0)==std::string(40,'Y'),
           "RV03 full Kit recall changed grid or name after query failure");
   }else {
     check(!f.c->hasSample(0)&&f.c->velocity(0,0,0)==
           (variant==1?127:19),
           "RV03 legacy recall failed after query OOM");
   }
 }
}
void testFr02ControllerMessageOOM(){
 Fixture f;fa23Kit(f);
 for(long failPoint=0;failPoint<16;++failPoint){
   bool escaped=false;
   allocationFail=failPoint;
   try {(void)f.c->previewSampleEdit(0,SampleEdit::reset(1024));}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"FR02 preview controller attribute OOM escaped UI path");
   escaped=false;allocationFail=failPoint;
   try {(void)f.c->commitSampleEdit(0,SampleEdit::reset(1024));}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"FR02 apply controller attribute OOM escaped UI path");
   SampleEdit edit;std::uint32_t frames=0;
   std::array<WaveformRange,256> wave{};
   escaped=false;allocationFail=failPoint;
   try {(void)f.c->sampleEditorSnapshot(0,edit,frames,wave);}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"FR02 query controller attribute OOM escaped UI path");
   escaped=false;allocationFail=failPoint;
   try {f.c->stopSampleEditPreview();}
   catch(const std::bad_alloc&){escaped=true;}
   allocationFail=-1;
   check(!escaped,"FR02 STOP message OOM escaped UI path");
 }
 check(!auditReturnedKit(*f.p).wav[0].empty(),"FR02 OOM corrupted source");
}

void testFr03LegacyRecallSamplePresence(){
 for(std::uint32_t magic:{0x31524d44u,0x32524d44u,0x33524d44u}){
   Fixture f;fa23Kit(f);f.open();
   check(f.c->hasSample(0),"FR03 loaded source missing before legacy recall");
   std::vector<std::uint8_t> legacy;
   for(auto n:{magic,std::uint32_t{0}})
     for(int i=0;i<4;++i)
       legacy.push_back(static_cast<std::uint8_t>(n>>(i*8)));
   if(magic==0x33524d44u)legacy.push_back(4);
   if(magic!=0x31524d44u)legacy.insert(legacy.end(),kCells,19);
   check(f.recall(legacy)==kResultOk,"FR03 legacy controller recall rejected");
   check(!f.c->hasSample(0),
         "FR03 stale sample-present bit enabled EDIT on empty legacy state");
 }
}
void testFr08PathSerializationBoundary(){
 Fixture f;fa23Kit(f);
 const auto old=auditReturnedKit(*f.p).wav[0];
 const std::string tooLong=std::string(4093u,'x')+".wav";
 check(tooLong.size()==4097u,"FR08 pathname fixture length");
 check(f.c->loadWav(0,tooLong)==Controller::LoadStatus::Rejected,
       "FR08 invalid unserializable path accepted for import");
 check(auditReturnedKit(*f.p).wav[0]==old,
       "FR08 rejected path changed embedded audio");
}

std::vector<std::uint32_t> frGuiPixels(DrumatorView& view,
                                     unsigned x0,unsigned y0,
                                     unsigned x1,unsigned y1){
 const auto size=view.getViewSize();
 auto bitmap=VSTGUI::renderBitmapOffscreen(
     VSTGUI::CPoint(size.getWidth(),size.getHeight()),1.0,
     [&](VSTGUI::CDrawContext& ctx){view.draw(&ctx);});
 check(bitmap!=nullptr,"FR GUI offscreen draw unavailable");
 auto* pixels=VSTGUI::CBitmapPixelAccess::create(bitmap.get(),false);
 check(pixels!=nullptr,"FR GUI offscreen pixel access unavailable");
 check(x1<=pixels->getBitmapWidth()&&y1<=pixels->getBitmapHeight(),
       "FR GUI capture bounds outside editor");
 std::vector<std::uint32_t> result;
 result.reserve(static_cast<std::size_t>(x1-x0)*(y1-y0));
 for(unsigned y=y0;y<y1;++y)
   for(unsigned x=x0;x<x1;++x){
     check(pixels->setPosition(x,y),"FR GUI pixel coordinate invalid");
     VSTGUI::CColor color{};pixels->getColor(color);
     result.push_back((static_cast<std::uint32_t>(color.red)<<16)|
                      (static_cast<std::uint32_t>(color.green)<<8)|color.blue);
   }
 pixels->forget();return result;
}
void testFr04RealWaveformEdgeMouse(){
 Fixture f;fa23Kit(f);auto& view=fa23Open(f);
 check(IntegrationAccess::sampleEditorDraft(view).start==0,
       "FR04 original marker state wrong");
 click(view,500.,165.);
 check(IntegrationAccess::sampleEditorDraft(view).start>0,
       "FR04 top 20px waveform strip not interactive");
}
void testFr05ShortFadePixelsMatchAudioContract(){
 Fixture f;fa23Kit(f);auto& view=fa23Open(f);
 const SampleEdit zeroOnLastFrame{511,512,0,1,1.f};
 check(zeroOnLastFrame.valid(1024)&&zeroOnLastFrame.amplitude(511)==0.f,
       "FR05 one-frame Fade Out PCM contract");
 IntegrationAccess::setSampleEditorDraft(view,zeroOnLastFrame);
 const auto faded=frGuiPixels(view,300,157,980,381);
 auto silent=zeroOnLastFrame;silent.gain=0.f;
 IntegrationAccess::setSampleEditorDraft(view,silent);
 const auto silentPixels=frGuiPixels(view,300,157,980,381);
 check(faded==silentPixels,
       "FR05 one-frame Fade Out graphic differs from its silent DSP envelope");
 IntegrationAccess::setSampleEditorDraft(view,SampleEdit::reset(1024));
 const auto full=frGuiPixels(view,300,157,980,381);
 check(full!=silentPixels,
       "FR05 negative control: live waveform renderer never shows audible signal");
 // RV04: hold start/end/fades fixed while changing gain. The original
 // renderer ignored gain entirely; the old code must fail this assertion.
 const SampleEdit audibleRange{128,896,0,0,1.f};
 auto mutedRange=audibleRange;
 mutedRange.gain=0.f;
 IntegrationAccess::setSampleEditorDraft(view,audibleRange);
 const auto audibleRangePixels=frGuiPixels(view,300,157,980,381);
 IntegrationAccess::setSampleEditorDraft(view,mutedRange);
 const auto mutedRangePixels=frGuiPixels(view,300,157,980,381);
 check(audibleRangePixels!=mutedRangePixels,
       "RV04 waveform renderer ignores edit gain; old fade-oracle blind spot");
}
void testFr06ModalErrorPixels(){
 Fixture f;fa23Kit(f);auto& view=fa23Open(f);
 IntegrationAccess::setStatusText(view,"");
 const auto without=frGuiPixels(view,265,556,1020,585);
 IntegrationAccess::setStatusText(view,"ZOOM: WAVEFORM UNAVAILABLE");
 const auto with=frGuiPixels(view,265,556,1020,585);
 check(with!=without,"FR06 actual modal error text not visible on native GUI pixels");
}
// RV05 independent raster oracle: the actual circle must be centered on
// the layout hit center. A return to old painted x=80/178/1178 fails.
void testRv05GlobalKnobPaintCenters(){
 Fixture f;fa23Kit(f);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 for(const double center:UiCompactLayout::globalKnobCenters){
   const auto left=static_cast<unsigned>(center-24.);
   const auto pixels=frGuiPixels(*view,left,61,left+48,63);
   const auto backgroundColor=pixels[0];
   double sum=0.,count=0.;
   for(unsigned y=0;y<2;++y)
     for(unsigned x=0;x<48;++x){
       if(pixels[y*48+x]==backgroundColor)continue;
       sum+=static_cast<double>(left+x);
       count+=1.;
     }
   check(count>=2.,"RV05 visible rotary outline missing from pixel capture");
   check(std::abs(sum/count-center)<0.85,
         "RV05 painted rotary center does not match input hit center");
 }
}
void testFr10GlobalKnobEdgeHit(){
 Fixture f;fa23Kit(f);f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 check(f.c->swingPercent()==0,"FR10 fixture swing should default to zero");
 VSTGUI::CPoint point(UiCompactLayout::globalKnobCenters[0]+16.5,76.);
 view->onMouseDown(point,VSTGUI::CButtonState(VSTGUI::kLButton));
 point.y-=25.;
 view->onMouseMoved(point,VSTGUI::CButtonState(VSTGUI::kLButton));
 view->onMouseUp(point,{});
 check(f.c->swingPercent()>0,"FR10 visible Swing knob edge is not draggable");
}
