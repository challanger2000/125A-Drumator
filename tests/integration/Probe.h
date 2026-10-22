#pragma once
#include <cstddef>
namespace drumator::qa {
extern thread_local bool legacyTriggerOrdering; // regression negative, test EXE only
enum Point { none, patternMidWrite, patternReadStarted, restoreSnapshot,
             restoreMidCommit, restoreBeforeSelection, songMidPublish,
             auditAfterLengthRead };
// Inserted only in generated test translation units, never the shipped VST3.
void probe(Point point) noexcept;
}
