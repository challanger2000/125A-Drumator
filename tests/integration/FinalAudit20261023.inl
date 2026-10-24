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
