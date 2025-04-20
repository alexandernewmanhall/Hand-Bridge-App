#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include "HandData.hpp"

struct FrameData {
    std::string deviceId;
    uint64_t timestamp = 0;
    std::vector<HandData> hands;
    // Add frameId or other metadata as needed
};
