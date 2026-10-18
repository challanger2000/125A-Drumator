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
    original.visible=6;original.selected=5;
    original.muted=0x8001;original.solo=0x0006;original.swing=35;original.humanize=70;
    original.gain[0]=50;original.pan[0]=0;original.pitch[0]=36;
    original.choke[0]=1;original.choke[15]=4;
    original.output[0]=7;original.output[15]=0;
    original.gain[15]=200;original.pan[15]=200;
    original.grid[AtomicPatternBank::index(5,2,31)]=95;
    original.grid[AtomicPatternBank::index(0,15,0)]=127;
    original.names[0]="Kick_01.wav";
    original.wav[0]=wav16();
    original.names[2]="HiHat_Closed.wav";
    original.wav[2]=wav16();
    std::vector<std::uint8_t> encoded;
    check(KitCodec::encode(original,encoded),"Unable to encode kit");
    check(KitCodec::u32(encoded.data())==kKitMagic,"DMR4 format ID");
    KitState copy{};
    check(KitCodec::decode(encoded.data(),encoded.size(),copy),"Unable to decode");
    check(copy.muted==0x8001&&copy.solo==0x0006&&copy.swing==35&&copy.humanize==70,
          "Mute/solo/swing state lost");
    check(copy.gain==original.gain&&copy.pan==original.pan&&
          copy.pitch==original.pitch&&copy.choke==original.choke&&
          copy.output==original.output,
          "Volume/pan/pitch/choke state lost");
    check(copy.visible==6&&copy.selected==5,"Song layout/program not restored");
    check(copy.grid==original.grid,"Step matrix not byte-identical");
    check(copy.names==original.names,"Sample names not restored");
    check(copy.wav==original.wav,"Source WAV bytes not identical");
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
    // Remove the DM11 output table to create a legitimate DM10 fixture.
    auto legacyBase=encoded;
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
    check(!KitCodec::encode(full,again),"Oversized single WAV accepted");
    check(encoded==previous,"Rejected single WAV modified prior bytes");
    for(std::size_t row=4;row<kRows;++row) {full.wav[row].clear();full.names[row].clear();}
    full.wav[0].resize(kMaxSingleWav,0);
    for(std::size_t row=1;row<4;++row)
      full.wav[row].resize(kMaxSingleWav,0);
    check(KitCodec::encode(full,again),"Exact 64 MiB kit boundary rejected");
    check(again.size()>kMaxKitWavs,"State header not accounted for");
    full.names[4]="Excess.wav";full.wav[4]=wav16();
    check(!KitCodec::encode(full,again),
          "Total kit above 64 MiB accepted");
    std::cout<<"M11 sixteen-row recall and WAV size boundaries PASS\n";
    std::cout<<"Embedded Drumator kit DMR4 encode/decode and rollback: PASS\n";
 }catch(const std::exception& e){
    std::cerr<<"Embedded kit state FAIL: "<<e.what()<<'\n';
    return 1;
 }
}
