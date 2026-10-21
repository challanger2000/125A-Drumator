// Included inside the real Windows integration fixture's anonymous namespace.
// These are real Controller/View/Processor calls, not replacement state models.
KitState auditReturnedKit(Processor& processor) {
 ResizableMemoryIBStream stream;KitState state;
 check(processor.getState(&stream)==kResultOk&&
       KitCodec::decode(static_cast<const std::uint8_t*>(stream.getData()),
                        static_cast<std::size_t>(stream.getCursor()),state),
       "Audit actual processor state decode");
 return state;
}
struct AuditAudioBlock {
 std::array<float,256> left{},right{};
 float* pointers[2]{left.data(),right.data()};
 AudioBusBuffers output{};
 ProcessContext clock{};
 ProcessData data{};
 AuditAudioBlock() {
   output.numChannels=2;output.channelBuffers32=pointers;
   clock.sampleRate=48000.;clock.tempo=120.;
   data.numSamples=256;data.symbolicSampleSize=kSample32;
   data.numOutputs=1;data.outputs=&output;data.processContext=&clock;
 }
};
void auditAddPatternNote(EventList& list,unsigned pattern,int32 offset) {
 Event note{};note.type=Event::kNoteOnEvent;note.sampleOffset=offset;
 note.noteOn.channel=0;note.noteOn.pitch=static_cast<int16>(24+pattern);
 note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
 check(list.addEvent(note)==kResultOk,"FA02 MIDI fixture append");
}
void testAuditMidiRecall() {
 for(bool busy:{false,true})for(int scenario=0;scenario<8;++scenario) {
   Fixture f;auto original=namedKit('X');original.selected=2;
   original.songEnabled=scenario>=6?1:0;
   f.kit(original);prepareAudio(*f.p);
   auto recalled=namedKit('Y');recalled.selected=6;
   recalled.songEnabled=original.songEnabled;
   auto bytes=Fixture::encode(recalled);
   arm(busy?qa::restoreMidCommit:qa::restoreSnapshot);
   tresult restored=kResultFalse;
   std::jthread restoring([&]{qa::ByteStream stream(bytes.data(),bytes.size());restored=f.p->setState(&stream);});
   waitEntered();
   AuditAudioBlock block;
   EventList notes;ParameterChanges hostValues,feedback;
   int32 queueIndex=0,pointIndex=0;
   auto hostPoint=[&](int32 offset,unsigned pattern){
     check(hostValues.addParameterData(kPatternParam,queueIndex)->addPoint(
         offset,static_cast<double>(pattern)/7.,pointIndex)==kResultOk,"FA02 host fixture append");
   };
   unsigned expected=5;
   if(scenario==1||scenario==2)expected=2;
   if(scenario==3) {
     auditAddPatternNote(notes,1,200); // deliberately unsorted MIDI list
     auditAddPatternNote(notes,5,20);hostPoint(128,7);expected=1;
   }else if(scenario==4) {
     auditAddPatternNote(notes,5,64);hostPoint(64,3);expected=3;
   }else if(scenario==5) {
     auditAddPatternNote(notes,5,1);hostPoint(256,7); // outside half-open block
   }else {
     auditAddPatternNote(notes,scenario==1?2:5,0);
     if(scenario==6)expected=6; // Song mode suppresses MIDI pattern commands
     if(scenario==7){hostPoint(0,3);expected=3;}
   }
   block.data.inputEvents=&notes;
   block.data.inputParameterChanges=&hostValues;
   block.data.outputParameterChanges=&feedback;
   const auto first=f.p->process(block.data);
   tresult second=kResultOk;
   if(scenario==2) {
     notes.clear();auditAddPatternNote(notes,2,0);
     second=f.p->process(block.data); // genuine A -> B -> A across callbacks
   }
   releaseProbe();restoring.join();
   check(first==kResultOk&&second==kResultOk&&restored==kResultOk&&!probeTimeout,
         "FA02 actual restore/process barrier failed");
   check(auditReturnedKit(*f.p).selected==expected,
         "FA02 newer MIDI/host selection lost, reordered, or out-of-block point accepted");
   const bool expectMidiFeedback=scenario!=4&&scenario<6;
   check((feedback.getParameterCount()>0)==expectMidiFeedback,
         "FA02 missing MIDI feedback or spurious feedback for host/Song suppression");
   if(expectMidiFeedback) {
     auto* queue=feedback.getParameterData(0);int32 offset=-1;ParamValue value=-1.;
     check(queue&&queue->getParameterId()==kPatternParam&&queue->getPointCount()>0&&
           queue->getPoint(queue->getPointCount()-1,offset,value)==kResultOk&&
           patternFromNormalized(value)==expected,"FA02 output parameter feedback differs");
   }
   const auto saved=Fixture::encode(auditReturnedKit(*f.p));
   check(f.recall(saved)==kResultOk&&f.c->currentPattern()==expected,
         "FA02 controller recall disagrees with processor");
   String128 label{};
   check(f.c->getParamStringByValue(kPatternParam,f.c->getParamNormalized(kPatternParam),label)==kResultOk&&
         label[0]==static_cast<TChar>('A'+expected),"FA02 host label disagrees with MIDI result");
   f.p->setProcessing(false);f.p->setActive(false);
 }
 evidence("FA02 16 real restore interleavings: MIDI equal/ABA/order, host ties, invalid offsets, feedback and Song suppression PASS");
}
void testAuditSongGeneration() {
 Fixture f;KitState previous;previous.visible=1;previous.grid.fill(0);
 previous.grid[0]=16;previous.names[0]="song-generation.wav";
 previous.wav[0]=constantStereo(.4f,-.2f);
 f.kit(previous);prepareAudio(*f.p);
 AuditAudioBlock block;
 block.clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 check(f.p->process(block.data)==kResultOk,"FA03 prime old-kit Song cache");
 auto next=previous;next.songEnabled=1;
 check(next.chain.setCount(1)&&next.chain.configure(0,1,32),"FA03 new Song chain fixture");
 next.grid[kRows*kSteps]=127;
 f.kit(next); // no audio callback has refreshed the new kit's cached chain yet
 HostMessage config;config.setMessageID("Drumator.SongConfig");
 const std::array<std::uint8_t,2> entry{{1,32}};
 auto* attributes=config.getAttributes();
 check(attributes&&attributes->setInt("Enabled",1)==kResultOk&&
       attributes->setInt("Count",1)==kResultOk&&
       attributes->setBinary("Entries",entry.data(),static_cast<uint32>(entry.size()))==kResultOk,
       "FA03 Song message fixture");
 arm(qa::songMidPublish);
 tresult rendered=kResultFalse;
 // UI thread publishes an unchanged Y chain while audio encounters an odd
 // mailbox sequence. Any fallback MUST also belong to Y, not previous kit X.
 std::jthread audioActor([&]{
   if(WaitForSingleObject(entered,10000)==WAIT_OBJECT_0)rendered=f.p->process(block.data);
   else probeTimeout=true;
   releaseProbe();
 });
 const auto notified=f.p->notify(&config);audioActor.join();
 check(notified==kResultOk&&rendered==kResultOk&&!probeTimeout,
       "FA03 controlled Song publication failed");
 for(std::size_t frame=80;frame<180;++frame)
   check(std::abs(block.left[frame]-.4f)<2e-4&&std::abs(block.right[frame]+.2f)<2e-4,
         "FA03 new-kit audio used old-kit Song fallback");
 check(std::abs(.4-.4*16./127.)>2e-4,"FA03 old/new waveform codes must differ");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("FA03 real UI publication overlaps first post-recall render: new-kit chain and independent stereo code PASS");
}
void testAuditZeroFlushOffsets() {
 for(bool busy:{false,true}) {
   Fixture f;auto original=namedKit('X');original.selected=2;
   f.kit(original);prepareAudio(*f.p);
   auto next=namedKit('Y');next.selected=6;auto bytes=Fixture::encode(next);
   arm(busy?qa::restoreMidCommit:qa::restoreSnapshot);
   tresult restored=kResultFalse;
   std::jthread restoring([&]{qa::ByteStream stream(bytes.data(),bytes.size());restored=f.p->setState(&stream);});
   waitEntered();
   ParameterChanges changes;int32 queueIndex=0,pointIndex=0;
   auto* queue=changes.addParameterData(kPatternParam,queueIndex);
   check(queue&&queue->addPoint(0,3./7.,pointIndex)==kResultOk&&
         queue->addPoint(1,1.,pointIndex)==kResultOk,"FA02 zero-flush fixture");
   ProcessData flush{};flush.symbolicSampleSize=kSample32;flush.inputParameterChanges=&changes;
   const auto result=f.p->process(flush);releaseProbe();restoring.join();
   check(result==kResultOk&&restored==kResultOk&&!probeTimeout&&auditReturnedKit(*f.p).selected==3,
         "FA02 zero-frame out-of-block point overrode valid offset-zero event");
   f.p->setProcessing(false);f.p->setActive(false);
 }
}
struct AuditFilePackage final:VSTGUI::IDataPackage {
 explicit AuditFilePackage(std::string bytes):bytes_(std::move(bytes)){}
 std::uint32_t getCount() const override{return 1;}
 std::uint32_t getDataSize(std::uint32_t index) const override {
   return index==0?static_cast<std::uint32_t>(bytes_.size()+1):0;
 }
 Type getDataType(std::uint32_t index) const override{return index==0?kFilePath:kError;}
 std::uint32_t getData(std::uint32_t index,const void*& buffer,Type& dataType) const override {
   dataType=getDataType(index);buffer=index==0?bytes_.data():nullptr;return getDataSize(index);
 }
 std::string bytes_;
};
void testAuditSamplePathFailures() {
 Fixture f;auto original=namedKit('X');original.wav[0]=constantStereo(.1f,-.1f);
 f.kit(original);prepareAudio(*f.p);f.open();
 const auto path=std::filesystem::current_path()/"125A-Drumator-functional-audit-long-path-sample.wav";
 struct Cleanup {std::filesystem::path file;~Cleanup(){std::error_code ec;std::filesystem::remove(file,ec);}} cleanup{path};
 const auto wav=constantStereo(.25f,-.5f);
 {std::ofstream file(path,std::ios::binary|std::ios::trunc);
  file.write(reinterpret_cast<const char*>(wav.data()),static_cast<std::streamsize>(wav.size()));
  check(file.good(),"FA04 actual sample file fixture");}
 const auto pathText=pathToUtf8(path);
 const auto before=IntegrationAccess::snapshot(*f.c);
 const auto beforeKit=Fixture::encode(auditReturnedKit(*f.p));
 // Negative control: the exact initial path API really allocates and throws;
 // this was outside loadWav's original exception boundary.
 bool pathThrows=false;
 allocationFail=0;
 try {const auto converted=pathFromUtf8(pathText);(void)converted;}
 catch(const std::bad_alloc&){pathThrows=true;}
 allocationFail=-1;
 check(pathThrows,"FA04 fault did not reach allocating path preparation");
 bool escaped=false;auto loaded=Controller::LoadStatus::Accepted;
 allocationFail=0;
 try {loaded=f.c->loadWav(0,pathText);}catch(const std::exception&){escaped=true;}
 allocationFail=-1;
 check(!escaped&&loaded==Controller::LoadStatus::ReadError&&
       IntegrationAccess::snapshot(*f.c)==before&&
       Fixture::encode(auditReturnedKit(*f.p))==beforeKit,
       "FA04 path OOM escaped or changed accepted kit/Undo/path");
 auto* view=IntegrationAccess::view(*f.c,0);
 AuditFilePackage package(pathText);
 VSTGUI::DragEventData drag{};drag.drag=&package;drag.pos={100.,122.};
 escaped=false;
 allocationFail=0;
 try {(void)view->onDragEnter(drag);}catch(const std::exception&){escaped=true;}
 allocationFail=-1;
 check(!escaped&&IntegrationAccess::snapshot(*f.c)==before,
       "FA04 drop path OOM escaped or changed kit");
 std::string malformed="C:\\invalid-";
 malformed.push_back(static_cast<char>(0xc0));malformed.push_back(static_cast<char>(0xaf));malformed+=".wav";
 AuditFilePackage invalid(malformed);drag.drag=&invalid;
 check(view->onDragEnter(drag)==VSTGUI::DragOperation::None&&!view->onDrop(drag)&&
       IntegrationAccess::snapshot(*f.c)==before&&
       Fixture::encode(auditReturnedKit(*f.p))==beforeKit,
       "FA04 malformed UTF-8 drop accepted or damaged kit");
 drag.drag=&package;
 check(view->onDragEnter(drag)==VSTGUI::DragOperation::Copy&&view->onDrop(drag),
       "FA04 real View sample import failed after recovery");
 const auto after=auditReturnedKit(*f.p);
 check(after.wav[0]==wav&&after.grid==original.grid&&
       IntegrationAccess::paths(*f.c)[0]==pathText,"FA04 successful drop lost pattern or path");
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("FA04 actual Controller path-OOM negative, View drop OOM/invalid UTF-8, unchanged prior kit and recovery PASS");
}
void testAuditMenuGutters() {
 for(bool large:{false,true}) {
   Fixture f;auto kit=namedKit('X');f.kit(kit);f.open();
   check(zoom(*f.c,large)==kResultOk,"FA05 zoom fixture");
   auto* view=IntegrationAccess::view(*f.c,0);
   const auto before=IntegrationAccess::snapshot(*f.c);
   const auto rowY=IntegrationAccess::rowTop(*view,0);
   const double fillTop=std::min(rowY+36.,view->getViewSize().getHeight()-230.);
   for(int item=0;item<9;++item) {
     click(*view,425.,rowY+18.);
     click(*view,450.,fillTop+item*25.+24.5);
     check(IntegrationAccess::snapshot(*f.c)==before,
           "FA05 blank FILL/CLEAR gutter edited a pattern or Undo");
   }
   // A real interior CLEAR and Undo must still work.
   click(*view,425.,rowY+18.);click(*view,450.,fillTop+5.*25.+12.);
   for(std::size_t step=0;step<kSteps;++step)
     check(f.c->velocity(0,0,step)==0,"FA05 interior CLEAR did not execute");
   check(f.c->undoCurrentPattern(),"FA05 real CLEAR Undo failed");
   for(std::size_t step=0;step<kSteps;++step)
     check(f.c->velocity(0,0,step)==kit.grid[step],"FA05 CLEAR Undo lost original steps");
   const auto preferred=f.c->preferredZoom();
   for(int item=0;item<4;++item) {
     click(*view,1150.,20.);click(*view,1150.,39.+item*30.+29.5);
     check(f.c->preferredZoom()==preferred&&view->zoomMatches(preferred),
           "FA05 blank Zoom gutter changed host geometry or saved preference");
   }
 }
 evidence("FA05 all 9 FILL and 4 Zoom menu gutters inert at 100/150; interior CLEAR and Undo PASS");
}
void testAuditMinimumHeightMenus() {
 Fixture f;auto kit=namedKit('X');f.kit(kit);
 f.frames[0].accept=false;f.open();
 auto* view=IntegrationAccess::view(*f.c,0);
 const auto height=view->getViewSize().getHeight();
 check(height==300.&&view->visibleRowsForHit()==1,
       "FA06 fixture must reach supported 300px host-refusal viewport");
 const auto top=UiCompactLayout::popupTop(108.,height,230.);
 check(top>=0.&&top+225.<=height,"FA06 complete FILL menu outside viewport");
 // Independent golden first-bar velocities for the nine visible FILL labels.
 const std::array<std::array<std::uint8_t,16>,9> golden{{
   {{127,0,127,0,127,0,127,0,127,0,127,0,127,0,127,0}},
   {{127,0,0,0,127,0,0,0,127,0,0,0,127,0,0,0}},
   {{127,0,0,0,0,0,0,0,127,0,0,0,0,0,0,0}},
   {{0,0,0,0,127,0,0,0,0,0,0,0,127,0,0,0}},
   {{127,127,127,127,127,127,127,127,127,127,127,127,127,127,127,127}},
   {{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}},
   {{0,127,0,127,0,127,0,127,0,127,0,127,0,127,0,127}},
   {{127,0,95,84,127,0,95,84,127,0,95,84,127,0,95,84}},
   {{0,0,0,0,127,0,0,0,0,0,0,42,127,0,0,42}}
 }};
 for(std::size_t item=0;item<golden.size();++item) {
   f.kit(kit);
   click(*view,425.,126.);click(*view,450.,top+static_cast<double>(item)*25.+12.);
   const auto saved=auditReturnedKit(*f.p);
   for(std::size_t step=0;step<kSteps;++step)
     check(saved.grid[step]==golden[item][step%16],"FA06 clipped or misrouted FILL menu item");
 }
 const auto outputTop=UiCompactLayout::popupTop(108.,height,210.);
 check(outputTop>=0.&&outputTop+200.<=height,"FA06 complete output menu outside viewport");
 for(unsigned bus=0;bus<16;++bus) {
   click(*view,550.,126.);
   click(*view,450.+static_cast<double>(bus/8)*102.,outputTop+(bus%8)*25.+12.);
   check(auditReturnedKit(*f.p).output[0]==bus,"FA06 output menu item inaccessible at minimum height");
 }
 evidence("FA06 supported 300px refusal: all 9 golden FILL actions and 16 output choices reachable PASS");
}
