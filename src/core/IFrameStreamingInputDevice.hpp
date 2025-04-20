#pragma once
#include "IInputDevice.hpp"
#include "core/FrameCallback.hpp"
#include "../pipeline/01_LeapPoller.hpp" // Include for callback type definitions
#include <functional> // Include for std::function
#include <string> // Include for std::string

// Interface for input devices that can stream frames
class IFrameStreamingInputDevice : public IInputDevice {
public:
    // Define callback types based on LeapPoller for now
    using DeviceConnectedCallback = LeapPoller::DeviceConnectedCallback; // Or std::function<void(const LeapPoller::DeviceInfo&)>
    using DeviceLostCallback = LeapPoller::DeviceLostCallback; // Or std::function<void(const std::string&)>    

    virtual void setFrameCallback(FrameCallback cb) = 0;
    // Add callback setters
    virtual void setDeviceConnectedCallback(DeviceConnectedCallback cb) = 0;
    virtual void setDeviceLostCallback(DeviceLostCallback cb) = 0;
};
