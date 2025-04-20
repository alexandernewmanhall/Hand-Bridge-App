#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <map>
#include <mutex>
#include <vector>
#include "../core/Log.hpp"
#include "../core/FrameData.hpp"

class LeapSorter {
public:
    // Ensure callback passes serialNumber
    using FilteredFrameCallback = std::function<void(const std::string& serialNumber, const FrameData& frame)>;

    explicit LeapSorter(FilteredFrameCallback onFilteredFrame);

    // Assign device to hand type ("left", "right", or "")
    void setDeviceHand(const std::string& deviceId, const std::string& handType);

    // Process a frame for a device
    void processFrame(const std::string& deviceId, const FrameData& frame);

private:
    std::map<std::string, std::string> deviceHandAssignments_;
    FilteredFrameCallback onFilteredFrame_;
    std::mutex assignmentMutex_;
};
