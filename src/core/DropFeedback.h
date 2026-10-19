#pragma once
namespace drumator {
enum class DropFeedback {TargetBlocked, InvalidFilePath, ValidTarget};
constexpr DropFeedback classifyDrop(bool targetAllowed,bool validFilePath) noexcept {
 return !targetAllowed?DropFeedback::TargetBlocked:
        !validFilePath?DropFeedback::InvalidFilePath:
                       DropFeedback::ValidTarget;
}
}
