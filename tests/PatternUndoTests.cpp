#include "core/PatternUndo.h"
using namespace drumator;
int main() {
 PatternUndo u;
 PatternSteps zero{},a{},b{},c{};
 a[0]=100;b[0]=127;c[kSteps+1]=64;
 if(u.available(0)||u.snapshot(0))return 1;
 if(u.capture(kPatterns,zero,a)||u.capture(0,zero,zero))return 2;
 if(!u.capture(0,zero,a)||*u.snapshot(0)!=zero)return 3;
 if(u.capture(0,a,a)||*u.snapshot(0)!=zero)return 4;
 if(!u.capture(0,a,b)||*u.snapshot(0)!=a)return 5;
 if(!u.capture(1,zero,c)||*u.snapshot(1)!=zero||*u.snapshot(0)!=a)return 6;
 u.consume(0);
 if(u.available(0)||!u.available(1))return 7;
 u.reset();
 if(u.available(1)||u.snapshot(1))return 8;
 // U09 duration-only Undo and ordinary step edits preserve duration.
 if(!u.capture(2,a,a,32,16)||u.snapshotLength(2)!=32||
    *u.snapshot(2)!=a)return 9;
 u.consume(2);
 if(u.snapshotLength(2)!=0)return 10;
 if(!u.capture(2,a,b,16,16)||u.snapshotLength(2)!=0)return 11;
 if(u.capture(2,b,b,16,16))return 12;
 u.reset();
 if(u.snapshotLength(2)!=0)return 13;
 return 0;
}
