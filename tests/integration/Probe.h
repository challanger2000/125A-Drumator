#pragma once
#include <cstddef>
#include <atomic>
namespace drumator::qa {
extern thread_local bool legacyTriggerOrdering; // regression negative, test EXE only
enum class AuditMutation { none, suppressZeroStop, staleDuration, skipPeerCommit,
 basenameOnly, delayMidiOne, dropMidi, doubleMidi, zeroPrefix,
 skipImport, muteImportedProcessor, delayImportedProcessor,
 rejectPreviewStop, rejectPreviewStopOnce, patternQueryOOM, patternReplyOOM,
 oldFadeRenderer, oldKnobDrawCenters };
extern std::atomic<AuditMutation> auditMutation;
extern std::atomic<unsigned> auditSongScheduleCalls;
extern std::atomic<const void*> auditImportTarget;
extern std::atomic<unsigned> auditImportCalls,auditDelayCalls;
enum Point { none, patternMidWrite, patternReadStarted, restoreSnapshot,
             restoreMidCommit, restoreBeforeSelection, songMidPublish,
             auditAfterLengthRead, auditAfterAuditionAdopt, auditAfterAuditionStartTaken,
             auditAfterAuditionAssetPublished };
// Inserted only in generated test translation units, never the shipped VST3.
void probe(Point point) noexcept;
}
