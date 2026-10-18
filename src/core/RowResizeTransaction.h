// Reversible GUI row count update; unlike GUI layout this policy is
// pure and can be tested with simulated rejecting host callbacks.
#pragma once
namespace drumator {
enum class RowResizeResult { Applied, EditRejected, Reverted, RollbackFailed };
template<class Edit,class Resize,class Revert>
RowResizeResult tryRowResizeTransaction(Edit edit,Resize resize,Revert revert) {
 if(!edit())return RowResizeResult::EditRejected;
 if(resize())return RowResizeResult::Applied;
 return revert()?RowResizeResult::Reverted:RowResizeResult::RollbackFailed;
}
}
