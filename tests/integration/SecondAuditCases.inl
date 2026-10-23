// Uses the same real Windows components and public event/state APIs as the
// original integration fixture. No substitute Controller or voice model.
void testSecondAuditViewInvalidation() {
 for(double userZoom:{1.,1.5}) {
 Fixture f;auto kit=namedKit('X');kit.wav[0]=constantStereo(.25f,-.5f);
 f.kit(kit);prepareAudio(*f.p);f.c->rememberZoom(userZoom);f.open();f.open();
 auto* first=IntegrationAccess::view(*f.c,0);
 auto* other=IntegrationAccess::view(*f.c,1);
 auto nativeWindow=[&](std::size_t index) {
   auto* platform=f.editors[index]->getFrame()->getPlatformFrame();
   check(platform!=nullptr,"SA01 native platform frame missing");
   auto window=static_cast<HWND>(platform->getPlatformRepresentation());
   check(IsWindow(window),"SA01 platform representation is not an HWND");
   return window;
 };
 const auto originalWindow=nativeWindow(0),otherWindow=nativeWindow(1);
 pump();ValidateRect(originalWindow,nullptr);ValidateRect(otherWindow,nullptr);
 check(!GetUpdateRect(otherWindow,nullptr,FALSE),"SA01 dirty initial fixture");
 other->invalid();
 check(GetUpdateRect(otherWindow,nullptr,FALSE),"SA01 invalidation oracle misses real repaint request");
 ValidateRect(otherWindow,nullptr);
 auto bothInvalidated=[&](const char* action,const auto& edit) {
   ValidateRect(originalWindow,nullptr);ValidateRect(otherWindow,nullptr);
   if(!edit())throw std::runtime_error(std::string("SA01 actual accepted edit failed: ")+action);
   check(GetUpdateRect(originalWindow,nullptr,FALSE)&&GetUpdateRect(otherWindow,nullptr,FALSE),
         "SA01 accepted row/global edit did not invalidate BOTH native editors");
 };
 // This path includes the real first View's click handler, not just a direct
 // Controller call. Its local invalid() must not conceal the stale other HWND.
 bothInvalidated("MUTE VIA VIEW",[&]{click(*first,475.,126.);return f.c->isRowMuted(0);});
 bothInvalidated("SOLO CONTROLLER",[&]{return f.c->toggleRowGate(0,true);});
 bothInvalidated("ROW VOL",[&]{return f.c->adjustRowMix(0,false,1);});
 bothInvalidated("ROW PAN",[&]{return f.c->adjustRowMix(0,true,1);});
 bothInvalidated("ROW PITCH",[&]{return f.c->adjustRowPitch(0,1);});
 bothInvalidated("SWING",[&]{return f.c->adjustSwing(1);});
 bothInvalidated("HUMANIZE",[&]{return f.c->adjustHumanize(1);});
 bothInvalidated("CLEAR SAMPLE",[&]{return f.c->clearSample(0);});
 const auto path=std::filesystem::current_path()/"125A-second-audit-load.wav";
 struct Cleanup {std::filesystem::path file;~Cleanup(){std::error_code ec;std::filesystem::remove(file,ec);}} cleanup{path};
 const auto wav=constantStereo(.125f,-.25f);
 {std::ofstream file(path,std::ios::binary|std::ios::trunc);
  file.write(reinterpret_cast<const char*>(wav.data()),static_cast<std::streamsize>(wav.size()));
  check(file.good(),"SA01 actual sample file fixture");}
 const auto pathText=pathToUtf8(path);
 bothInvalidated("LOAD SAMPLE",[&]{return f.c->loadWav(0,pathText)==Controller::LoadStatus::Accepted;});
 // Start with a short editor. A change in the shared count must leave every
 // row reachable in the other editor, without unsolicited resize requests.
 auto one=kit;one.visible=1;
 // namedKit deliberately gives even unused slots nonempty custom names.
 // Removal is correctly forbidden until the unused slots are actually empty.
 for(std::size_t row=1;row<kRows;++row){one.names[row].clear();one.displayNames[row].clear();}
 f.kit(one);
 const auto resizeCalls=f.frames[0].calls+f.frames[1].calls;
 bothInvalidated("ADD FIRST ROW",[&]{return f.c->addRow();});
 bothInvalidated("ADD SECOND ROW",[&]{return f.c->addRow();});
 check(f.c->visibleRows()==3&&other->visibleRowsForHit()==2&&
       f.frames[0].calls+f.frames[1].calls==resizeCalls,
       "SA01 peer editor clipped added rows or issued an extra host resize");
 click(*other,1180.,108.+2.*94.+15.);
 check(IntegrationAccess::firstRow(*other)==2&&other->visibleRowsForHit()==1,
       "SA01 newly added row inaccessible in peer compact page");
 bothInvalidated("HIDE LAST ROW",[&]{return f.c->hideLastEmptyRow();});
 check(f.c->visibleRows()==2&&IntegrationAccess::firstRow(*other)<2,
       "SA01 peer page invalid after row removal");
 bothInvalidated("RESTORE ROW COUNT",[&]{return f.c->restoreVisibleRows(1);});
 check(other->visibleRowsForHit()==1,"SA01 row rollback left a stale viewport");
 auto prior=IntegrationAccess::snapshot(*f.c);
 ValidateRect(originalWindow,nullptr);ValidateRect(otherWindow,nullptr);
 check(!f.c->adjustRowMix(kRows,false,1)&&IntegrationAccess::snapshot(*f.c)==prior&&
       !GetUpdateRect(originalWindow,nullptr,FALSE)&&!GetUpdateRect(otherWindow,nullptr,FALSE),
       "SA01 rejected edit changed or invalidated peer state");
 f.p->setProcessing(false);f.p->setActive(false);
 }
 evidence("SA01 100/150 actual view M click, row/global/sample edits, count paging/rollback and rejected edit: both native HWNDs PASS");
}
void testSecondAuditTriggerPartition() {
 // A dense sequencer downbeat plus MIDI on the same row is a normal merger
 // boundary. A later event must not alter the order of simultaneous chokes.
 Fixture f;KitState kit;kit.visible=16;kit.grid.fill(0);
 kit.names[0]="SA02-choke.wav";kit.wav[0]=constantStereo(.25f,-.5f);
 kit.choke[0]=1;
 for(std::size_t row=0;row<kRows;++row)kit.grid[row*kSteps]=64;
 f.kit(kit);prepareAudio(*f.p);
 const auto bytes=Fixture::encode(kit);
 struct OrderingReset {~OrderingReset(){qa::legacyTriggerOrdering=false;}} orderingReset;
 unsigned legacyDivergences=0;
 std::string divergentDensities;
 for(unsigned simultaneous:{1u,8u,15u,16u,17u,20u}) {
  for(bool legacy:{true,false}) {
   qa::legacyTriggerOrdering=legacy;
   std::array<float,256> wholeL{},wholeR{},splitL{},splitR{};
   auto render=[&](bool split,auto& leftSamples,auto& rightSamples) {
     qa::ByteStream state(const_cast<std::uint8_t*>(bytes.data()),bytes.size());
     check(f.p->setState(&state)==kResultOk,"SA02 fresh actual kit recall");
     EventList early,all,late;
     for(unsigned n=0;n<simultaneous;++n) {
       Event hit{};hit.type=Event::kNoteOnEvent;hit.noteOn.pitch=36;
       hit.noteOn.velocity=static_cast<float>(40+n*3)/127.f;hit.noteOn.noteId=-1;
       check(early.addEvent(hit)==kResultOk&&all.addEvent(hit)==kResultOk,"SA02 early MIDI fixture");
     }
     Event tail{};tail.type=Event::kNoteOnEvent;tail.noteOn.pitch=51;
     tail.noteOn.velocity=1.f;tail.noteOn.noteId=-1;tail.sampleOffset=200;
     check(all.addEvent(tail)==kResultOk,"SA02 future event fixture");
     tail.sampleOffset=72;check(late.addEvent(tail)==kResultOk,"SA02 split event fixture");
     float* pointers[]{leftSamples.data(),rightSamples.data()};
     AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=pointers;
     ProcessContext clock{};clock.sampleRate=48000.;clock.tempo=120.;
     clock.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
     ProcessData block{};block.symbolicSampleSize=kSample32;block.numSamples=split?128:256;
     block.numOutputs=1;block.outputs=&bus;block.processContext=&clock;
     block.inputEvents=split?&early:&all;
     check(f.p->process(block)==kResultOk,"SA02 first actual render");
     if(split) {
       pointers[0]=leftSamples.data()+128;pointers[1]=rightSamples.data()+128;
       clock.projectTimeSamples=128;clock.projectTimeMusic=128.*120./(60.*48000.);
       block.inputEvents=&late;
       check(f.p->process(block)==kResultOk,"SA02 second actual render");
     }
   };
   render(false,wholeL,wholeR);render(true,splitL,splitR);
   const bool equal=wholeL==splitL&&wholeR==splitR;
   if(legacy) {
     if(!equal){++legacyDivergences;divergentDensities+=std::to_string(simultaneous)+" ";}
   }else {
     check(equal,"SA02 same absolute sequencer/MIDI/choke events produce different audio at 256 versus 128+128");
     const double velocity=static_cast<double>(40+(simultaneous-1)*3)/127.;
     check(std::abs(splitL[180]-.25*velocity)<2e-7&&
           std::abs(splitR[180]+.5*velocity)<2e-7,
           "SA02 final simultaneous MIDI voice does not match independent velocity/stereo code");
   }
   check(std::abs(splitL[180])>.01f,"SA02 partition comparison is vacuously silent");
  }
 }
 check(legacyDivergences>0,"SA02 original-comparator negative control did not reproduce block-dependent PCM");
 qa::legacyTriggerOrdering=false;
 f.p->setProcessing(false);f.p->setActive(false);
 evidence("SA02 real Processor exact 256/128+128 PCM at six densities; original-comparator negatives="+
          std::to_string(legacyDivergences)+" PASS");
 qa::workflowNotice("SA02 actual Processor PCM PASS; original-comparator negative divergences="+
     std::to_string(legacyDivergences)+"; simultaneous MIDI counts="+divergentDensities);
}
