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
    check(copy.visible==6&&copy.selected==5,"Song layout/program not restored");
    check(copy.grid==original.grid,"Step matrix not byte-identical");
    check(copy.names==original.names,"Sample names not restored");
    check(copy.wav==original.wav,"Source WAV bytes not identical");
    std::vector<std::uint8_t> again;
    check(KitCodec::encode(copy,again)&&encoded==again,
          "Roundtrip must produce deterministic DMR4");
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
    // Both independent caps must reject, without modifying an earlier result.
    const auto previous=encoded;
    full.wav[0].resize(kMaxSingleWav+1,0);
    check(!KitCodec::encode(full,again),"Oversized single WAV accepted");
    check(encoded==previous,"Rejected single WAV modified prior bytes");
    for(std::size_t row=4;row<kRows;++row) {full.wav[row].clear();full.names[row].clear();}\n    full.wav[0].resize(kMaxSingleWav,0);
    for(std::size_t row=1;row<4;++row)
      full.wav[row].resize(kMaxSingleWav,0);
    check(KitCodec::encode(full,again),"Exact 64 MiB kit boundary rejected");
    check(again.size()>kMaxKitWavs,"State header not accounted for");
    full.names[4]="Excess.wav";full.wav[4]=wav16();
    check(!KitCodec::encode(full,again),
          "Total kit above 64 MiB accepted");
    std::cout<<"M11 sixteen-row recall and WAV size boundaries PASS\\n";
    std::cout<<"Embedded Drumator kit DMR4 encode/decode and rollback: PASS\n";
 }catch(const std::exception& e){
    std::cerr<<"Embedded kit state FAIL: "<<e.what()<<'\n';
    return 1;
 }
}
