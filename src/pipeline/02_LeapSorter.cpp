#include "02_LeapSorter.hpp"
#include <algorithm> // Needed for std::transform
#include <cctype>    // Needed for ::toupper
// #include "FrameData.hpp" // REMOVED - Included via header
// Renamed: 02_LeapSorter

LeapSorter::LeapSorter(FilteredFrameCallback onFilteredFrame) // Renamed: 02_LeapSorter
    : onFilteredFrame_(std::move(onFilteredFrame)) {}

void LeapSorter::setDeviceHand(const std::string& serialNumber, const std::string& handType) {
    std::lock_guard<std::mutex> lock(assignmentMutex_);
    if (handType.empty()) {
        deviceHandAssignments_.erase(serialNumber);
        LOG("Cleared hand assignment for device: " << serialNumber);
    } else {
        deviceHandAssignments_[serialNumber] = handType;
        LOG("Assigned device " << serialNumber << " to hand: " << handType);
    }
    // TODO: Persist this change? (e.g., call configManager->setDefaultHandAssignment(serialNumber, handType))
}

void LeapSorter::processFrame(const std::string& serialNumber, const FrameData& frame) {
    bool hasHands = !frame.hands.empty();
    // Only log processing if hands are present - COMMENTED OUT
    // if (hasHands) {
    //     LOG("[LeapSorter] Processing frame for SN: " << serialNumber.c_str() << " with " << frame.hands.size() << " hands.");
    // }

    FrameData filteredFrame;
    filteredFrame.deviceId = frame.deviceId;
    filteredFrame.timestamp = frame.timestamp;

    auto it = deviceHandAssignments_.find(serialNumber);
    if (it == deviceHandAssignments_.end()) {
        // No specific assignment, pass through all hands
        filteredFrame.hands = frame.hands;
    } else {
        const std::string& assigned = it->second;
        if (assigned.empty() || assigned == "NONE") {
             // Explicitly assigned to NONE or empty string, pass through all hands
            filteredFrame.hands = frame.hands;
        } else {
            // Filter based on assigned hand type
            for (const auto& hand : frame.hands) {
                 std::string handTypeLower = hand.handType;
                 std::string handTypeUpper = handTypeLower;
                 std::transform(handTypeUpper.begin(), handTypeUpper.end(), handTypeUpper.begin(), ::toupper);
                 bool match = (handTypeUpper == assigned);
                 LOG("[LeapSorter] SN: " << serialNumber << " | Assigned: '" << assigned << "' | Hand type (raw): '" << handTypeLower << "' | Hand type (upper): '" << handTypeUpper << "' | Match: " << (match ? "YES" : "NO"));
                 // Compare uppercase version with the assignment (which is expected to be uppercase)
                 if (match) { 
                     filteredFrame.hands.push_back(hand);
                 }
            }
        }
    }

    // Only call callback if there's a listener
    if (onFilteredFrame_) {
        onFilteredFrame_(serialNumber, filteredFrame);
    } else if (hasHands) { // Log warning only if we dropped a frame with hands
        LOG("WARN: [LeapSorter] SN: " << serialNumber.c_str() << " - onFilteredFrame_ callback is null! Dropping frame with hands.");
    }
}
