#include "core/PreparedKitNames.h"
#include "core/ZoomRecallPolicy.h"
#include "core/KitEditGeneration.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include <type_traits>
namespace {
 // Deterministic throw at allocation N; disabled when negative.
 int failAt=-1;
}
void* operator new(std::size_t size) {
 if(failAt==0)throw std::bad_alloc{};
 if(failAt>0)--failAt;
 if(auto* p=std::malloc(size?size:1))return p;
 throw std::bad_alloc{};
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
#define CHECK(expr) do {if(!(expr)) {std::cerr<<"Recall transaction failure at "<<__LINE__<<": " #expr "\n";return __LINE__;}}while(false)
int main() {
 using Names=std::array<std::string,16>;
 using Prepared=drumator::PreparedKitNames<16>;
 static_assert(std::is_nothrow_swappable_v<Names>);
 Names liveCustom{},liveLabels{},wantedCustom{},wantedLabels{};
 for(std::size_t i=0;i<16;++i) {
   liveCustom[i]=std::string(90,'X');
   liveLabels[i]=std::string(90,'x');
   wantedCustom[i]=std::string(100,'Y');
   wantedLabels[i]=std::string(110,'y');
 }
 drumator::KitEditGeneration edits;
 const auto oldEpoch=edits.current();
 // Both the 16 custom names and 16 sample labels allocate during prepare.
 // Fault every one of those 32 stages; none may alter live data or tokens.
 for(int failure=0;failure<32;++failure) {
   Prepared staged;
   bool threw=false;
   failAt=failure;
   try {staged.prepare(wantedCustom,wantedLabels);}
   catch(const std::bad_alloc&) {threw=true;}
   failAt=-1;
   CHECK(threw);
   CHECK(edits.matches(oldEpoch));
   CHECK(liveCustom[0][0]=='X'&&liveLabels[15][0]=='x');
 }
 Prepared staged;
 staged.prepare(wantedCustom,wantedLabels);
 edits.onSuccessfulRecall();
 // The actual commit must allocate nothing, even under forced OOM.
 failAt=0;
 staged.commit(liveCustom,liveLabels);
 failAt=-1;
 CHECK(!edits.matches(oldEpoch));
 CHECK(liveCustom==wantedCustom&&liveLabels==wantedLabels);
 Names empty{};
 Prepared defaults;
 defaults.prepare(empty,empty);
 defaults.commit(liveCustom,liveLabels);
 CHECK(liveCustom==empty);
 CHECK(liveLabels[0]=="LEER - WAV LADEN"&&liveLabels[15]=="LEER - WAV LADEN");
 CHECK(drumator::zoomRecallIsAtomic(0));
 CHECK(drumator::zoomRecallIsAtomic(1));
 CHECK(!drumator::zoomRecallIsAtomic(2));
 return 0;
}
