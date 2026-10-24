// Independent re-audit: original fresh-state matrix and deterministic VFA02.
// Sources of the copied cases: audit commit 0833f8b. Product classes are real.
struct FinalAuditFiles {
 std::filesystem::path directory=std::filesystem::current_path()/"125A-final-audit-files";
 FinalAuditFiles(){std::filesystem::create_directories(directory);}
 ~FinalAuditFiles(){std::error_code ec;std::filesystem::remove_all(directory,ec);}
 std::string write(const char* name,const std::vector<std::uint8_t>& bytes){
   const auto path=directory/name;
   std::ofstream file(path,std::ios::binary|std::ios::trunc);
   file.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
   check(file.good(),"VFA actual sample fixture write");return pathToUtf8(path);
 }
};
std::vector<std::uint8_t> finalAuditControllerBytes(Controller& controller){
 ResizableMemoryIBStream stream;
 check(controller.getState(&stream)==kResultOk,"VFA controller state save");
 const auto* first=static_cast<const std::uint8_t*>(stream.getData());
 return {first,first+static_cast<std::size_t>(stream.getCursor())};
}
tresult finalAuditControllerRecall(Controller& controller,const std::vector<std::uint8_t>& bytes){
 qa::ByteStream stream(bytes.data(),bytes.size());return controller.setState(&stream);
}
void testFinalAuditDzm3(){
 FinalAuditFiles files;
 const auto wavA=constantStereo(.25f,-.5f),wavB=constantStereo(-.125f,.375f);
 const auto pathA=files.write("a.wav",wavA),pathB=files.write("b.wav",wavB);
 for(double userZoom:{1.,1.2,1.3,1.5})for(bool controllerFirst:{true,false}){
   std::vector<std::uint8_t> componentBytes,controllerBytes;
   {
     Fixture source;KitState kit;kit.grid.fill(0);source.kit(kit);prepareAudio(*source.p);
     check(source.c->loadWav(0,pathA)==Controller::LoadStatus::Accepted,"VFA DZM3 source file load");
     source.c->rememberZoom(userZoom);
     controllerBytes=finalAuditControllerBytes(*source.c);
     componentBytes=Fixture::encode(auditReturnedKit(*source.p));
     source.p->setProcessing(false);source.p->setActive(false);
   } // Destroy ALL source instances, not an in-place controller roundtrip.
   Fixture destination;
   qa::ByteStream processorStream(componentBytes.data(),componentBytes.size());
   check(destination.p->setState(&processorStream)==kResultOk,"VFA fresh processor recall");
   if(controllerFirst)check(finalAuditControllerRecall(*destination.c,controllerBytes)==kResultOk,
                            "VFA DZM3 controller-first restore");
   check(destination.recall(componentBytes)==kResultOk,"VFA fresh component recall");
   if(!controllerFirst)check(finalAuditControllerRecall(*destination.c,controllerBytes)==kResultOk,
                             "VFA DZM3 component-first restore");
   auto& editor=destination.open();
   check(std::abs(editor.getZoomFactor()-userZoom)<.001&&
         IntegrationAccess::view(*destination.c,0)->zoomMatches(userZoom)&&
         IntegrationAccess::paths(*destination.c)[0]==pathA,"VFA fresh origin/zoom lost");
   prepareAudio(*destination.p);
   check(auditReturnedKit(*destination.p).wav[0]==wavA,"VFA embedded audio changed by controller state");
   check(destination.c->stepSample(0,+1)==Controller::LoadStatus::Accepted&&
         IntegrationAccess::paths(*destination.c)[0]==pathB&&
         auditReturnedKit(*destination.p).wav[0]==wavB,"VFA fresh next navigation");
   check(destination.c->stepSample(0,-1)==Controller::LoadStatus::Accepted&&
         IntegrationAccess::paths(*destination.c)[0]==pathA,"VFA fresh previous navigation");
   // Every proper prefix must reject before changing zoom, paths, Undo or edits.
   const auto before=IntegrationAccess::snapshot(*destination.c);
   for(std::size_t length=0;length<controllerBytes.size();++length){
     qa::ByteStream truncated(controllerBytes.data(),length);
     check(destination.c->setState(&truncated)!=kResultOk&&
           IntegrationAccess::snapshot(*destination.c)==before,"VFA partial DZM3 state mutation");
   }
   auto corrupt=controllerBytes;corrupt[4]=4;
   check(finalAuditControllerRecall(*destination.c,corrupt)!=kResultOk&&
         IntegrationAccess::snapshot(*destination.c)==before,"VFA invalid DZM3 zoom accepted");
   corrupt=controllerBytes;corrupt[7]=0;
   check(finalAuditControllerRecall(*destination.c,corrupt)!=kResultOk&&
         IntegrationAccess::snapshot(*destination.c)==before,"VFA DZM3 embedded NUL accepted");
   allocationFail=0;
   const auto rejected=finalAuditControllerRecall(*destination.c,controllerBytes);
   allocationFail=-1;
   check(rejected!=kResultOk&&IntegrationAccess::snapshot(*destination.c)==before,
         "VFA DZM3 path-allocation failure mutated state");
   destination.closeLast();destination.open();
   check(IntegrationAccess::view(*destination.c,0)->zoomMatches(userZoom),"VFA DZM3 reopen zoom");
   destination.p->setProcessing(false);destination.p->setActive(false);
 }
 qa::workflowNotice("VFA01 PASS: fresh instances, four DZM3 zooms, both stream orders, real folder navigation, every truncated prefix/NUL/invalid zoom/OOM and reopen");
}
void testReauditPatternLength(){
 Fixture f;KitState kit;kit.visible=1;kit.grid.fill(0);
 kit.names[0]="length-race.wav";kit.wav[0]=constantStereo(.5f,-.25f);
 kit.patternLengths[0]=32;kit.patternLengths[1]=16;
 kit.grid[kRows*kSteps]=100;kit.grid[kRows*kSteps+16]=50;
 f.kit(kit);prepareAudio(*f.p);
 check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&f.c->copyCurrentPattern()&&
       f.c->setParamNormalized(kPatternParam,0.)==kResultOk,"VFA02 copy 1T source");
 AuditAudioBlock audio;
 audio.clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 // Establish uninterrupted phase: 375 * 256 samples = four quarter notes.
 for(unsigned block=0;block<375;++block){
   audio.clock.projectTimeMusic=static_cast<double>(block*256u)/24000.;
   check(f.p->process(audio.data)==kResultOk,"VFA02 continuous first bar");
 }
 audio.clock.projectTimeMusic=4.;
 arm(qa::auditAfterLengthRead);
 tresult rendered=kResultFalse;
 std::jthread audioActor([&]{rendered=f.p->process(audio.data);});
 waitEntered();
 const bool pasted=f.c->pasteCurrentPattern();
 releaseProbe();audioActor.join();
 check(pasted&&rendered==kResultOk&&!probeTimeout,"VFA02 controlled actual paste/audio interleave");
 const auto returned=auditReturnedKit(*f.p);
 check(returned.patternLengths[0]==16&&returned.grid[0]==100&&returned.grid[16]==50,
       "VFA02 pasted state oracle");
 const double expected=.5*100./127.,hidden=.5*50./127.;
 qa::workflowNotice("VFA02 observed L[80]="+std::to_string(audio.left[80])+"; coherent 1T expected="+
    std::to_string(expected)+"; mixed new-grid/old-32-length="+std::to_string(hidden));
 check(std::abs(expected-hidden)>.1,"VFA02 oracle cannot distinguish generations");
 f.p->setProcessing(false);f.p->setActive(false);
 const bool completeOld=audio.left[80]==0.f&&audio.right[80]==0.f;
 const bool completeNew=std::abs(audio.left[80]-expected)<2e-6&&
                        std::abs(audio.right[80]+expected*.5)<2e-6;
 check(completeOld||completeNew,
       "VFA02 FAIL: actual audio combines new pasted grid with old 32-step length");
}

struct ReauditMutationScope {
 explicit ReauditMutationScope(qa::AuditMutation mode){
   qa::auditImportTarget=nullptr;qa::auditImportCalls=0;qa::auditDelayCalls=0;
   qa::auditMutation=mode;
 }
 ~ReauditMutationScope(){qa::auditMutation=qa::AuditMutation::none;qa::auditImportTarget=nullptr;}
};
template<class Test>
void expectMutationKilled(qa::AuditMutation mode,const char* label,Test test){
 test(); // A red control must never count as detection of the mutation.
 ReauditMutationScope scope(mode);bool killed=false;std::string reason;
 try{test();}catch(const std::exception& ex){killed=true;reason=ex.what();}
 qa::workflowNotice(std::string("MUTATION ")+label+": "+(killed?"KILLED: ":"SURVIVED")+reason);
 check(killed,"Relevant injected mutation survived the existing test");
}
void testReauditDurationFallback(){
 for(bool song:{false,true}){
   Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
   kit.wav[0]=constantStereo(.5f,-.25f);kit.names[0]="fallback.wav";
   kit.patternLengths[0]=16;kit.patternLengths[1]=32;
   kit.grid[0]=100;kit.grid[16]=50;
   kit.grid[kRows*kSteps]=70;kit.grid[kRows*kSteps+16]=30;
   kit.chain.setCount(1);kit.chain.configure(0,0,32);kit.songEnabled=song?1:0;
   f.kit(kit);prepareAudio(*f.p);
   check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&f.c->copyCurrentPattern()&&
         f.c->setParamNormalized(kPatternParam,0.)==kResultOk,"RE03 clipboard");
   AuditAudioBlock audio;
   audio.clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
   for(unsigned block=0;block<375;++block){
     audio.clock.projectTimeMusic=static_cast<double>(block*256u)/24000.;
     check(f.p->process(audio.data)==kResultOk,"RE03 cache warmup");
   }
   audio.clock.projectTimeMusic=4.;arm(qa::patternMidWrite);
   tresult rendered=kResultFalse;bool reached=false;
   std::jthread reader([&]{
     reached=WaitForSingleObject(entered,10000)==WAIT_OBJECT_0;
     if(reached)rendered=f.p->process(audio.data);
     releaseProbe();
   });
   const bool pasted=f.c->pasteCurrentPattern();reader.join();
   check(reached&&pasted&&rendered==kResultOk&&!probeTimeout,"RE03 actual fallback interleave");
   check(std::abs(audio.left[80]-.5*100./127.)<2e-6&&
         std::abs(audio.right[80]+.25*100./127.)<2e-6,"RE03 cache mixed old grid and new duration");
   f.p->setProcessing(false);f.p->setActive(false);
 }
 qa::workflowNotice("RE03 actual Pattern/Song fallback: old complete 16-step generation survives paused 32-step writer PASS");
}
void testReauditPeerFill(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.grid[0]=60;f.kit(kit);
 f.open();f.open();auto* first=IntegrationAccess::view(*f.c,0);auto* peer=IntegrationAccess::view(*f.c,1);
 click(*first,1248.,168.);click(*first,630.,214.);
 check(std::get<0>(IntegrationAccess::viewSnapshot(*first)),"RE04 first gesture absent");
 // Native View FILL menu on row two, ALL item; a bulk Controller operation.
 const auto rowY=IntegrationAccess::rowTop(*peer,1);
 click(*peer,425.,rowY+20.);
 const auto menuTop=UiCompactLayout::popupTop(rowY,peer->getViewSize().getHeight(),230.);
 click(*peer,430.,menuTop+4.*25.+10.);
 check(f.c->velocity(0,1,0)==127&&auditReturnedKit(*f.p).grid[kSteps]==127,
       "RE04 peer FILL was never committed");
 first->onMouseCancel();
 const auto result=f.c->velocity(0,1,0);
 qa::workflowNotice("RE04 actual peer FILL row2 step1: before Cancel=127 after="+std::to_string(result));
 check(result==127&&auditReturnedKit(*f.p).grid[kSteps]==127,
       "RE04 peer FILL erased by old velocity Cancel");
}
void testReauditOriginOrder(){
 FinalAuditFiles files;const auto x=constantStereo(.25f,-.5f),y=constantStereo(-.125f,.375f);
 const auto path=files.write("kick.wav",x);
 for(bool controllerFirst:{true,false}){
   Fixture f;KitState kit;kit.grid.fill(0);f.kit(kit);prepareAudio(*f.p);
   check(f.c->loadWav(0,path)==Controller::LoadStatus::Accepted,"RE05 actual origin load");
   const auto component=Fixture::encode(auditReturnedKit(*f.p));
   const auto control=finalAuditControllerBytes(*f.c);
   if(controllerFirst)check(finalAuditControllerRecall(*f.c,control)==kResultOk,"RE05 Controller first");
   check(f.recall(component)==kResultOk,"RE05 complete project component restore");
   if(!controllerFirst)check(finalAuditControllerRecall(*f.c,control)==kResultOk,"RE05 Controller last");
   check(IntegrationAccess::paths(*f.c)[0]==path,"RE05 matching project lost origin");
   KitState different;different.grid.fill(0);different.names[0]="kick.wav";different.wav[0]=y;
   const auto candidate=Fixture::encode(different);
   check(f.recall(candidate)==kResultOk,"RE05 next independent Component-only restore");
   const bool empty=IntegrationAccess::paths(*f.c)[0].empty();
   qa::workflowNotice(std::string("RE05 next-kit origin cleared after ")+
     (controllerFirst?"Controller-first":"Component-first")+" full project="+(empty?"YES":"NO"));
   f.p->setProcessing(false);f.p->setActive(false);
   check(empty,"RE05 completed Component-first restore authorizes stale origin in next unrelated kit");
 }
}
// Exact SDK 3.8.1 interfaces are siblings, not an inheritance chain.
struct ReauditDirtyHost final:IComponentHandler,IComponentHandler2 {
 std::function<void()> capture;
 unsigned calls=0;
 tresult PLUGIN_API queryInterface(const TUID requestedIid,void** object) override {
   if(!object)return kInvalidArgument;
   if(FUnknownPrivate::iidEqual(requestedIid,IComponentHandler::iid)||
      FUnknownPrivate::iidEqual(requestedIid,FUnknown::iid))*object=static_cast<IComponentHandler*>(this);
   else if(FUnknownPrivate::iidEqual(requestedIid,IComponentHandler2::iid))*object=static_cast<IComponentHandler2*>(this);
   else {*object=nullptr;return kNoInterface;}
   addRef();return kResultOk;
 }
 uint32 PLUGIN_API addRef() override{return 1;}
 uint32 PLUGIN_API release() override{return 1;}
 tresult PLUGIN_API beginEdit(ParamID) override{return kResultOk;}
 tresult PLUGIN_API performEdit(ParamID,ParamValue) override{return kResultOk;}
 tresult PLUGIN_API endEdit(ParamID) override{return kResultOk;}
 tresult PLUGIN_API restartComponent(int32) override{return kResultOk;}
 tresult PLUGIN_API setDirty(TBool isDirty) override{if(isDirty){++calls;if(capture)capture();}return kResultOk;}
 tresult PLUGIN_API requestOpenEditor(FIDString) override{return kResultOk;}
 tresult PLUGIN_API startGroupEdit() override{return kResultOk;}
 tresult PLUGIN_API finishGroupEdit() override{return kResultOk;}
};
void testReauditDirtySave(){
 FinalAuditFiles files;const auto wav=constantStereo(.25f,-.5f);const auto path=files.write("dirty.wav",wav);
 Fixture f;KitState kit;kit.grid.fill(0);f.kit(kit);prepareAudio(*f.p);
 ReauditDirtyHost handler;std::vector<std::uint8_t> component,controller;
 handler.capture=[&]{component=Fixture::encode(auditReturnedKit(*f.p));controller=finalAuditControllerBytes(*f.c);};
 check(f.c->setComponentHandler(static_cast<IComponentHandler*>(&handler))==kResultOk,"RE06 actual handler setup");
 struct Detach {Controller* controller;~Detach(){controller->setComponentHandler(nullptr);}} detach{f.c};
 check(f.c->loadWav(0,path)==Controller::LoadStatus::Accepted&&handler.calls==1,
       "RE06 import/real setDirty callback failed");
 check(!component.empty()&&!controller.empty(),"RE06 callback did not save actual streams");
 Fixture fresh;qa::ByteStream input(component.data(),component.size());
 check(fresh.p->setState(&input)==kResultOk&&fresh.recall(component)==kResultOk&&
       finalAuditControllerRecall(*fresh.c,controller)==kResultOk,"RE06 captured two-stream restore failed");
 check(auditReturnedKit(*fresh.p).wav[0]==wav,"RE06 save did not capture NEW processor WAV");
 const auto restored=IntegrationAccess::paths(*fresh.c)[0];
 qa::workflowNotice("RE06 save within real setDirty: NEW embedded WAV retained; origin restored="+
                    std::string(restored==path?"YES":"NO"));
 f.p->setProcessing(false);f.p->setActive(false);
 check(restored==path,"RE06 reentrant host save captured new Processor with old Controller origin");
}
void testReauditHostMirror(){
 for(bool feedbackAfter:{true,false}){
   Fixture f;KitState old;old.selected=2;old.grid.fill(0);f.kit(old);prepareAudio(*f.p);
   KitState kit;kit.selected=6;kit.grid.fill(0);kit.visible=1;
   kit.names[0]="host-choice.wav";kit.wav[0]=constantStereo(.5f,-.25f);
   for(std::size_t p=0;p<kPatterns;++p)kit.grid[p*kRows*kSteps]=static_cast<std::uint8_t>(10+p*15);
   const auto bytes=Fixture::encode(kit);arm(qa::restoreMidCommit);
   // All Component/Controller calls stay serialized on the real fixture UI
   // thread. Only process() runs on the audio actor, as required by the SDK.
   tresult processed=kResultFalse;bool reached=false;
   std::jthread audioActor([&]{
     reached=WaitForSingleObject(entered,10000)==WAIT_OBJECT_0;
     if(reached){
       ParameterChanges changes;int32 index=0;
       changes.addParameterData(kPatternParam,index)->addPoint(0,1./7.,index);
       ProcessData flush{};flush.inputParameterChanges=&changes;
       processed=f.p->process(flush);
     }
     releaseProbe();
   });
   qa::ByteStream input(bytes.data(),bytes.size());const auto loaded=f.p->setState(&input);
   audioActor.join();
   check(reached&&loaded==kResultOk&&processed==kResultOk&&!probeTimeout,"RE07 host ordering fixture");
   if(!feedbackAfter)f.c->setParamNormalized(kPatternParam,1./7.);
   check(f.recall(bytes)==kResultOk,"RE07 original incoming Component state to Controller");
   if(feedbackAfter)f.c->setParamNormalized(kPatternParam,1./7.);
   const auto actual=auditReturnedKit(*f.p);AuditAudioBlock audio;
   audio.clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
   check(f.p->process(audio.data)==kResultOk,"RE07 final audible state");
   check(actual.selected==1&&std::abs(audio.left[80]-.5*25./127.)<2e-6,"RE07 Processor lost later host event");
   const auto visible=f.c->currentPattern();
   qa::workflowNotice(std::string("RE07 Controller feedback ")+(feedbackAfter?"AFTER":"BEFORE")+
      " incoming Component recall: audible Processor=B; Controller="+std::to_string(visible));
   f.p->setProcessing(false);f.p->setActive(false);
   check(visible==1,"RE07 Controller/GUI reverted to recalled G while Processor audibly remains B");
 }
}
void testReauditPcmTimeline(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;kit.names[0]="coded.wav";
 auto wav=constantStereo(.25f,-.5f);
 for(std::size_t frame=0;frame<1024;++frame){
   const float l=static_cast<float>(1+frame%13)/64.f,r=-static_cast<float>(1+frame%19)/128.f;
   std::memcpy(wav.data()+44+frame*8,&l,4);std::memcpy(wav.data()+48+frame*8,&r,4);
 }
 kit.wav[0]=wav;f.kit(kit);prepareAudio(*f.p);AuditAudioBlock audio;
 EventList notes;Event hit{};hit.type=Event::kNoteOnEvent;hit.sampleOffset=17;
 hit.noteOn.pitch=36;hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
 check(notes.addEvent(hit)==kResultOk,"RE08 MIDI setup");
 unsigned absolute=0;
 for(int32 frames:{128,256}){
   audio.data.numSamples=frames;audio.data.inputEvents=absolute==0?&notes:nullptr;
   check(f.p->process(audio.data)==kResultOk,"RE08 real Processor render");
   for(int32 sample=0;sample<frames;++sample){
     const auto at=absolute+static_cast<unsigned>(sample);
     const auto source=at>=17?at-17:0;
     const float l=at<17?0.f:static_cast<float>(1+source%13)/64.f;
     const float r=at<17?0.f:-static_cast<float>(1+source%19)/128.f;
     check(audio.left[static_cast<std::size_t>(sample)]==l&&audio.right[static_cast<std::size_t>(sample)]==r,
           "RE08 full independent PCM oracle detects missing/shifted/double/gapped audio");
   }
   absolute+=static_cast<unsigned>(frames);
 }
 f.p->setProcessing(false);f.p->setActive(false);
}
void testReauditPcmNegatives(){
 for(auto mode:{qa::AuditMutation::delayMidiOne,qa::AuditMutation::zeroPrefix,
                qa::AuditMutation::dropMidi,qa::AuditMutation::doubleMidi})
   expectMutationKilled(mode,"independent complete PCM",testReauditPcmTimeline);
}
void testReauditDeadlineDiagnostic(){
 testPacedSampleSwapContinuity();
 ReauditMutationScope scope(qa::AuditMutation::delayImportedProcessor);
 testPacedSampleSwapContinuity();
 check(qa::auditDelayCalls.load()>=8u&&qa::auditImportCalls.load()==12u,
       "RE deadline injection did not reach actual imported processor");
 qa::workflowNotice("MUTATION PACING eight 10ms callback stalls: function still PASS by its diagnostic-only deadline contract; inspect its published miss counter");
}
std::vector<std::uint8_t> reauditLongStereo(float left,float right,std::uint32_t rate,
                                          std::uint32_t frames=300000){
 auto wav=constantStereo(left,right);wav.resize(44+static_cast<std::size_t>(frames)*8);
 auto put=[&](std::size_t at,std::uint32_t value){
   for(unsigned byte=0;byte<4;++byte)wav[at+byte]=static_cast<std::uint8_t>(value>>(byte*8));
 };
 put(4,static_cast<std::uint32_t>(wav.size()-8));put(24,rate);put(28,rate*8);put(40,frames*8);
 for(std::uint32_t frame=0;frame<frames;++frame){
   std::memcpy(wav.data()+44+static_cast<std::size_t>(frame)*8,&left,4);
   std::memcpy(wav.data()+48+static_cast<std::size_t>(frame)*8,&right,4);
 }
 return wav;
}
void reaudit64Notes(EventList& events,unsigned rows){
 for(unsigned voice=0;voice<64;++voice){
   Event hit{};hit.type=Event::kNoteOnEvent;hit.noteOn.pitch=static_cast<int16>(36+voice%rows);
   hit.noteOn.velocity=1.f;hit.noteOn.noteId=static_cast<int32>(voice);
   check(events.addEvent(hit)==kResultOk,"RE 64 note fixture");
 }
}
void reauditTiming(const std::string& label,std::vector<double> times,double deadline){
 check(!times.empty(),"RE no timing samples");
 const auto misses=std::count_if(times.begin(),times.end(),[&](double ms){return ms>deadline;});
 std::sort(times.begin(),times.end());
 const auto quantile=[&](double q){return times[static_cast<std::size_t>(std::ceil(q*static_cast<double>(times.size())))-1];};
 qa::workflowNotice(label+" n="+std::to_string(times.size())+" p95-ms="+std::to_string(quantile(.95))+
   " p99-ms="+std::to_string(quantile(.99))+" max-ms="+std::to_string(times.back())+
   " deadline-ms="+std::to_string(deadline)+" misses="+std::to_string(misses)+
   "; diagnostic on shared runner, no ASIO guarantee");
}
void testReauditLoadedSwap(){
 using Clock=std::chrono::steady_clock;
 FinalAuditFiles files;Fixture f;KitState kit;kit.visible=2;kit.grid.fill(0);
 kit.names[0]="old.wav";kit.names[1]="fixed.wav";
 kit.wav[0]=reauditLongStereo(.25f,-.5f,48000);kit.wav[1]=reauditLongStereo(.125f,-.25f,48000);
 kit.pitch[0]=48;kit.pitch[1]=48;
 const auto oldPath=files.write("old.wav",kit.wav[0]);
 const auto newPath=files.write("new.wav",reauditLongStereo(-.375f,.125f,48000));
 f.kit(kit);prepareAudio(*f.p);
 std::atomic<unsigned> blocks{0},errors{0},badPcm{0},allocations{0},duringImport{0};
 std::atomic<bool> loading{false},ready{false};
 std::vector<double> times;times.reserve(500);
 std::exception_ptr audioException;
 std::jthread actor([&](std::stop_token stop){
  try {
   AuditAudioBlock audio;EventList notes(64);reaudit64Notes(notes,2);
   audio.data.numSamples=64;
   constexpr auto playing=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
   double ppq=0.;auto target=Clock::now();
   for(unsigned block=0;block<500&&!stop.stop_requested();++block){
     // Explicit barrier OUTSIDE process: no claim that this delay meets ASIO deadlines.
     if(block==200){
       while(!ready.load()&&!stop.stop_requested())std::this_thread::yield();
       if(stop.stop_requested())break;
       ppq=0.;
     }
     audio.clock.state=playing;audio.clock.projectTimeMusic=ppq;
     audio.data.inputEvents=(block==0||block==200||block==351)?&notes:nullptr;
     if(block==350){
       audio.data.numSamples=0;audio.clock.state=0;audio.data.inputEvents=nullptr;
       if(f.p->process(audio.data)!=kResultOk)++errors;
       audio.data.numSamples=64;audio.clock.state=playing;
     }
     const auto allocationsBefore=allocationCount;
     const auto begin=Clock::now();const auto result=f.p->process(audio.data);const auto end=Clock::now();
     times.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
     if(allocationCount!=allocationsBefore)++allocations;
     if(loading.load())++duringImport;
     if(result!=kResultOk)++errors;
     const float expectedLeft=block==350?0.f:block<200?12.f:-8.f;
     const float expectedRight=block==350?0.f:block<200?-24.f:-4.f;
     for(std::size_t sample=0;sample<64;++sample){
       if(!std::isfinite(audio.left[sample])||!std::isfinite(audio.right[sample]))++badPcm;
       // Sinc boundary transient excluded explicitly; RE08 independently checks every unity-rate sample.
       if((sample>=20||block==350)&&
          (std::abs(audio.left[sample]-expectedLeft)>2e-4f||std::abs(audio.right[sample]-expectedRight)>2e-4f))++badPcm;
     }
     if(block!=350&&IntegrationAccess::activeAudioVoices(*f.p)!=64)++errors;
     if(block==350&&IntegrationAccess::activeAudioVoices(*f.p)!=0)++errors;
     ppq+=64./24000.;++blocks;
     target+=std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(64./48000.));
     std::this_thread::sleep_until(target);
   }
  }catch(...) {
    audioException=std::current_exception();
    ++errors;
    ready.store(true,std::memory_order_release);
  }
 });
 while(blocks.load()<4&&errors.load()==0)std::this_thread::yield();
 for(unsigned swap=0;swap<12;++swap){
   loading=true;
   if(f.c->loadWav(0,(swap&1)?newPath:oldPath)!=Controller::LoadStatus::Accepted)++errors;
   loading=false;
 }
 ready=true;actor.join();
 if(audioException) {
   try {std::rethrow_exception(audioException);}
   catch(const std::exception& ex) {throw std::runtime_error(std::string("RE09 audio-thread exception: ")+ex.what());}
   catch(...) {throw std::runtime_error("RE09 audio-thread nonstandard exception");}
 }
 reauditTiming("RE09 48k/64, 64 voices ratio4, 12 actual 2.4MB imports",times,64./48000.*1000.);
 qa::workflowNotice("RE09 blocks="+std::to_string(blocks.load())+" callbacks-during-import="+
    std::to_string(duringImport.load())+" PCM-errors="+std::to_string(badPcm.load())+
    " audio-new-calls="+std::to_string(allocations.load())+" errors="+std::to_string(errors.load()));
 const double blockDeadlineMs=64.0/48000.0*1000.0;
 const auto actualDeadlineMisses=std::count_if(times.begin(),times.end(),
     [&](double elapsed){return elapsed>blockDeadlineMs;});
 qa::workflowNotice("RE09 paced 64-voice/12-import deadline-misses="+
     std::to_string(actualDeadlineMisses)+"/"+std::to_string(times.size())+
     " at 1.333ms; shared runner is NOT ASIO release evidence");
 f.p->setProcessing(false);f.p->setActive(false);
 check(blocks==500u&&duringImport>0u&&badPcm==0u&&errors==0u&&allocations==0u,
       "RE09 old asset lifetime, audible replacement, Stop/locate or realtime allocation failure");
 check(actualDeadlineMisses==0,
       "RE09 paced 64-voice live imports exceeded 48k/64 audio deadline");
}
void testReauditDeadlineMatrix(){
 using Clock=std::chrono::steady_clock;
 for(std::uint32_t rate:{44100u,48000u})for(std::uint32_t ratio:{4u,16u}){
   Fixture f;KitState kit;kit.visible=16;kit.grid.fill(0);kit.pitch.fill(48);
   const auto wav=reauditLongStereo(.125f,-.25f,rate*(ratio/4));
   for(std::size_t row=0;row<kRows;++row){kit.names[row]="load"+std::to_string(row)+".wav";kit.wav[row]=wav;}
   f.kit(kit);
   std::array<SpeakerArrangement,kRows> arrangements{};arrangements.fill(SpeakerArr::kStereo);
   check(f.p->setBusArrangements(nullptr,0,arrangements.data(),static_cast<int32>(kRows))==kResultOk,"RE10 output fixture");
   ProcessSetup setup{};setup.sampleRate=rate;setup.maxSamplesPerBlock=512;
   setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
   check(f.p->setupProcessing(setup)==kResultOk&&f.p->setActive(true)==kResultOk,"RE10 setup");
   for(int32 frames:{64,128,256,512}){
     check(f.p->setProcessing(true)==kResultOk,"RE10 start");
     std::array<float,512> left{},right{};float* channels[2]{left.data(),right.data()};
     AudioBusBuffers output{};output.numChannels=2;output.channelBuffers32=channels;
     ProcessData data{};data.symbolicSampleSize=kSample32;data.numSamples=frames;data.numOutputs=1;data.outputs=&output;
     EventList notes(64);reaudit64Notes(notes,16);
     std::vector<double> times;times.reserve(32);unsigned badPcm=0,allocations=0;
     for(unsigned block=0;block<32;++block){
       data.inputEvents=block==0?&notes:nullptr;
       const auto allocationsBefore=allocationCount;
       const auto begin=Clock::now();const auto result=f.p->process(data);const auto end=Clock::now();
       times.push_back(std::chrono::duration<double,std::milli>(end-begin).count());
       if(allocationCount!=allocationsBefore)++allocations;
       check(result==kResultOk&&IntegrationAccess::activeAudioVoices(*f.p)==64,"RE10 64 active voice precondition");
       for(int32 sample=20;sample<frames;++sample)
         if(std::abs(left[static_cast<std::size_t>(sample)]-8.f)>2e-4f||
            std::abs(right[static_cast<std::size_t>(sample)]+16.f)>2e-4f)++badPcm;
     }
     reauditTiming("RE10 rate="+std::to_string(rate)+" block="+std::to_string(frames)+
                   " voices=64 ratio="+std::to_string(ratio),times,static_cast<double>(frames)/rate*1000.);
     check(badPcm==0&&allocations==0,"RE10 PCM or C++ allocation failure");
     check(f.p->setProcessing(false)==kResultOk,"RE10 stop");
   }
   f.p->setActive(false);
 }
}
// Run in a child process: the unmodified cold Song path may raise native
// EXCEPTION_INT_DIVIDE_BY_ZERO. Do not replace or guard the product modulo.
void testReauditSongCold(bool warm){
 Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
 kit.names[0]="cold.wav";kit.wav[0]=constantStereo(.5f,-.25f);
 kit.patternLengths[0]=16;kit.patternLengths[1]=32;
 kit.grid[0]=100;kit.grid[kRows*kSteps]=70;
 kit.songEnabled=1;kit.chain.setCount(1);kit.chain.configure(0,0,32);
 f.kit(kit);prepareAudio(*f.p);
 check(f.c->setParamNormalized(kPatternParam,1./7.)==kResultOk&&f.c->copyCurrentPattern()&&
       f.c->setParamNormalized(kPatternParam,0.)==kResultOk,"RE11 source clipboard");
 AuditAudioBlock audio;
 audio.clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 if(warm)for(unsigned block=0;block<375;++block){
   audio.clock.projectTimeMusic=static_cast<double>(block*256u)/24000.;
   check(f.p->process(audio.data)==kResultOk,"RE11 warm control");
 }
 audio.clock.projectTimeMusic=4.;arm(qa::patternMidWrite);
 bool reached=false;tresult result=kResultFalse;
 std::jthread reader([&]{
   reached=WaitForSingleObject(entered,10000)==WAIT_OBJECT_0;
   if(reached)result=f.p->process(audio.data);
   releaseProbe();
 });
 const bool pasted=f.c->pasteCurrentPattern();reader.join();
 check(reached&&pasted&&result==kResultOk&&!probeTimeout,"RE11 first Song concurrent paste failed");
 f.p->setProcessing(false);f.p->setActive(false);
}
void testReauditSongColdChild(const char* resource){
 auto child=[&](bool warm){
   std::array<wchar_t,32768> executable{};
   const auto length=GetModuleFileNameW(nullptr,executable.data(),static_cast<DWORD>(executable.size()));
   check(length>0&&length<executable.size(),"RE11 executable path");
   std::wstring command=L"\""+std::wstring(executable.data(),length)+L"\" \""+
       pathFromUtf8(resource).wstring()+L"\" "+(warm?L"--reaudit-song-warm":L"--reaudit-song-cold");
   std::vector<wchar_t> arguments(command.begin(),command.end());arguments.push_back(0);
   STARTUPINFOW startup{};startup.cb=static_cast<DWORD>(sizeof(startup));PROCESS_INFORMATION process{};
   check(CreateProcessW(nullptr,arguments.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
                        nullptr,nullptr,&startup,&process)!=FALSE,"RE11 child creation");
   const auto waited=WaitForSingleObject(process.hProcess,30000);DWORD result=0;
   if(waited!=WAIT_OBJECT_0)TerminateProcess(process.hProcess,2);
   const bool got=GetExitCodeProcess(process.hProcess,&result)!=FALSE;
   CloseHandle(process.hThread);CloseHandle(process.hProcess);
   check(waited==WAIT_OBJECT_0&&got,"RE11 child timeout/status failure");
   qa::workflowNotice(std::string("RE11 ")+(warm?"WARM":"COLD")+" Song/paste child exit="+
       std::to_string(result)+(result==EXCEPTION_INT_DIVIDE_BY_ZERO?" (EXCEPTION_INT_DIVIDE_BY_ZERO)":""));
   return result;
 };
 check(child(true)==0u,"RE11 warm child failed; cold inference invalid");
 check(child(false)==0u,"RE11 first Song callback crashes during concurrent PatternReplace");
}
