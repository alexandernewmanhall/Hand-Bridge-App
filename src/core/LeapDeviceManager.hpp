#pragma once
#include <string>
#include <vector>
#include <functional>
#include "FrameData.hpp"

// Interface for LeapDeviceManager (for DI & testability)
class ILeapDeviceManager {
public:
    using FrameCallback = std::function<void(const std::string& deviceId, const FrameData&)>;
    virtual ~ILeapDeviceManager() = default;
    virtual void poll() = 0; // Poll devices for new frames
    virtual void registerFrameCallback(FrameCallback cb) = 0;
};

// Concrete implementation
class LeapDeviceManager : public ILeapDeviceManager {
public:
    void poll() override;
    void registerFrameCallback(FrameCallback cb) override;
private:
    std::vector<FrameCallback> callbacks_;
    // ... internal device state (device list, etc.)
};
