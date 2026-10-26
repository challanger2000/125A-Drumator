#pragma once
#include <array>
#include <cstdint>
namespace drumator {
// Format sticky >0 dBFS output CLIP flags; an empty label is intentional
// in the normal state. Output names follow MAIN / OUT 2..16.
constexpr std::array<char,16> clipWarningText(std::uint32_t mask) noexcept {
 std::array<char,16> result{};
 mask&=0xffffu;
 if(!mask)return result;
 constexpr char prefix[]="CLIP ";
 for(unsigned i=0;i<5;++i)result[i]=prefix[i];
 for(unsigned bus=0;bus<16;++bus)if(mask&(1u<<bus)){
   if(bus==0){result[5]='M';result[6]='A';result[7]='I';result[8]='N';}
   else {
     const unsigned index=bus+1;
     if(index>=10){result[5]=static_cast<char>('0'+index/10);result[6]=static_cast<char>('0'+index%10);}
     else result[5]=static_cast<char>('0'+index);
   }
   break;
 }
 return result;
}
}
