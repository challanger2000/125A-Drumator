#pragma once
#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
namespace drumator::qa {
// Official Actions workflow-command escaping. Publish actual test outcomes
// through the Checks annotations API as well as the downloadable job log.
// Called only by test/reporting actors, never inside product process().
inline void workflowNotice(const std::string& message){
 std::string escaped;
 escaped.reserve(message.size());
 for(char ch:message){
   if(ch=='%')escaped+="%25";
   else if(ch=='\r')escaped+="%0D";
   else if(ch=='\n')escaped+="%0A";
   else escaped+=ch;
 }
 std::cout<<"::notice title=125A Drumator QA::"<<escaped<<'\n';
}
inline void result(const std::string& line){
 workflowNotice(line);
 char file[32768]{};
 const auto size=GetEnvironmentVariableA("GITHUB_STEP_SUMMARY",file,static_cast<DWORD>(sizeof(file)));
 if(size>0&&size<sizeof(file)){std::ofstream out(file,std::ios::app);out<<"- "<<line<<'\n';}
}
}
