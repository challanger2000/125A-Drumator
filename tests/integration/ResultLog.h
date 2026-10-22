#pragma once
#include <windows.h>
#include <fstream>
#include <iostream>
#include <string>
namespace drumator::qa {
// Official Actions workflow-command escaping. Publish actual test outcomes
// through the Checks annotations API as well as the downloadable job log.
// Called only by test/reporting actors, never inside product process().
// Audit fixture may aggregate notices to avoid the per-step GitHub annotation cap.
// Set/read on its UI thread only; packaged-host defaults remain unchanged.
inline std::string* collectedNotices=nullptr;
inline void workflowNotice(const std::string& message){
 if(collectedNotices){*collectedNotices+=message;*collectedNotices+='\n';return;}
 std::string escaped;
 escaped.reserve(message.size());
 for(char ch:message){
   if(ch=='%')escaped+="%25";
   else if(ch=='\r')escaped+="%0D";
   else if(ch=='\n')escaped+="%0A";
   else escaped+=ch;
 }
 // Older test output may end in a literal "\\n" instead of a newline.
 // Workflow commands must start on a real line to remain individually visible.
 std::cout<<'\n'<<"::notice title=125A Drumator QA::"<<escaped<<'\n';
}
inline void result(const std::string& line){
 workflowNotice(line);
 char file[32768]{};
 const auto size=GetEnvironmentVariableA("GITHUB_STEP_SUMMARY",file,static_cast<DWORD>(sizeof(file)));
 if(size>0&&size<sizeof(file)){std::ofstream out(file,std::ios::app);out<<"- "<<line<<'\n';}
}
}
