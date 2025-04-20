#pragma once

#include "core/FrameData.hpp"

// Supplies a stream of FrameData
class IFrameSource {
public:
    virtual ~IFrameSource() = default;
    virtual bool getNextFrame(FrameData& outFrame) = 0;
    // Add more as needed for frame streaming
};
