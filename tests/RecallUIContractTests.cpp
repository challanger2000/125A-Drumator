#include "core/KitEditGeneration.h"
#include "core/DropFeedback.h"
#include <iostream>
#define CHECK(expr) do { if(!(expr)) {std::cerr<<"UI lifecycle check failed at "<<__LINE__<<": " #expr "\n";return __LINE__;}}while(false)
int main() {
 drumator::KitEditGeneration epoch;
 const auto kitX=epoch.current();
 CHECK(epoch.matches(kitX));
 // Failed recall must not advance generation.
 CHECK(epoch.matches(kitX));
 epoch.onSuccessfulRecall();
 CHECK(!epoch.matches(kitX));
 CHECK(epoch.matches(epoch.current()));
 const auto kitY=epoch.current();
 epoch.onSuccessfulRecall();
 CHECK(!epoch.matches(kitY)&&!epoch.matches(0));
 using drumator::DropFeedback;
 using drumator::classifyDrop;
 CHECK(classifyDrop(false,false)==DropFeedback::TargetBlocked);
 CHECK(classifyDrop(false,true)==DropFeedback::TargetBlocked);
 CHECK(classifyDrop(true,false)==DropFeedback::InvalidFilePath);
 CHECK(classifyDrop(true,true)==DropFeedback::ValidTarget);
 return 0;
}
