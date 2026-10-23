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
