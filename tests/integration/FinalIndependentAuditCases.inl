// Independent audit additions. Actual product classes; no production repairs.
// The two adversarial cases deliberately fail if the defects are present.
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
void testFinalAuditPatternLengthRace(){
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
void testFinalAuditZeroFrameStop(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.visible=1;
 kit.names[0]="zero-stop.wav";kit.wav[0]=constantStereo(.25f,-.5f);
 f.kit(kit);prepareAudio(*f.p);
 AuditAudioBlock audio;
 constexpr auto playing=ProcessContext::kPlaying|ProcessContext::kTempoValid|
                        ProcessContext::kProjectTimeMusicValid;
 audio.clock.state=playing;audio.clock.projectTimeMusic=4.;
 EventList notes;Event hit{};hit.type=Event::kNoteOnEvent;hit.noteOn.pitch=36;
 hit.noteOn.velocity=1.f;hit.noteOn.noteId=-1;
 check(notes.addEvent(hit)==kResultOk,"VFA05 trigger fixture");
 audio.data.inputEvents=&notes;
 check(f.p->process(audio.data)==kResultOk&&IntegrationAccess::activeAudioVoices(*f.p)==1,
       "VFA05 initial actual voice");
 audio.data.inputEvents=nullptr;audio.data.numSamples=0;
 audio.clock.state=0;audio.clock.projectTimeMusic=4.+256./24000.;
 check(f.p->process(audio.data)==kResultOk,"VFA05 zero-frame stopped callback");
 const auto afterStop=IntegrationAccess::activeAudioVoices(*f.p);
 audio.data.numSamples=256;audio.clock.state=playing;
 check(f.p->process(audio.data)==kResultOk,"VFA05 continuous-PPQ restart");
 const float resumedL=audio.left[80],resumedR=audio.right[80];
 // Same transport-stop contract, ordinary full block: existing policy MUST
 // retire the voice. This control distinguishes stale audio from a new hit.
 audio.clock.state=0;
 check(f.p->process(audio.data)==kResultOk&&IntegrationAccess::activeAudioVoices(*f.p)==0,
       "VFA05 full-block Stop control failed");
 audio.clock.state=playing;
 check(f.p->process(audio.data)==kResultOk&&audio.left[80]==0.f&&audio.right[80]==0.f,
       "VFA05 empty pattern should not generate a replacement hit");
 qa::workflowNotice("VFA05 zero-frame Stop active voices="+std::to_string(afterStop)+
   "; restart L/R="+std::to_string(resumedL)+","+std::to_string(resumedR)+
   "; full-block Stop control clears voice and restarts silent");
 f.p->setProcessing(false);f.p->setActive(false);
 check(afterStop==0u&&resumedL==0.f&&resumedR==0.f,
       "VFA05 FAIL: zero-frame transport Stop ignored; old voice survives continuous-PPQ restart");
}
void testFinalAuditPeerCancel(){
 Fixture f;KitState kit;kit.grid.fill(0);kit.grid[0]=60;f.kit(kit);
 f.open();f.open();
 auto* first=IntegrationAccess::view(*f.c,0);auto* peer=IntegrationAccess::view(*f.c,1);
 click(*first,1248.,168.); // Expand only the first view's Velocity lane.
 click(*first,630.,214.);
 check(std::get<0>(IntegrationAccess::viewSnapshot(*first)),"VFA03 real velocity gesture absent");
 click(*peer,630.,108.+94.+60.);
 check(f.c->velocity(0,1,0)==100&&auditReturnedKit(*f.p).grid[kSteps]==100,
       "VFA03 peer click never committed its own step");
 first->onMouseCancel();
 const auto retained=f.c->velocity(0,1,0);
 qa::workflowNotice("VFA03 peer step before Cancel=100, after="+std::to_string(retained));
 check(f.c->velocity(0,0,0)==60,"VFA03 cancel did not restore its own velocity");
 check(retained==100&&auditReturnedKit(*f.p).grid[kSteps]==100,
       "VFA03 FAIL: cancel in first real View erased the second View's committed step");
}
std::vector<std::uint8_t> finalAuditLargeWav(std::uint32_t sourceRate){
 // 16 MiB minus at most seven bytes: near the accepted single-file boundary.
 const std::size_t frames=(kMaxSingleWav-44u)/8u;
 auto bytes=constantStereo(.125f,-.375f);bytes.resize(44u+8u*frames);
 for(std::size_t frame=1024;frame<frames;++frame)
   std::memcpy(bytes.data()+44u+8u*frame,bytes.data()+44u,8u);
 auto put=[&](std::size_t at,std::uint32_t value){for(unsigned n=0;n<4;++n)bytes[at+n]=static_cast<std::uint8_t>(value>>(8u*n));};
 put(4,static_cast<std::uint32_t>(bytes.size()-8u));put(24,sourceRate);put(28,sourceRate*8u);
 put(40,static_cast<std::uint32_t>(frames*8u));return bytes;
}
void testFinalAuditLiveImport(){
 using Timer=std::chrono::steady_clock;
 FinalAuditFiles files;
 const auto path44=files.write("large44.wav",finalAuditLargeWav(44100));
 const auto path48=files.write("large48.wav",finalAuditLargeWav(48000));
 const auto bad=files.write("bad.wav",std::vector<std::uint8_t>(128,0));
 for(double outputRate:{44100.,48000.})for(int semitones:{0,12}){
   Fixture f,reference;KitState kit;kit.grid.fill(0);kit.visible=2;kit.choke[1]=1;
   kit.wav[1]=constantStereo(.25f,-.5f);kit.names[1]="unaffected.wav";
   f.kit(kit);reference.kit(kit);prepareAudio(*f.p);prepareAudio(*reference.p);
   if(outputRate!=48000.){
     ProcessSetup setup{};setup.sampleRate=outputRate;setup.maxSamplesPerBlock=256;
     setup.symbolicSampleSize=kSample32;setup.processMode=kRealtime;
     f.p->setProcessing(false);f.p->setActive(false);
     reference.p->setProcessing(false);reference.p->setActive(false);
     check(f.p->setupProcessing(setup)==kResultOk&&reference.p->setupProcessing(setup)==kResultOk,
           "VFA04 44.1k setup");
     f.p->setActive(true);f.p->setProcessing(true);reference.p->setActive(true);reference.p->setProcessing(true);
   }
   check(f.c->adjustRowPitch(0,semitones),"VFA04 actual row pitch edit");
   std::atomic<unsigned> blocks{0},mismatches{0},errors{0},callbackAllocations{0};
   std::vector<double> times;times.reserve(50000);
   std::jthread audioActor([&](std::stop_token stop){
     AuditAudioBlock observed,expected;
     EventList notes;Event note{};note.type=Event::kNoteOnEvent;note.noteOn.pitch=37;
     note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
     if(notes.addEvent(note)!=kResultOk){++errors;return;}
     observed.data.inputEvents=&notes;expected.data.inputEvents=&notes;
     auto target=Timer::now();
     while(!stop.stop_requested()&&blocks.load()<50000u){
       const auto frames=std::array<int32,3>{64,128,256}[blocks.load()%3u];
       observed.data.numSamples=frames;expected.data.numSamples=frames;
       const auto allocationsBefore=allocationCount;
       const auto start=Timer::now();const auto result=f.p->process(observed.data);
       const auto end=Timer::now();
       callbackAllocations.fetch_add(static_cast<unsigned>(allocationCount-allocationsBefore));
       if(result!=kResultOk||reference.p->process(expected.data)!=kResultOk)++errors;
       times.push_back(std::chrono::duration<double,std::milli>(end-start).count());
       for(int32 frame=0;frame<frames;++frame)
         if(observed.left[static_cast<std::size_t>(frame)]!=expected.left[static_cast<std::size_t>(frame)]||
            observed.right[static_cast<std::size_t>(frame)]!=expected.right[static_cast<std::size_t>(frame)])++mismatches;
       // Oracle negative: dropping even one reference sample must be visible.
       if(expected.left[20]==0.f||expected.right[20]==0.f)++errors;
       ++blocks;
       target+=std::chrono::duration_cast<Timer::duration>(std::chrono::duration<double>(frames/outputRate));
       std::this_thread::sleep_until(target); // Synthetic pacing, not ASIO.
     }
   });
   while(blocks.load()<2u&&errors.load()==0u)std::this_thread::yield();
   std::vector<double> imports;
   for(const auto& path:{path44,path48}){
     const auto start=Timer::now();const auto status=f.c->loadWav(0,path);
     imports.push_back(std::chrono::duration<double,std::milli>(Timer::now()-start).count());
     check(status==Controller::LoadStatus::Accepted,"VFA04 near-limit real file load rejected");
   }
   const auto oldPaths=IntegrationAccess::paths(*f.c);
   check(f.c->loadWav(0,bad)!=Controller::LoadStatus::Accepted&&
         IntegrationAccess::paths(*f.c)==oldPaths,"VFA04 bad replacement changed origin");
   audioActor.request_stop();audioActor.join();
   check(!times.empty(),"VFA04 no actual concurrent callbacks");
   std::sort(times.begin(),times.end());
   auto percentile=[&](double fraction){return times[static_cast<std::size_t>(std::ceil(fraction*static_cast<double>(times.size())))-1u];};
   qa::workflowNotice("VFA04 rate="+std::to_string(outputRate)+" pitch="+std::to_string(semitones)+
     " full 16MiB imports ms="+std::to_string(imports[0])+","+std::to_string(imports[1])+
     "; callbacks="+std::to_string(blocks.load())+" PCM mismatches="+std::to_string(mismatches.load())+
     " C++ new calls="+std::to_string(callbackAllocations.load())+" process p95/p99/max ms="+
     std::to_string(percentile(.95))+"/"+std::to_string(percentile(.99))+"/"+std::to_string(times.back())+
     "; synthetic paced native EXE, NOT Studio One/ASIO; unaffected row only");
   f.p->setProcessing(false);f.p->setActive(false);reference.p->setProcessing(false);reference.p->setActive(false);
   check(blocks.load()>2u&&mismatches.load()==0u&&errors.load()==0u&&callbackAllocations.load()==0u,
         "VFA04 concurrent import disrupted actual audio or allocated in callback");
 }
}
