#pragma once
#include <cstdint>
namespace drumator {
class KitEditGeneration final {
public:
 using Token=std::uint64_t;
 Token current() const noexcept {return generation_;}
 bool matches(Token token) const noexcept {return token!=0&&token==generation_;}
 void onSuccessfulRecall() noexcept {
  ++generation_;
  if(generation_==0)++generation_;
 }
private:
 Token generation_{1};
};
}
