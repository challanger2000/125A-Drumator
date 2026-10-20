#pragma once
#include <windows.h>
#include <fstream>
#include <string>
namespace drumator::qa {
inline void result(const std::string& line){
 char file[32768]{};
 const auto size=GetEnvironmentVariableA("GITHUB_STEP_SUMMARY",file,static_cast<DWORD>(sizeof(file)));
 if(size>0&&size<sizeof(file)){std::ofstream out(file,std::ios::app);out<<"- "<<line<<'\n';}
}
}
