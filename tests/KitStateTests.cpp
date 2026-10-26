#include "core/KitState.h"
#include "core/AtomicPatternBank.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace drumator;
static void check(bool ok,const char* message) {
 if(!ok)throw std::runtime_error(message);
}
static std::vector<std::uint8_t> wav16() {
 std::vector<std::uint8_t> w{};
 auto id=[&](const char* p){for(int i=0;i<4;++i)w.push_back(static_cast<std::uint8_t>(p[i]));};
 auto put32=[&](std::uint32_t value) {
   for(int i=0;i<4;++i)w.push_back(static_cast<std::uint8_t>(value>>(i*8)));
 };
 auto put16=[&](std::uint16_t value){
   w.push_back(static_cast<std::uint8_t>(value));
   w.push_back(static_cast<std::uint8_t>(value>>8));
 };
 id("RIFF");put32(36+4);id("WAVE");
 id("fmt ");put32(16);put16(1);put16(1);
 put32(44100);put32(88200);put16(2);put16(16);
 id("data");put32(4);put16(16384);put16(8192);
 return w;
}
int main() {
 try {
    KitState original{};
    check(original.songStartBar==1,"factory SONG START not BAR 1");
    original.songStartBar=2;
    for(const auto bus:original.output)check(bus==0,"new kit not MAIN routed");
    original.visible=6;original.selected=5;
    original.muted=0x8001;original.solo=0x0006;original.swing=35;original.humanize=70;
    original.songEnabled=1;
    original.randomVelocity[0]=45;
    original.randomVelocity[15]=80;
    original.masterVolume=73;
    original.patternLengths[0]=16;
    original.patternLengths[5]=16;
    check(original.chain.setCount(3)&&original.chain.configure(0,0,16)&&
          original.chain.configure(1,SongChain::kRest,8)&&original.chain.configure(2,2,32),
          "DM12 chain setup");
    original.gain[0]=50;original.pan[0]=0;original.pitch[0]=36;
    original.choke[0]=1;original.choke[15]=4;
    original.output[0]=7;original.output[15]=0;
    original.gain[15]=200;original.pan[15]=200;
    original.grid[AtomicPatternBank::index(5,2,31)]=95;
    original.grid[AtomicPatternBank::index(0,15,0)]=127;
    original.names[0]="Kick_01.wav";
    original.displayNames[0]="KICK";
    original.displayNames[3]="Closed Hat";
    original.displayNames[6]="Buehne";
    original.wav[0]=wav16();
    original.sampleEdits[0]={0,1,0,0,.5f};
    original.names[2]="HiHat_Closed.wav";
    original.wav[2]=wav16();
    std::vector<std::uint8_t> encoded;
    check(KitCodec::encode(original,encoded),"Unable to encode kit");
    check(KitCodec::u32(encoded.data())==kKitMagic,"DMR4 format ID");
     // DM16 sample edits must be serialized losslessly; old DM15 projects
     // will default to unedited samples after migration.
     check(encoded.size()>=kRows*kSampleEditBytes+4,"DM16 edit tail missing");
    KitState copy{};
    check(KitCodec::decode(encoded.data(),encoded.size(),copy),"Unable to decode");
    check(copy.muted==0x8001&&copy.solo==0x0006&&copy.swing==35&&copy.humanize==70,
          "Mute/solo/swing state lost");
    check(copy.gain==original.gain&&copy.pan==original.pan&&
          copy.pitch==original.pitch&&copy.choke==original.choke&&
          copy.output==original.output,
          "Volume/pan/pitch/choke state lost");
    check(copy.visible==6&&copy.selected==5,"Song layout/program not restored");
    check(copy.songEnabled==1&&copy.chain.count()==3&&
          copy.chain.entry(0).steps==16&&copy.chain.entry(1).pattern==SongChain::kRest&&
          copy.chain.entry(1).steps==8&&copy.chain.entry(2).steps==32,
          "DM12 chain roundtrip");
    check(copy.songStartBar==2,"DM17 SONG START lost during roundtrip");
    check(copy.randomVelocity==original.randomVelocity&&copy.masterVolume==73,
          "DM13 random velocity/master roundtrip");
    check(copy.grid==original.grid,"Step matrix not byte-identical");
    check(copy.names==original.names,"Sample names not restored");
    check(copy.patternLengths==original.patternLengths,"DM15 pattern lengths lost");
    // DM16 stored no SONG START and could not contain REST. Reproduce
    // authentic predecessor layout by stripping DM17's trailing uint16,
    // adjusting version/length/CRC, then verify backward read compatibility.
    auto legacy16Source=original;
    legacy16Source.songStartBar=1;
    check(legacy16Source.chain.configure(1,1,8),"DM16 fixture pattern");
    std::vector<std::uint8_t> beforeDM16;
    check(KitCodec::encode(legacy16Source,beforeDM16),"DM17 migration fixture");
    beforeDM16.erase(beforeDM16.end()-6,beforeDM16.end()-4);
    for(unsigned i=0;i<4;++i){
       beforeDM16[i]=static_cast<std::uint8_t>(kKitMagicV16>>(i*8));
       beforeDM16[4+i]=static_cast<std::uint8_t>((beforeDM16.size()-8)>>(i*8));
    }
    const auto previousHash=KitCodec::hash(beforeDM16.data()+8,beforeDM16.size()-12);
    for(unsigned i=0;i<4;++i)
       beforeDM16[beforeDM16.size()-4+i]=static_cast<std::uint8_t>(previousHash>>(i*8));
    KitState migrated16;
    check(KitCodec::decode(beforeDM16.data(),beforeDM16.size(),migrated16)&&
          migrated16.songStartBar==1&&migrated16.chain.entry(1).pattern==1&&
          migrated16.grid==legacy16Source.grid&&migrated16.output==legacy16Source.output,
          "DM16 backward migration to BAR 1 failed");
    auto invalidLength=original;
    invalidLength.patternLengths[1]=8;
    std::vector<std::uint8_t> rejectLength;
    check(!KitCodec::encode(invalidLength,rejectLength),"Invalid DM15 length serialized");
    check(copy.displayNames==original.displayNames,"DM14 independent labels not recalled");
    check(validRowLabel("Gruen")&&!validRowLabel(std::string(65,'X'))&&
          !validRowLabel(std::string({static_cast<char>(0xc0),static_cast<char>(0xaf)}))&&
          !validRowLabel(std::string("Bad\nLine")),
          "DM14 label validation");
    check(copy.wav==original.wav,"Source WAV bytes not identical");
    check(copy.sampleEdits[0].start==0&&copy.sampleEdits[0].end==1&&
          copy.sampleEdits[0].gain==.5f,"DM16 sample edit roundtrip lost");
    std::vector<std::uint8_t> again;
    check(KitCodec::encode(copy,again)&&encoded==again,
          "Roundtrip must produce deterministic DMR4");
    // DM11 rejects out-of-range bus assignments without replacing saved state.
    KitState invalidBus=original;
    invalidBus.output[3]=16;
    std::vector<std::uint8_t> invalidBusBytes;
    check(!KitCodec::encode(invalidBus,invalidBusBytes),
          "invalid output bus was serialized");
    auto tamperedBus=encoded;
    tamperedBus[78+3]=16;
    const auto correctedCrc=KitCodec::hash(tamperedBus.data()+8,tamperedBus.size()-12);
    for(int i=0;i<4;++i)
      tamperedBus[tamperedBus.size()-4+static_cast<std::size_t>(i)]=
          static_cast<std::uint8_t>(correctedCrc>>(8*i));
    KitState preservedRouting=copy;
    check(!KitCodec::decode(tamperedBus.data(),tamperedBus.size(),copy),
          "out-of-range DM11 bus accepted despite valid checksum");
    check(copy.output==preservedRouting.output&&copy.grid==preservedRouting.grid,
          "rejected bus changed previously loaded kit");
    // First strip the DM16 WAV-edit extension to recreate an actual DM15.
    auto old15=beforeDM16; // DM16 layout, without the appended DM17 start bar.
    old15.erase(old15.end()-static_cast<std::ptrdiff_t>(4+kRows*kSampleEditBytes),old15.end()-4);
    for(int i=0;i<4;++i)
      old15[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV15>>(8*i));
    const auto old15Size=static_cast<std::uint32_t>(old15.size()-8);
    for(int i=0;i<4;++i)
      old15[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(old15Size>>(8*i));
    const auto old15Hash=KitCodec::hash(old15.data()+8,old15.size()-12);
    for(int i=0;i<4;++i)
      old15[old15.size()-4+static_cast<std::size_t>(i)]=
          static_cast<std::uint8_t>(old15Hash>>(8*i));
    KitState migrated15;
    check(KitCodec::decode(old15.data(),old15.size(),migrated15)&&
          migrated15.sampleEdits[0].end==0,
          "DM15 WAV-edit default migration failed");
    // Strip the DM15 eight-byte length extension to recreate DM14.
    auto old14=old15;
    old14.erase(old14.end()-static_cast<std::ptrdiff_t>(4+kPatterns),old14.end()-4);
    for(int i=0;i<4;++i)
      old14[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV14>>(8*i));
    const auto old14Size=static_cast<std::uint32_t>(old14.size()-8);
    for(int i=0;i<4;++i)old14[4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old14Size>>(8*i));
    const auto old14Hash=KitCodec::hash(old14.data()+8,old14.size()-12);
    for(int i=0;i<4;++i)old14[old14.size()-4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old14Hash>>(8*i));
    KitState restored14;
    check(KitCodec::decode(old14.data(),old14.size(),restored14)&&
          restored14.displayNames==original.displayNames&&
          restored14.patternLengths[0]==32&&restored14.patternLengths[5]==32,
          "DM14 backwards compatibility failed");
    // DM14 appends per-row labels. Remove them to reproduce DM13.
    std::size_t labelsSize=kRows;
    for(const auto& label:original.displayNames)labelsSize+=label.size();
    auto old13=old14;
    old13.erase(old13.end()-static_cast<std::ptrdiff_t>(4+labelsSize),old13.end()-4);
    for(int i=0;i<4;++i)
      old13[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV13>>(8*i));
    const auto old13Length=static_cast<std::uint32_t>(old13.size()-8);
    for(int i=0;i<4;++i)old13[4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old13Length>>(8*i));
    const auto old13Hash=KitCodec::hash(old13.data()+8,old13.size()-12);
    for(int i=0;i<4;++i)old13[old13.size()-4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old13Hash>>(8*i));
    KitState migrated13;
    check(KitCodec::decode(old13.data(),old13.size(),migrated13)&&
          migrated13.displayNames[0].empty()&&
          migrated13.names[0]=="Kick_01.wav"&&migrated13.wav[0]==original.wav[0],
          "DM13 backwards compatible instrument name migration");
    // Corrupt display-label length with a corrected checksum, rejecting
    // the state without mutating its previous successful contents.
    auto badDisplay=encoded;
    const auto labelsBegin=badDisplay.size()-4-kRows*kSampleEditBytes-kPatterns-labelsSize;
    badDisplay[labelsBegin]=static_cast<std::uint8_t>(kMaxRowLabel+1);
    const auto badDisplayHash=KitCodec::hash(badDisplay.data()+8,badDisplay.size()-12);
    for(int i=0;i<4;++i)badDisplay[badDisplay.size()-4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(badDisplayHash>>(8*i));
    KitState beforeBad=copy;
    check(!KitCodec::decode(badDisplay.data(),badDisplay.size(),copy)&&
          copy.displayNames==beforeBad.displayNames&&copy.wav==beforeBad.wav,
          "DM14 invalid name length destroyed previous state");
    // Create a genuine DM12 fixture, migrating its absent mix/random fields.
    auto old12=old13;
    old12.erase(old12.begin()+226,old12.begin()+243);
    for(int i=0;i<4;++i)old12[static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(kKitMagicV12>>(8*i));
    const auto old12Size=static_cast<std::uint32_t>(old12.size()-8);
    for(int i=0;i<4;++i)old12[4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old12Size>>(8*i));
    const auto old12Hash=KitCodec::hash(old12.data()+8,old12.size()-12);
    for(int i=0;i<4;++i)old12[old12.size()-4+static_cast<std::size_t>(i)]=
        static_cast<std::uint8_t>(old12Hash>>(8*i));
    KitState restored12{};
    check(KitCodec::decode(old12.data(),old12.size(),restored12)&&
          restored12.masterVolume==100&&restored12.randomVelocity[0]==0&&
          restored12.songEnabled==1&&restored12.chain.count()==3,
          "DM12 migration must preserve SONG and default new controls");
    // Remove DM11 output table to create legitimate DM10 fixture.
    auto legacyBase=old12;
    // Remove DM12 song data at offset 96 before deriving older fixtures.
    legacyBase.erase(legacyBase.begin()+96,legacyBase.begin()+226);
    legacyBase.erase(legacyBase.begin()+78,legacyBase.begin()+94);
    for(int i=0;i<4;++i)
      legacyBase[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV10>>(8*i));
    const auto oldLength=static_cast<std::uint32_t>(legacyBase.size()-8);
    for(int i=0;i<4;++i)
      legacyBase[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(oldLength>>(8*i));
    const auto oldCrc=KitCodec::hash(legacyBase.data()+8,legacyBase.size()-12);
    for(int i=0;i<4;++i)
      legacyBase[legacyBase.size()-4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(oldCrc>>(8*i));
    KitState v10{};
    check(KitCodec::decode(legacyBase.data(),legacyBase.size(),v10)&&
          v10.output[0]==0&&v10.output[15]==15,"DM10 output migration");
    check(v10.songEnabled==0,"DM10 must restore old pattern mode");
    // Old DMR4 projects have no row masks and must reopen with both off.
    auto legacy=legacyBase;
    legacy.erase(legacy.begin()+10,legacy.begin()+80);
    for(int i=0;i<4;++i)
      legacy[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV4>>(8*i));
    const auto legacyLength=static_cast<std::uint32_t>(legacy.size()-8);
    for(int i=0;i<4;++i)
      legacy[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(legacyLength>>(8*i));
    const auto legacyCrc=KitCodec::hash(legacy.data()+8,legacy.size()-12);
    for(int i=0;i<4;++i)
      legacy[legacy.size()-4+static_cast<std::size_t>(i)]=
          static_cast<std::uint8_t>(legacyCrc>>(8*i));
    KitState prior{};
    check(KitCodec::decode(legacy.data(),legacy.size(),prior),
          "Legacy DMR4 project cannot reopen");
    check(prior.muted==0&&prior.solo==0&&prior.wav==original.wav&&
          prior.grid==original.grid,"Legacy migration damaged kit");
    auto priorV5=legacyBase;
    priorV5.erase(priorV5.begin()+14,priorV5.begin()+80);
    for(int i=0;i<4;++i)
      priorV5[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV5>>(8*i));
    const auto len5=static_cast<std::uint32_t>(priorV5.size()-8);
    for(int i=0;i<4;++i)
      priorV5[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(len5>>(8*i));
    const auto crc5=KitCodec::hash(priorV5.data()+8,priorV5.size()-12);
    for(int i=0;i<4;++i)
      priorV5[priorV5.size()-4+static_cast<std::size_t>(i)]=
          static_cast<std::uint8_t>(crc5>>(8*i));
    KitState migrated{};
    check(KitCodec::decode(priorV5.data(),priorV5.size(),migrated),
          "Legacy DMR5 project cannot reopen");
    check(migrated.muted==original.muted&&migrated.solo==original.solo&&
          migrated.gain[0]==100&&migrated.pan[0]==100,
          "DMR5 migration lost gates or neutral row mix");
    auto priorV6=legacyBase;
    priorV6.erase(priorV6.begin()+46,priorV6.begin()+80);
    for(int i=0;i<4;++i)
       priorV6[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV6>>(8*i));
    const auto len6=static_cast<std::uint32_t>(priorV6.size()-8);
    for(int i=0;i<4;++i)
       priorV6[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(len6>>(8*i));
    const auto crc6=KitCodec::hash(priorV6.data()+8,priorV6.size()-12);
    for(int i=0;i<4;++i)
       priorV6[priorV6.size()-4+static_cast<std::size_t>(i)]=
           static_cast<std::uint8_t>(crc6>>(8*i));
    KitState migrated6{};
    check(KitCodec::decode(priorV6.data(),priorV6.size(),migrated6),
          "Legacy DMR6 project cannot reopen");
    check(migrated6.pitch[0]==24&&migrated6.gain==original.gain&&
          migrated6.pan==original.pan,"DMR6 migration changed row mix");
    auto priorV7=legacyBase;
    priorV7.erase(priorV7.begin()+62,priorV7.begin()+80);
    for(int i=0;i<4;++i)
      priorV7[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV7>>(8*i));
    const auto len7=static_cast<std::uint32_t>(priorV7.size()-8);
    for(int i=0;i<4;++i)
      priorV7[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(len7>>(8*i));
    const auto crc7=KitCodec::hash(priorV7.data()+8,priorV7.size()-12);
    for(int i=0;i<4;++i)
      priorV7[priorV7.size()-4+static_cast<std::size_t>(i)]=
         static_cast<std::uint8_t>(crc7>>(8*i));
    KitState migrated7{};
    check(KitCodec::decode(priorV7.data(),priorV7.size(),migrated7),
          "Legacy DMR7 project cannot reopen");
    check(migrated7.choke[0]==0&&migrated7.pitch[0]==original.pitch[0],
          "DMR7 migration did not preserve pitch and disable choke");
    auto priorV8=legacyBase;
    // Keep checksum bytes; remove only swing byte at offset 78.
    priorV8.erase(priorV8.begin()+78,priorV8.begin()+80);
    for(int i=0;i<4;++i)
      priorV8[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV8>>(8*i));
    const auto len8=static_cast<std::uint32_t>(priorV8.size()-8);
    for(int i=0;i<4;++i)
      priorV8[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(len8>>(8*i));
    const auto crc8=KitCodec::hash(priorV8.data()+8,priorV8.size()-12);
    for(int i=0;i<4;++i)
      priorV8[priorV8.size()-4+static_cast<std::size_t>(i)]=
          static_cast<std::uint8_t>(crc8>>(8*i));
    KitState migrated8{};
    check(KitCodec::decode(priorV8.data(),priorV8.size(),migrated8),
          "Legacy DMR8 project cannot reopen");
    check(migrated8.swing==0&&migrated8.choke==original.choke,
          "DMR8 migration lost choke or did not reset swing");
    auto priorV9=legacyBase;
    priorV9.erase(priorV9.begin()+79,priorV9.begin()+80);
    for(int i=0;i<4;++i)
      priorV9[static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(kKitMagicV9>>(8*i));
    const auto len9=static_cast<std::uint32_t>(priorV9.size()-8);
    for(int i=0;i<4;++i)
      priorV9[4+static_cast<std::size_t>(i)]=static_cast<std::uint8_t>(len9>>(8*i));
    const auto crc9=KitCodec::hash(priorV9.data()+8,priorV9.size()-12);
    for(int i=0;i<4;++i)
      priorV9[priorV9.size()-4+static_cast<std::size_t>(i)]=
           static_cast<std::uint8_t>(crc9>>(8*i));
    KitState migrated9{};
    check(KitCodec::decode(priorV9.data(),priorV9.size(),migrated9),
          "Legacy DMR9 project cannot reopen");
    check(migrated9.humanize==0&&migrated9.swing==35,
          "DMR9 migration failed");
    auto damaged=encoded;
    damaged[20]^=1;
    check(!KitCodec::decode(damaged.data(),damaged.size(),copy),
          "Bitflipped state must fail CRC");
    check(copy.wav[0]==original.wav[0],"Failed decode must preserve old state");
    damaged=encoded;
    damaged.resize(damaged.size()-2);
    check(!KitCodec::decode(damaged.data(),damaged.size(),copy),
          "Truncated kit must fail");
    original.visible=17;
    check(!KitCodec::encode(original,again),"Out-of-range rows");
    original.visible=6;
    original.wav[1]=wav16();original.names[1].clear();
    check(!KitCodec::encode(original,again),"Unnamed embedded sample accepted");
    original.wav[1].clear();
    original.names[0]="../bad.wav";
    check(!KitCodec::encode(original,again),"Path traversal name accepted");
    original.names[0]="Kick_01.wav";
    // The raw-invalid-WAV fixture must not also retain a DM16 edit:
    // edited samples are intentionally decoded during encode validation.
    original.sampleEdits[0]=SampleEdit{};
    original.wav[0][0]='X'; // invalid WAV, checksum would be recomputed
    check(KitCodec::encode(original,again),"Raw invalid fixture creation");
    check(!KitCodec::decode(again.data(),again.size(),copy),
          "CRC-valid but invalid WAV must fail before host state commit");
    // M11: a populated 16-row kit must roundtrip with every filename and WAV intact.
    KitState full{};
    full.visible=16;
    for(std::size_t row=0;row<kRows;++row) {
      full.names[row]="Row_"+std::to_string(row)+".wav";
      full.wav[row]=wav16();
      full.grid[AtomicPatternBank::index(0,row,row%kSteps)]=
          static_cast<std::uint8_t>(row+1);
    }
    check(KitCodec::encode(full,encoded),"Full 16-row kit encode failed");
    check(KitCodec::decode(encoded.data(),encoded.size(),copy),
          "Full 16-row kit decode failed");
    check(copy.names==full.names&&copy.wav==full.wav&&copy.grid==full.grid,
          "Full 16-row kit did not roundtrip");
    // M32: malformed projects must fail atomically without losing the current kit.
    const auto valid16=encoded;
    for(std::size_t index : {std::size_t{0},std::size_t{4},std::size_t{8},
                              std::size_t{40},std::size_t{80},
                              valid16.size()/2,valid16.size()-1}) {
      auto corrupted=valid16;
      corrupted[index]^=0x5a;
      KitState preserved=copy;
      check(!KitCodec::decode(corrupted.data(),corrupted.size(),copy),
            "Corrupted 16-row project unexpectedly accepted");
      check(copy.wav==preserved.wav&&copy.names==preserved.names&&
            copy.grid==preserved.grid&&copy.selected==preserved.selected,
            "Failed 16-row recall changed the active project");
    }
    for(std::size_t length : {std::size_t{0},std::size_t{1},
                               std::size_t{7},std::size_t{64},
                               valid16.size()-1}) {
      KitState preserved=copy;
      check(!KitCodec::decode(valid16.data(),length,copy),
            "Truncated project unexpectedly accepted");
      check(copy.wav==preserved.wav&&copy.names==preserved.names&&
            copy.grid==preserved.grid,
            "Truncated project changed active kit");
    }
    std::cout<<"M32 corrupt and truncated 16-row project rollback PASS\\n";
    // Both independent caps must reject, without modifying an earlier result.
    const auto previous=encoded;
    full.wav[0].resize(kMaxSingleWav+1,0);
    const auto previousAgain=again;
    check(!KitCodec::encode(full,again),"Oversized single WAV accepted");
    check(again==previousAgain,"Rejected single WAV modified output buffer");
    check(encoded==previous,"Rejected single WAV modified prior bytes");
    for(std::size_t row=4;row<kRows;++row) {full.wav[row].clear();full.names[row].clear();}
    full.wav[0].resize(kMaxSingleWav,0);
    for(std::size_t row=1;row<4;++row)
      full.wav[row].resize(kMaxSingleWav,0);
    check(KitCodec::encode(full,again),"Exact 64 MiB kit boundary rejected");
    check(again.size()>kMaxKitWavs,"State header not accounted for");
    full.names[4]="Excess.wav";full.wav[4]=wav16();
    const auto atBudgetLimit=again;
    check(!KitCodec::encode(full,again),
          "Total kit above 64 MiB accepted");
    check(again==atBudgetLimit,
          "Rejected 64 MiB budget overflow modified previous encoded kit");
    std::cout<<"M11 sixteen-row recall and WAV size boundaries PASS\n";
    std::cout<<"Embedded Drumator kit DMR4 encode/decode and rollback: PASS\n";
 }catch(const std::exception& e){
    std::cerr<<"Embedded kit state FAIL: "<<e.what()<<'\n';
    return 1;
 }
}
