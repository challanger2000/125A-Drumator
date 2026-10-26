// Real packaged VST3 regression paths, included after the WAV fixture helpers.
void auditRestoreKit(IComponent* component,const drumator::KitState& kit) {
 std::vector<std::uint8_t> bytes;
 require(drumator::KitCodec::encode(kit,bytes),"Audit encode valid kit");
 drumator::qa::ByteStream stream(bytes.data(),bytes.size());
 require(component->setState(&stream)==kResultOk,"Audit restore actual packaged kit");
}
void auditRateSwitchClock(IComponent* component,IAudioProcessor* audio,
                         const ProcessSetup& originalSetup) {
 Steinberg::ResizableMemoryIBStream priorState;
 require(component->getState(&priorState)==kResultOk,"T28 retain original packaged state");
 auto configure=[&](double rate) {
   require(audio->setProcessing(false)==kResultOk&&component->setActive(false)==kResultOk,
           "T28 stop/deactivate for real rate transition");
   auto requested=originalSetup;requested.sampleRate=rate;
   require(audio->setupProcessing(requested)==kResultOk&&
           component->setActive(true)==kResultOk&&audio->setProcessing(true)==kResultOk,
           "T28 configure/activate rate transition");
 };
 configure(48000.);
 drumator::KitState fixture;fixture.visible=1;fixture.grid.fill(0);
 fixture.grid[0]=127;fixture.names[0]="T28-independent-stereo.wav";
 fixture.wav[0]=makeStereoWav(.25f,-.5f);
 auditRestoreKit(component,fixture);
 std::array<float,256> fullLeft{},fullRight{},splitLeft{},splitRight{};
 float* channels[2]{fullLeft.data(),fullRight.data()};
 AudioBusBuffers bus{};bus.numChannels=2;bus.channelBuffers32=channels;
 ProcessContext transport{};transport.sampleRate=48000.;transport.tempo=120.;
 transport.state=ProcessContext::kPlaying|ProcessContext::kTempoValid|ProcessContext::kProjectTimeMusicValid;
 ProcessData block{};block.numSamples=256;block.symbolicSampleSize=kSample32;
 block.numOutputs=1;block.outputs=&bus;block.processContext=&transport;
 require(audio->process(block)==kResultOk,"T28 seed previous real transport at PPQ zero");
 configure(44100.);
 transport.sampleRate=44100.;transport.projectTimeMusic=4.;transport.projectTimeSamples=88200;
 require(audio->process(block)==kResultOk,"T28 first new-rate block at bar two");
 const auto matchesCode=[](const auto& leftCode,const auto& rightCode) {
   for(std::size_t frame=80;frame<180;++frame)
     if(!std::isfinite(leftCode[frame])||!std::isfinite(rightCode[frame])||
        std::abs(leftCode[frame]-.25f)>2e-4||std::abs(rightCode[frame]+.5f)>2e-4)return false;
   return true;
 };
 require(matchesCode(fullLeft,fullRight),"T28 bar-two clock did not start saved Step 1 with preserved stereo sample");
 std::array<float,256> silent{};
 require(!matchesCode(silent,silent)&&!matchesCode(fullRight,fullLeft),
         "T28 independent waveform oracle accepts silence or swapped channels");
 // Repeat the same start using 128+128 frames. The oracle is the independently
 // known sample code AND the actual single-block render, not private fields.
 require(audio->setProcessing(false)==kResultOk&&audio->setProcessing(true)==kResultOk,
         "T28 restart for partition equivalence");
 block.numSamples=128;channels[0]=splitLeft.data();channels[1]=splitRight.data();
 require(audio->process(block)==kResultOk,"T28 first split block");
 channels[0]=splitLeft.data()+128;channels[1]=splitRight.data()+128;
 transport.projectTimeMusic=4.+128.*120./(60.*44100.);transport.projectTimeSamples+=128;
 require(audio->process(block)==kResultOk&&matchesCode(splitLeft,splitRight),
         "T28 split transport onset differs");
 for(std::size_t frame=0;frame<256;++frame)
   require(std::abs(fullLeft[frame]-splitLeft[frame])<2e-5&&
           std::abs(fullRight[frame]-splitRight[frame])<2e-5,
           "T28 rendered audio depends on block partition after rate switch");
 Steinberg::ResizableMemoryIBStream returned;drumator::KitState saved;
 require(component->getState(&returned)==kResultOk&&
         drumator::KitCodec::decode(static_cast<const std::uint8_t*>(returned.getData()),
           static_cast<std::size_t>(returned.getCursor()),saved)&&
         saved.wav==fixture.wav&&saved.grid==fixture.grid,
         "T28 sample-rate setup changed sample bytes or pattern");
 configure(originalSetup.sampleRate);
 priorState.rewind();require(component->setState(&priorState)==kResultOk,"T28 restore previous fixture");
 drumator::qa::result("T28 PASS: packaged 48k->44.1k, no-MIDI Step-1 onset at PPQ4, independent stereo code, 256 versus 128+128, sample/state preservation");
}
void auditZeroChannelMain(IComponent* component,IAudioProcessor* audio,double rate) {
 Steinberg::ResizableMemoryIBStream priorState;
 require(component->getState(&priorState)==kResultOk,"T29 retain original kit");
 drumator::KitState fixture;fixture.visible=2;fixture.grid.fill(0);
 // T29 intentionally tests a manually routed AUX with zero-channel MAIN.
 // An unconfigured NEW kit now routes every row to MAIN by design.
 fixture.output[1]=1;
 fixture.names[1]="T29-stereo-signature.wav";fixture.wav[1]=makeStereoWav(.2f,-.4f);
 require(component->activateBus(kAudio,kOutput,1,true)==kResultOk,"T29 activate real AUX1");
 const auto matchesCode=[](const auto& leftCode,const auto& rightCode) {
   for(std::size_t frame=0;frame<256;++frame) {
     if(!std::isfinite(leftCode[frame])||!std::isfinite(rightCode[frame]))return false;
     if(frame<64&&(leftCode[frame]!=0.f||rightCode[frame]!=0.f))return false;
     if(frame>=100&&frame<180&&
        (std::abs(leftCode[frame]-.2f)>2e-4||std::abs(rightCode[frame]+.4f)>2e-4))return false;
   }
   return true;
 };
 for(bool nonnullTable:{false,true}) {
   auditRestoreKit(component,fixture); // removes every previous sounding voice
   std::array<float,256> auxLeft{},auxRight{},guard{};guard.fill(17.5f);
   float* ignoredMain[2]{guard.data(),nullptr};
   float* auxChannels[2]{auxLeft.data(),auxRight.data()};
   std::array<AudioBusBuffers,2> outputs{};
   outputs[0].numChannels=0;outputs[0].channelBuffers32=nonnullTable?ignoredMain:nullptr;
   outputs[1].numChannels=2;outputs[1].channelBuffers32=auxChannels;
   EventList notes;Event note{};note.type=Event::kNoteOnEvent;
   note.sampleOffset=64;note.noteOn.pitch=37;note.noteOn.channel=0;
   note.noteOn.velocity=1.f;note.noteOn.noteId=-1;
   require(notes.addEvent(note)==kResultOk,"T29 fresh sample-offset MIDI event");
   ProcessContext transport{};transport.sampleRate=rate;
   ProcessData block{};block.symbolicSampleSize=kSample32;block.numSamples=256;
   block.numOutputs=2;block.outputs=outputs.data();block.inputEvents=&notes;block.processContext=&transport;
   require(audio->process(block)==kResultOk&&matchesCode(auxLeft,auxRight),
           "T29/FA01 disabled zero-channel MAIN prevented distinct fresh AUX signal");
   require(std::all_of(guard.begin(),guard.end(),[](float value){return value==17.5f;}),
           "FA01 zero-channel MAIN buffer was inspected/rendered as an active channel");
   std::array<float,256> silent{};
   require(!matchesCode(silent,silent)&&!matchesCode(auxRight,auxLeft),
           "T29 oracle accepts silence/stale missing trigger or L/R swap");
   auto staleLeft=auxLeft;staleLeft[0]=.2f;
   require(!matchesCode(staleLeft,auxRight),"T29 oracle accepts a continuing pre-trigger voice");
 }
 priorState.rewind();require(component->setState(&priorState)==kResultOk,"T29 restore previous fixture");
 drumator::qa::result("T29/FA01 PASS: real packaged AUX1 with MAIN zero channels, null/non-null pointer table, untouched MAIN guard, fresh offset64 stereo signature and oracle negatives");
}
