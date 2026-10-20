#pragma once
#include <cstddef>
namespace drumator::qa {
enum Point { none, patternMidWrite, patternReadStarted, restoreSnapshot,
             restoreMidCommit, restoreBeforeSelection };
// Inserted only in generated test translation units, never the shipped VST3.
void probe(Point point) noexcept;
}
