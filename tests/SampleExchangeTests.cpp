#include "core/SampleExchange.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

using namespace drumator;
static void require(bool test,const char* msg) {
    if(!test)throw std::runtime_error(msg);
}
static void u16(std::vector<std::uint8_t>& v,std::uint16_t value) {
    v.push_back(static_cast<std::uint8_t>(value));
    v.push_back(static_cast<std::uint8_t>(value>>8));
}
static void u32(std::vector<std::uint8_t>& v,std::uint32_t value) {
    for(int b=0;b<4;++b)
        v.push_back(static_cast<std::uint8_t>(value>>(8*b)));
}
static void marker(std::vector<std::uint8_t>& v,const char* s) {
    for(int i=0;i<4;++i)v.push_back(static_cast<std::uint8_t>(s[i]));
}
static std::vector<std::uint8_t> pcm16(std::int16_t level,
                                        std::uint32_t frames=2048) {
    std::vector<std::uint8_t> v;
    marker(v,"RIFF");u32(v,36+frames*2);
    marker(v,"WAVE");
    marker(v,"fmt ");u32(v,16);
    u16(v,1);u16(v,1);u32(v,48000);
    u32(v,96000);u16(v,2);u16(v,16);
    marker(v,"data");u32(v,frames*2);
    for(std::uint32_t i=0;i<frames;++i)u16(v,static_cast<std::uint16_t>(level));
    return v;
}
static std::unique_ptr<PreparedSample> makeAsset(
    const std::vector<std::uint8_t>& wav,double rate=48000,
    float pan=-1.f) {
    std::unique_ptr<PreparedSample> asset;
    const auto status=PreparedSample::fromWav(
        wav.data(),wav.size(),rate,0.0,1.f,pan,0,false,asset);
    require(status==PrepareStatus::Ok&&asset,"PCM16 preparation failed");
    return asset;
}
static void exerciseTransferAndOwnership() {
    const auto pcmA=pcm16(16384);
    const auto pcmB=pcm16(-16384);
    SampleExchange exchanger;
    SampleVoiceEngine voices;
    require(voices.setOutputRate(48000),"voice rate");
    auto a=makeAsset(pcmA);
    const auto* aIdentity=a.get();
    require(!exchanger.publish(kRows,a),"out of bounds publication");
    require(exchanger.publish(0,a)&&!a,"first publication");
    auto b=makeAsset(pcmB);
    const auto* bIdentity=b.get();
    require(!exchanger.publish(0,b)&&b.get()==bIdentity,
            "second publish did not retain caller ownership");
    require(exchanger.audioAdopt(voices,48000).adopted==1,"first audio adoption");
    const Trigger hit{0,0,0,127,0};
    std::array<float,128> left{},right{};
    require(voices.render(&hit,1,left.data(),right.data(),
                          static_cast<std::uint32_t>(left.size())).accepted==1,
            "first audible trigger");
    require(voices.references(aIdentity),"first asset not tracked as active");
    require(!exchanger.pollReclaim(),"active asset prematurely reclaimed");
    require(exchanger.publish(0,b)&&!b,"new sound queued");
    require(exchanger.audioAdopt(voices,48000).adopted==1,
            "replacement not adopted");
    require(voices.references(aIdentity),"sample swap killed old voice");
    require(!exchanger.pollReclaim(),"retired sample freed while still sounding");
    const auto mixed=voices.render(&hit,1,left.data(),right.data(),
                                   static_cast<std::uint32_t>(left.size()));
    require(mixed.accepted==1&&mixed.activeVoices==2,
            "replacement did not overlap old one-shot");
    require(std::abs(left[64])<0.005f,"old + new samples not both audible");
    exchanger.audioCollect(voices);
    require(!exchanger.pollReclaim(),"old sample incorrectly retired");
    std::array<float,2048> tailL{},tailR{};
    require(voices.render(nullptr,0,tailL.data(),tailR.data(),
                          static_cast<std::uint32_t>(tailL.size())).status==
            RenderStatus::Ok,"tail render");
    require(!voices.references(aIdentity)&&!voices.references(bIdentity),
            "voices failed to finish naturally");
    exchanger.audioCollect(voices);
    auto returned=exchanger.pollReclaim();
    require(returned.get()==aIdentity,"wrong asset reclaimed");
    returned.reset(); // caller's worker/control thread, not audio
    require(!exchanger.pollReclaim(),"unexpected extra reclamation");
}
static void exerciseRejectAndWithdraw() {
    const auto pcm=pcm16(8192);
    SampleExchange exchanger;
    SampleVoiceEngine voices;
    require(voices.setOutputRate(48000),"rate");
    auto invalid=makeAsset(pcm,44100);
    const auto* invalidIdentity=invalid.get();
    require(exchanger.publish(3,invalid)&&!invalid,"queue wrong-rate sample");
    auto stats=exchanger.audioAdopt(voices,48000);
    require(stats.adopted==0&&stats.wrongOutputRate==1,
            "host rate mismatch silently applied");
    exchanger.audioCollect(voices);
    auto recovered=exchanger.pollReclaim();
    require(recovered.get()==invalidIdentity,"wrong-rate asset not safely returned");
    require(!exchanger.withdraw(3),"rejected update remained pending");
    require(!exchanger.pollReclaim(),"wrong-rate asset returned twice");
    recovered.reset(); // worker thread disposes rejected asset
    auto cancelled=makeAsset(pcm,48000);
    const auto* cancelledIdentity=cancelled.get();
    require(exchanger.publish(3,cancelled),"queue cancellable asset");
    auto withdrawn=exchanger.withdraw(3);
    require(withdrawn.get()==cancelledIdentity,"withdraw did not transfer ownership");
    require(!exchanger.withdraw(3),"empty withdraw unexpectedly succeeded");
    auto valid=makeAsset(pcm);
    require(exchanger.publish(15,valid),"last row publication");
    require(exchanger.audioAdopt(voices,48000).adopted==1,
            "last row adoption");
    std::array<float,96> l{},r{};
    const Trigger t{0,15,0,127,0};
    require(voices.render(&t,1,l.data(),r.data(),96).accepted==1,
            "last row playback");
    require(std::abs(l[60]-.25f)<0.002,"PCM16 amplitude altered");
    require(std::abs(r[60])<0.0001,"hard pan unexpectedly stereo");
}
static void exerciseLiveSwapKeepsOtherRowAudible() {
    const auto steady=pcm16(8192,8192);
    const auto replacement=pcm16(-8192,8192);
    SampleExchange exchanger;
    SampleVoiceEngine voices;
    require(voices.setOutputRate(48000),"SOR04 output rate");
    auto first=makeAsset(steady);
    auto second=makeAsset(steady);
    require(exchanger.publish(0,first)&&exchanger.publish(1,second),
            "SOR04 initial assets publish");
    require(exchanger.audioAdopt(voices,48000).adopted==2,
            "SOR04 initial assets adopt");
    std::array<float,128> left{},right{};
    const Trigger sustained{0,1,0,127,0};
    require(voices.render(&sustained,1,left.data(),right.data(),128).accepted==1,
            "SOR04 unaffected row did not start");
    // Replace row 0 repeatedly while row 1 is sounding, across block
    // boundaries. The unchanged row must not be reset or drop a frame.
    for(int pass=0;pass<40;++pass) {
        auto candidate=makeAsset(pass%2?steady:replacement);
        require(exchanger.publish(0,candidate),"SOR04 replacement publish");
        require(exchanger.audioAdopt(voices,48000).adopted==1,
                "SOR04 replacement adoption");
        // Alternate 64/128-frame host callbacks; the remaining voice must
        // not restart, fade or disappear at either buffer size.
        const std::uint32_t frames=pass%2?64u:128u;
        require(voices.render(nullptr,0,left.data(),right.data(),frames).status==
                RenderStatus::Ok,"SOR04 render after replacement");
        for(std::size_t i=0;i<frames;++i)
            require(std::abs(left[i]-8192.f/32768.f)<0.002f,
                    "SOR04 unaffected instrument dropped out on replacement");
        exchanger.audioCollect(voices);
        for(;;) {
            auto old=exchanger.pollReclaim();
            if(!old)break;
        }
    }
}
static void exerciseFailedPreparationIsTransactional() {
    const auto wav=pcm16(12345);
    auto previous=makeAsset(wav);
    const auto* prior=previous.get();
    auto invalid=wav;
    invalid[0]='X';
    require(PreparedSample::fromWav(invalid.data(),invalid.size(),48000,0,1,0,0,
                                    false,previous)==PrepareStatus::InvalidWav,
            "malformed file accepted");
    require(previous.get()==prior,"failed decode overwrote prior asset");
    require(PreparedSample::fromWav(wav.data(),wav.size(),0,0,1,0,0,
                                    false,previous)==PrepareStatus::InvalidSettings,
            "invalid rate accepted");
    require(previous.get()==prior,"failed settings overwrote prior asset");
    require(PreparedSample::fromWav(wav.data(),wav.size(),48000,100,1,0,0,
                                    false,previous)==PrepareStatus::InvalidSettings,
            "unbounded pitch accepted");
    require(previous.get()==prior,"invalid pitch overwrote prior asset");
}
int main() {
    try {
        exerciseTransferAndOwnership();
        exerciseLiveSwapKeepsOtherRowAudible();
        exerciseRejectAndWithdraw();
        // M32: reject a sequence of defective replacements without touching
        // the last successfully prepared sound or its ownership.
        {
          auto stable=makeAsset(pcm16(4096));
          const auto* identity=stable.get();
          auto bad=pcm16(4096);
          for(int attempt=0;attempt<12;++attempt) {
            bad[static_cast<std::size_t>(attempt%4)]^=0x40;
            require(PreparedSample::fromWav(bad.data(),bad.size(),48000,
                         0,1,0,0,false,stable)==PrepareStatus::InvalidWav,
                    "corrupt sample replacement accepted");
            require(stable.get()==identity,
                    "failed sample replacement discarded valid sound");
            bad=pcm16(4096);
          }
        }
        std::cout<<"M32 repeated invalid sample replacement rollback PASS\\n";
        exerciseFailedPreparationIsTransactional();
        std::cout<<"Drumator WAV-to-voice exchange: 3 groups PASS\n";
        return 0;
    }catch(const std::exception& ex) {
        std::cerr<<"Drumator sample exchange FAIL: "<<ex.what()<<'\n';
        return 1;
    }
}
