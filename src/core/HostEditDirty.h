// Common UI-thread host dirty notification policy. Do not mark rejected
// edits as song changes. Caller supplies the optional host handler.
#pragma once
namespace drumator {
template<class Callback>
void markSuccessfulHostEdit(bool accepted,Callback&& notify) {
 if(accepted)notify();
}
}
